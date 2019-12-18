#include "Core/Math/Float.h"
#include "Core/Misc/SafeDestroy.h"
#include "Physics/Body.h"
#include "Physics/PhysicsManager.h"
#include "Physics/World/Vehicle/VehicleComponent.h"
#include "Physics/World/Vehicle/VehicleComponentData.h"
#include "Physics/World/Vehicle/Wheel.h"
#include "Physics/World/Vehicle/WheelData.h"
#include "World/Entity.h"

#include "Core/Log/Log.h"

namespace traktor
{
	namespace physics
	{
		namespace
		{

const float c_maxSuspensionForce = 250.0f;
const float c_maxDampingForce = 250.0f;
const Scalar c_slowGripCoeff = 1.0_simd;
const Scalar c_fastGripCoeff = 0.01_simd;
const float c_throttleThreshold = 0.01f;
const Scalar c_linearVelocityThreshold = 4.0_simd;
const float c_suspensionTraceRadius = 0.25f;

		}

T_IMPLEMENT_RTTI_CLASS(L"traktor.physics.VehicleComponent", VehicleComponent, world::IEntityComponent)

VehicleComponent::VehicleComponent(
	PhysicsManager* physicsManager,
	const VehicleComponentData* data,
	Body* body,
	const RefArray< Wheel >& wheels,
	uint32_t traceInclude,
	uint32_t traceIgnore
)
:	m_owner(nullptr)
,	m_physicsManager(physicsManager)
,	m_data(data)
,	m_body(body)
,	m_wheels(wheels)
,	m_traceInclude(traceInclude)
,	m_traceIgnore(traceIgnore)
,	m_totalMass(0.0_simd)
,	m_steerAngle(0.0f)
,	m_steerAngleTarget(0.0f)
,	m_engineThrottle(0.0f)
,	m_airBorn(true)
{
	m_totalMass = 1.0_simd / Scalar(m_body->getInverseMass());
}

void VehicleComponent::destroy()
{
	safeDestroy(m_body);
	m_owner = nullptr;
}

void VehicleComponent::setOwner(world::Entity* owner)
{
	if ((m_owner = owner) != nullptr)
	{
		Transform transform;
		if (m_owner->getTransform(transform))
		{
			m_body->setTransform(transform);
			m_body->setEnable(true);
		}
	}
}

void VehicleComponent::setTransform(const Transform& transform)
{
	m_body->setTransform(transform);
}

Aabb3 VehicleComponent::getBoundingBox() const
{
	return Aabb3();
}

void VehicleComponent::update(const world::UpdateParams& update)
{
	if (m_owner)
		m_owner->setTransform(m_body->getTransform());

	float dT = update.deltaTime;

	updateSteering(dT);
	updateSuspension(dT);
	updateFriction(dT);
	updateEngine(dT);
	updateWheels(dT);
}

void VehicleComponent::setSteerAngle(float steerAngle)
{
	m_steerAngleTarget = steerAngle;
}

float VehicleComponent::getSteerAngle() const
{
	return m_steerAngleTarget;
}

float VehicleComponent::getSteerAngleFiltered() const
{
	return m_steerAngle;
}

void VehicleComponent::setEngineThrottle(float engineThrottle)
{
	m_engineThrottle = engineThrottle;
}

float VehicleComponent::getEngineThrottle() const
{
	return m_engineThrottle;
}

void VehicleComponent::updateSteering(float dT)
{
	// Update steer angle, aiming for target angle.
	if (m_steerAngle < m_steerAngleTarget)
	{
		float dA = min(m_steerAngleTarget - m_steerAngle, m_data->getSteerAngleVelocity() * dT);
		m_steerAngle += dA;
	}
	else
	{
		float dA = min(m_steerAngle - m_steerAngleTarget, m_data->getSteerAngleVelocity() * dT);
		m_steerAngle -= dA;
	}

	// Update wheel direction from steering.
	Vector4 direction(std::sin(m_steerAngle), 0.0f, std::cos(m_steerAngle), 0.0f);
	Vector4 directionPerp(std::cos(m_steerAngle), 0.0f, -std::sin(m_steerAngle), 0.0f);
	for (auto wheel : m_wheels)
	{
		if (wheel->data->getSteer())
		{
			wheel->direction = direction;
			wheel->directionPerp = directionPerp;
		}
	}
}

void VehicleComponent::updateSuspension(float dT)
{
	physics::QueryResult result;

	Transform bodyT = m_body->getTransform();
	Transform bodyTinv = bodyT.inverse();

	m_airBorn = true;

	for (auto wheel : m_wheels)
	{
		const WheelData* data = wheel->data;
		T_ASSERT(data != nullptr);

		Vector4 anchorW = bodyT * data->getAnchor().xyz1();
		Vector4 axisW = bodyT * -data->getAxis().xyz0().normalized();

		float contactFudge = 0.0f;

		if (m_physicsManager->querySweep(
			anchorW,
			axisW,
			data->getSuspensionLength().max + data->getRadius() + m_data->getFudgeDistance(),
			c_suspensionTraceRadius,
			physics::QueryFilter(m_traceInclude, m_traceIgnore),
			result
		))
		{
			if (result.distance <= data->getSuspensionLength().max + data->getRadius())
				contactFudge = 1.0f;
			else
				contactFudge = 1.0f - (result.distance - (data->getSuspensionLength().max + data->getRadius())) / m_data->getFudgeDistance();
		}

		if (contactFudge >= FUZZY_EPSILON)
		{
			Vector4 normal = result.normal.normalized();

			// Suspension current length.
			float suspensionLength = result.distance - data->getRadius();

			// Clamp lengths.
			if (suspensionLength < data->getSuspensionLength().min)
				suspensionLength = data->getSuspensionLength().min;
			else if (suspensionLength > data->getSuspensionLength().max)
				suspensionLength = data->getSuspensionLength().max;

			// Suspension velocity.
			float suspensionVelocity = (wheel->suspensionLength - suspensionLength) / dT;

			// Suspension forces.
			float t = 1.0f - (suspensionLength - data->getSuspensionLength().min) / (data->getSuspensionLength().max - data->getSuspensionLength().min);
			float springForce = clamp(t * data->getSuspensionSpring(), 0.0f, c_maxSuspensionForce);
			float dampingForce = clamp(suspensionVelocity * data->getSuspensionDamping(), -c_maxDampingForce, c_maxDampingForce);

			// Apply forces.
			m_body->addForceAt(
				anchorW,
				normal * Scalar(springForce + dampingForce),
				false
			);

			// Apply sway-bar force on the opposite side.
			m_body->addForceAt(
				bodyT * (data->getAnchor() * Vector4(-1.0f, 1.0f, 1.0f, 1.0f)),
				normal * -Scalar((springForce + dampingForce) * m_data->getSwayBarForce()),
				false
			);

			// Save suspension state.
			wheel->suspensionLength = suspensionLength;

			// Contact attributes.
			Vector4 wheelVelocity = m_body->getVelocityAt(result.position.xyz1(), false);
			Vector4 groundVelocity = result.body->getVelocityAt(result.position.xyz1(), false);
			Vector4 velocity = wheelVelocity - groundVelocity;

			wheel->contact = true;
			wheel->contactFudge = contactFudge;
			wheel->contactMaterial = result.material;
			wheel->contactPosition = result.position.xyz1();
			wheel->contactNormal = normal;

			Scalar k = dot3(normal, velocity);
			wheel->contactVelocity = velocity - normal * (-k);

			m_airBorn = false;
		}
		else
		{
			wheel->suspensionLength = data->getSuspensionLength().max;

			wheel->contact = false;
			wheel->contactFudge = 0.0f;
			wheel->contactMaterial = 0;
			wheel->contactPosition = Vector4::origo();
			wheel->contactNormal = Vector4::zero();
			wheel->contactVelocity = Vector4::zero();
		}
	}
}

void VehicleComponent::updateFriction(float dT)
{
	Transform bodyT = m_body->getTransform();
	Transform bodyTinv = bodyT.inverse();

	Scalar rollingFriction = 0.0_simd;
	Scalar massPerWheel = m_totalMass / Scalar(m_wheels.size());

	for (auto wheel : m_wheels)
	{
		if (!wheel->contact)
			continue;

		const WheelData* data = wheel->data;
		T_ASSERT(data != nullptr);

		// Get suspension axis in world space.
		Vector4 axisW = bodyT * data->getAxis();

		// Wheel directions in world space.
		Vector4 directionW = bodyT * wheel->direction;
		Vector4 directionPerpW = bodyT * wheel->directionPerp;

		// Project wheel directions onto contact plane.
		directionW -= wheel->contactNormal * dot3(wheel->contactNormal, directionW);
		directionPerpW -= wheel->contactNormal * dot3(wheel->contactNormal, directionPerpW);
		directionW = directionW.normalized();
		directionPerpW = directionPerpW.normalized();

		// Determine velocities and percent of maximum velocity.
		Scalar forwardVelocity = dot3(directionW, wheel->contactVelocity);
		Scalar sideVelocity = dot3(directionPerpW, wheel->contactVelocity);

		// \tbd Should use absolute contact position movement to determine velocity; this will introduce error creep.
		if (abs(forwardVelocity) > FUZZY_EPSILON)
		{
			// Calculate slip angle.
			float k = std::atan2(
				forwardVelocity,
				sideVelocity
			);
			float slipAngle = abs(k - HALF_PI);

			// Calculate grip factor of this wheel.
			Scalar grip = 1.0_simd;

			// Less grip if wheel is less aligned to contact plane.
			grip *= abs(dot3(axisW, wheel->contactNormal));

			// Less grip from fudge.
			grip *= Scalar(wheel->contactFudge);

			// Calculate amount of force from slip angle. \tbd Should use curves.
			const float peakSlipFriction = data->getSlipCornerForce();
			const float maxSlipAngle = data->getPeakSlipAngle();

			float force = 0.0f;
			if (slipAngle < maxSlipAngle)
			{
				force = (slipAngle / maxSlipAngle) * peakSlipFriction;
			}
			else
			{
				const float c_fallOff = 2.0f;
				float f = clamp(rad2deg(slipAngle - maxSlipAngle) / c_fallOff, 0.0f, 1.0f);
				force = peakSlipFriction * f;
			}

			// Apply friction force.
			m_body->addForceAt(
				wheel->contactPosition,
				directionPerpW * Scalar(force * sign(-sideVelocity)) * grip,
				false
			);

			// Accumulate rolling friction, applied at center of mass for simplicity.
			rollingFriction += forwardVelocity * Scalar(data->getRollingFriction()) * grip;
		}
		else if (abs(sideVelocity) > FUZZY_EPSILON)
		{
			// All movement lateral; complete static friction.
			const float peakSlipFriction = data->getSlipCornerForce();
			m_body->addForceAt(
				wheel->contactPosition,
				directionPerpW * Scalar(peakSlipFriction * sign(-sideVelocity)),
				false
			);
		}
	}

	if (abs(rollingFriction) > FUZZY_EPSILON)
	{
		m_body->addForceAt(
			Vector4::origo(),
			Vector4(0.0f, 0.0f, -rollingFriction, 0.0f),
			true
		);
	}

	// Help keep vehicle stationary if almost standstill.
	if (!m_airBorn)
	{
		Vector4 linearVelocityW = m_body->getLinearVelocity();
		Scalar forwardVelocityW = dot3(bodyT.axisZ(), linearVelocityW);

		bool throttleIdle = abs(m_engineThrottle) < c_throttleThreshold;
		bool almostStill = (bool)(abs(forwardVelocityW) < c_linearVelocityThreshold);

		if (throttleIdle && almostStill)
		{
			Scalar s = 1.0_simd - abs(forwardVelocityW) / c_linearVelocityThreshold;
			m_body->addLinearImpulse(
				-bodyT.axisZ() * forwardVelocityW * s * s * 0.3_simd * m_totalMass,
				false
			);
		}
	}

	// Apply some rotational damping to prevent oscillation from suspension forces.
	if (!m_airBorn && abs(m_steerAngle) < FUZZY_EPSILON)
	{
		const Scalar damping = 0.45_simd;
		Scalar headVelocity = dot3(m_body->getAngularVelocity(), bodyT.axisY());
		Matrix33 inertiaInv = m_body->getInertiaTensorInverseWorld();
		Vector4 C = Vector4(0.0f, -headVelocity * damping, 0.0f);
		m_body->addAngularImpulse(inertiaInv.inverse() * (bodyT * C), false);
	}
}

void VehicleComponent::updateEngine(float /*dT*/)
{
	Transform bodyT = m_body->getTransform();
	Transform bodyTinv = bodyT.inverse();

	Scalar forwardVelocity = dot3(m_body->getLinearVelocity(), bodyT.axisZ());
	Scalar engineForce = Scalar(m_engineThrottle * m_data->getEngineForce()) * (1.0_simd - clamp(abs(forwardVelocity) / Scalar(m_data->getMaxVelocity()), 0.0_simd, 1.0_simd));

	for (auto wheel : m_wheels)
	{
		if (!wheel->contact)
			continue;

		const WheelData* data = wheel->data;
		T_ASSERT(data != nullptr);

		if (!data->getDrive())
			continue;

		Vector4 direction = wheel->direction * Vector4(1.0f, 0.0f, 1.0f, 0.0f);
		direction.normalize();

		Scalar grip = clamp(wheel->contactNormal.y(), 0.0_simd, 1.0_simd) * Scalar(wheel->contactFudge);

		m_body->addForceAt(
			bodyTinv * wheel->contactPosition,
			direction * engineForce * grip,
			true
		);
	}
}

void VehicleComponent::updateWheels(float dT)
{
	Transform bodyT = m_body->getTransform();

	for (auto wheel : m_wheels)
	{
		float targetVelocity = 0.0f;

		const WheelData* data = wheel->data;
		T_ASSERT(data != nullptr);

		if (!data->getDrive() || abs(m_engineThrottle) <= FUZZY_EPSILON)
			targetVelocity = wheel->velocity * 0.95f;
		else
			targetVelocity = (m_engineThrottle * m_data->getMaxVelocity()) / data->getRadius();

		if (wheel->contact)
		{
			float d = dot3(wheel->contactVelocity, bodyT * wheel->direction);
			wheel->velocity = lerp(d / data->getRadius(), targetVelocity, 0.25f);
		}
		else
			wheel->velocity = targetVelocity;

		wheel->angle += wheel->velocity * dT;
	}
}

	}
}
