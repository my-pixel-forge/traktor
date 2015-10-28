#include "Core/Class/IRuntimeClass.h"
#include "Core/Serialization/ISerializer.h"
#include "Resource/IResourceManager.h"
#include "Resource/Member.h"
#include "World/ScriptComponent.h"
#include "World/ScriptComponentData.h"

namespace traktor
{
	namespace world
	{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"traktor.world.ScriptComponentData", 0, ScriptComponentData, IEntityComponentData)

Ref< IEntityComponent > ScriptComponentData::createInstance(Entity* owner, resource::IResourceManager* resourceManager) const
{
	resource::Proxy< IRuntimeClass > clazz;
	if (!resourceManager->bind(m_class, clazz))
		return 0;

	return new ScriptComponent(owner, clazz);
}

void ScriptComponentData::serialize(ISerializer& s)
{
	s >> resource::Member< IRuntimeClass >(L"class", m_class);
}

	}
}
