#include "Core/Serialization/AttributePrivate.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Illuminate/Editor/IlluminateConfiguration.h"

namespace traktor
{
	namespace illuminate
	{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"traktor.illuminate.IlluminateConfiguration", 4, IlluminateConfiguration, ISerializable)

IlluminateConfiguration::IlluminateConfiguration()
:	m_seedGuid(Guid::create())
,	m_traceDirect(false)
,	m_traceIndirect(true)
,	m_indirectSampleCount(1000)
,	m_shadowSampleCount(100)
,	m_pointLightShadowRadius(1.5f)
,	m_lumelDensity(4.0f)
,	m_enableAutoTexCoords(true)
,	m_enableShadowFix(true)
,	m_enableDilate(true)
,	m_enableDenoise(false)
{
}

void IlluminateConfiguration::serialize(ISerializer& s)
{
	s >> Member< Guid >(L"seedGuid", m_seedGuid, AttributePrivate());
	s >> Member< bool >(L"traceDirect", m_traceDirect);
	s >> Member< bool >(L"traceIndirect", m_traceIndirect);
	s >> Member< uint32_t >(L"indirectSampleCount", m_indirectSampleCount);
	s >> Member< uint32_t >(L"shadowSampleCount", m_shadowSampleCount);
	s >> Member< float >(L"pointLightShadowRadius", m_pointLightShadowRadius);
	s >> Member< float >(L"lumelDensity", m_lumelDensity);

	if (s.getVersion() >= 2)
		s >> Member< bool >(L"enableAutoTexCoords", m_enableAutoTexCoords);

	if (s.getVersion() >= 1)
		s >> Member< bool >(L"enableShadowFix", m_enableShadowFix);

	if (s.getVersion() >= 3)
		s >> Member< bool >(L"enableDilate", m_enableDilate);

	if (s.getVersion() >= 4)
		s >> Member< bool >(L"enableDenoise", m_enableDenoise);
}

	}
}