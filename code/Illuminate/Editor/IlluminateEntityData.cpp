#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/MemberRefArray.h"
#include "Illuminate/Editor/IlluminateEntityData.h"
#include "World/EntityData.h"

namespace traktor
{
	namespace illuminate
	{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"traktor.illuminate.IlluminateEntityData", 0, IlluminateEntityData, world::EntityData)

IlluminateEntityData::IlluminateEntityData()
:	m_seedGuid(Guid::create())
,	m_lumelsPerUnit(10.0f)
,	m_directLighting(true)
,	m_indirectLighting(true)
,	m_indirectTraceSamples(64)
,	m_highDynamicRange(false)
,	m_compressLightMap(true)
{
}

void IlluminateEntityData::addEntityData(world::EntityData* entityData)
{
	T_ASSERT (std::find(m_entityData.begin(), m_entityData.end(), entityData) == m_entityData.end());
	m_entityData.push_back(entityData);
}

void IlluminateEntityData::removeEntityData(world::EntityData* entityData)
{
	RefArray< world::EntityData >::iterator i = std::find(m_entityData.begin(), m_entityData.end(), entityData);
	if (i != m_entityData.end())
		m_entityData.erase(i);
}

void IlluminateEntityData::removeAllEntityData()
{
	m_entityData.resize(0);
}

void IlluminateEntityData::setTransform(const Transform& transform)
{
	Transform deltaTransform = transform * getTransform().inverse();
	for (RefArray< world::EntityData >::iterator i = m_entityData.begin(); i != m_entityData.end(); ++i)
	{
		Transform currentTransform = (*i)->getTransform();
		(*i)->setTransform(deltaTransform * currentTransform);
	}
	world::EntityData::setTransform(transform);
}

void IlluminateEntityData::serialize(ISerializer& s)
{
	world::EntityData::serialize(s);
	
	s >> Member< Guid >(L"seedGuid", m_seedGuid);
	s >> Member< float >(L"lumelsPerUnit", m_lumelsPerUnit);
	s >> Member< bool >(L"directLighting", m_directLighting);
	s >> Member< bool >(L"indirectLighting", m_indirectLighting);
	s >> Member< int32_t >(L"indirectTraceSamples", m_indirectTraceSamples);
	s >> Member< bool >(L"highDynamicRange", m_highDynamicRange);
	s >> Member< bool >(L"compressLightMap", m_compressLightMap);
	s >> MemberRefArray< world::EntityData >(L"entityData", m_entityData);
}

	}
}
