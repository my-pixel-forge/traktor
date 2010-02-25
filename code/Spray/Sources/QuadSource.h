#ifndef traktor_spray_QuadSource_H
#define traktor_spray_QuadSource_H

#include "Core/Math/RandomGeometry.h"
#include "Core/Math/Range.h"
#include "Spray/Source.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_SPRAY_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace spray
	{

/*! \brief Quad particle source.
 * \ingroup Spray
 */
class T_DLLCLASS QuadSource : public Source
{
	T_RTTI_CLASS;

public:
	QuadSource();

	virtual bool bind(resource::IResourceManager* resourceManager);

	virtual void emit(
		EmitterUpdateContext& context,
		const Transform& transform,
		uint32_t emitCount,
		EmitterInstance& emitterInstance
	) const;

	virtual bool serialize(ISerializer& s);

	inline const Vector4& getCenter() const { return m_center; }

	inline const Vector4& getAxis1() const { return m_axis1; }

	inline const Vector4& getAxis2() const { return m_axis2; }

	inline const Vector4& getNormal() const { return m_normal; }

private:
	Vector4 m_center;
	Vector4 m_axis1;
	Vector4 m_axis2;
	Vector4 m_normal;
	Range< float > m_velocity;
	Range< float > m_orientation;
	Range< float > m_angularVelocity;
	Range< float > m_age;
	Range< float > m_mass;
	Range< float > m_size;
};

	}
}

#endif	// traktor_spray_QuadSource_H
