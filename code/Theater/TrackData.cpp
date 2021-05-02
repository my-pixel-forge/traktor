#include "Core/Serialization/AttributePrivate.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/MemberComposite.h"
#include "Core/Serialization/MemberRef.h"
#include "Theater/TrackData.h"
#include "World/EntityData.h"

namespace traktor
{
	namespace theater
	{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.theater.TrackData", 7, TrackData, ISerializable)

void TrackData::setEntityId(const Guid& entityId)
{
	m_entityId = entityId;
}

const Guid& TrackData::getEntityId() const
{
	return m_entityId;
}

void TrackData::setLookAtEntityId(const Guid& entityId)
{
	m_lookAtEntityId = entityId;
}

const Guid& TrackData::getLookAtEntityId() const
{
	return m_lookAtEntityId;
}

void TrackData::setPath(const TransformPath& path)
{
	m_path = path;
}

const TransformPath& TrackData::getPath() const
{
	return m_path;
}

TransformPath& TrackData::getPath()
{
	return m_path;
}

void TrackData::serialize(ISerializer& s)
{
	T_FATAL_ASSERT(s.getVersion< TrackData >() >= 6);

	s >> Member< Guid >(L"entityId", m_entityId, AttributePrivate());
	s >> Member< Guid >(L"lookAtEntityId", m_lookAtEntityId, AttributePrivate());
	s >> MemberComposite< TransformPath >(L"path", m_path);

	if (s.getVersion< TrackData >() < 7)
	{
		float dummy;
		s >> Member< float >(L"loopStart", dummy);
		s >> Member< float >(L"loopEnd", dummy);
		s >> Member< float >(L"timeOffset", dummy);
		s >> Member< float >(L"wobbleMagnitude", dummy);
		s >> Member< float >(L"wobbleRate", dummy);
	}
}

	}
}
