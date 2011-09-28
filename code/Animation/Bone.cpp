#include "Animation/Bone.h"
#include "Core/Serialization/AttributePoint.h"
#include "Core/Serialization/AttributeRange.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Core/Math/Const.h"

namespace traktor
{
	namespace animation
	{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.animation.Bone", 1, Bone, ISerializable)

Bone::Bone()
:	m_parent(-1)
,	m_position(0.0f, 0.0f, 0.0f, 1.0f)
,	m_orientation(Quaternion::identity())
,	m_length(1.0f)
,	m_radius(0.0f)
,	m_enableLimits(false)
,	m_twistLimit(0.0f)
,	m_coneLimit(0.0f, 0.0f)
{
}

bool Bone::serialize(ISerializer& s)
{
	s >> Member< std::wstring >(L"name", m_name);
	s >> Member< int32_t >(L"parent", m_parent);
	s >> Member< Vector4 >(L"position", m_position, AttributePoint());
	s >> Member< Quaternion >(L"orientation", m_orientation);
	s >> Member< Scalar >(L"length", m_length, AttributeRange(0.0f));
	s >> Member< Scalar >(L"radius", m_radius, AttributeRange(0.0f));
	
	if (s.getVersion() >= 1)
	{
		s >> Member< bool >(L"enableLimits", m_enableLimits);
		s >> Member< float >(L"twistLimit", m_twistLimit, AttributeRange(-PI, PI));
		s >> Member< Vector2 >(L"coneLimit", m_coneLimit);
	}

	return true;
}

	}
}
