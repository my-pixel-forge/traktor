#include "Spray/Modifiers/PlaneCollisionModifier.h"

namespace traktor
{
	namespace spray
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.spray.PlaneCollisionModifier", PlaneCollisionModifier, Modifier)

PlaneCollisionModifier::PlaneCollisionModifier(const Plane& plane, float radius, float restitution)
:	m_plane(plane)
,	m_radius(radius)
,	m_restitution(restitution)
{
}

void PlaneCollisionModifier::update(const Scalar& deltaTime, const Transform& transform, PointVector& points, size_t first, size_t last) const
{
	Plane planeW = transform.toMatrix44() * m_plane;
	Vector4 center = transform.translation();

	for (size_t i = first; i < last; ++i)
	{
		Scalar rv = dot3(planeW.normal(), points[i].velocity);
		if (rv >= 0.0f)
			continue;

		Scalar rd = planeW.distance(points[i].position);
		if (rd >= points[i].size)
			continue;

		if ((points[i].position - center).length2() >= m_radius)
			continue;

		points[i].velocity = -reflect(points[i].velocity, m_plane.normal()) * m_restitution;
	}
}

	}
}
