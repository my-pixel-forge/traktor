#include "Physics/Editor/RigidEntityPipeline.h"
#include "Physics/World/RigidEntityData.h"
#include "Physics/BodyDesc.h"
#include "Physics/MeshShapeDesc.h"
#include "World/Entity/EntityInstance.h"
#include "Editor/IPipelineDepends.h"

namespace traktor
{
	namespace physics
	{

T_IMPLEMENT_RTTI_SERIALIZABLE_CLASS(L"traktor.physics.RigidEntityPipeline", RigidEntityPipeline, world::EntityPipeline)

TypeSet RigidEntityPipeline::getAssetTypes() const
{
	TypeSet typeSet;
	typeSet.insert(&type_of< RigidEntityData >());
	return typeSet;
}

bool RigidEntityPipeline::buildDependencies(
	editor::IPipelineDepends* pipelineDepends,
	const db::Instance* sourceInstance,
	const Serializable* sourceAsset,
	Ref< const Object >& outBuildParams
) const
{
	if (const RigidEntityData* rigidEntityData = dynamic_type_cast< const RigidEntityData* >(sourceAsset))
	{
		pipelineDepends->addDependency(rigidEntityData->getBodyDesc());

		const world::EntityInstance* instance = rigidEntityData->getInstance();
		if (instance)
			pipelineDepends->addDependency(instance);
	}
	return true;
}

	}
}
