#include "Core/Math/Const.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Core/Serialization/MemberRefArray.h"
#include "Theater/Act.h"
#include "Theater/ActData.h"
#include "Theater/Track.h"
#include "Theater/TrackData.h"
#include "World/Entity.h"

namespace traktor
{
	namespace theater
	{
		namespace
		{

world::Entity* findEntityDataProduct(const SmallMap< Guid, Ref< world::Entity > >& entityProducts, const Guid& entityId)
{
	auto it = entityProducts.find(entityId);
	return it != entityProducts.end() ? it->second : nullptr;
}

		}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.theater.ActData", 0, ActData, ISerializable)

Ref< Act > ActData::createInstance(const SmallMap< Guid, Ref< world::Entity > >& entityProducts) const
{
	bool infinite = false;

	RefArray< const Track > tracks(m_tracks.size());
	for (size_t i = 0; i < m_tracks.size(); ++i)
	{
		Ref< world::Entity > entity = findEntityDataProduct(entityProducts, m_tracks[i]->getEntityId());
		if (!entity)
			return nullptr;

		Ref< world::Entity > lookAtEntity = findEntityDataProduct(entityProducts, m_tracks[i]->getLookAtEntityId());

		tracks[i] = new Track(
			entity,
			lookAtEntity,
			m_tracks[i]->getPath(),
			m_tracks[i]->getLoopStart(),
			m_tracks[i]->getLoopEnd(),
			m_tracks[i]->getTimeOffset(),
			m_tracks[i]->getWobbleMagnitude(),
			m_tracks[i]->getWobbleRate()
		);

		if (m_tracks[i]->getLoopStart() < m_tracks[i]->getLoopEnd() - FUZZY_EPSILON)
			infinite |= true;
	}

	return new Act(m_duration, infinite, tracks);
}

void ActData::serialize(ISerializer& s)
{
	s >> Member< std::wstring >(L"name", m_name);
	s >> Member< float >(L"duration", m_duration);
	s >> MemberRefArray< TrackData >(L"tracks", m_tracks);
}

	}
}
