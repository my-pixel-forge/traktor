#include "World/Entity/EntityInstance.h"
#include "World/Entity/EntityData.h"
#include "Core/Serialization/Serializer.h"
#include "Core/Serialization/MemberRef.h"

namespace traktor
{
	namespace world
	{

T_IMPLEMENT_RTTI_SERIALIZABLE_CLASS(L"traktor.world.EntityInstance", EntityInstance, Serializable)

EntityInstance::EntityInstance()
{
}

EntityInstance::EntityInstance(const std::wstring& name, EntityData* entityData)
:	m_name(name)
,	m_entityData(entityData)
{
}

void EntityInstance::setName(const std::wstring& name)
{
	m_name = name;
}

const std::wstring& EntityInstance::getName() const
{
	return m_name;
}

EntityData* EntityInstance::getEntityData() const
{
	return m_entityData;
}

void EntityInstance::addReference(EntityInstance* reference)
{
	m_references.push_back(reference);
}

const RefArray< EntityInstance >& EntityInstance::getReferences() const
{
	return m_references;
}

bool EntityInstance::serialize(Serializer& s)
{
	s >> Member< std::wstring >(L"name", m_name);
	s >> MemberRef< EntityData >(L"entityData", m_entityData);
	s >> MemberRefArray< EntityInstance >(L"references", m_references);
	return true;
}

	}
}
