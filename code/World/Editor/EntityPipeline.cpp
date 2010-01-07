#include "World/Editor/EntityPipeline.h"
#include "World/Entity/EntityInstance.h"
#include "World/Entity/ExternalEntityData.h"
#include "World/Entity/ExternalSpatialEntityData.h"
#include "World/Entity/GroupEntityData.h"
#include "World/Entity/SpatialGroupEntityData.h"
#include "Editor/IPipelineDepends.h"
#include "Editor/IPipelineBuilder.h"
#include "Database/Instance.h"

namespace traktor
{
	namespace world
	{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.world.EntityPipeline", 1, EntityPipeline, editor::IPipeline)

bool EntityPipeline::create(const editor::IPipelineSettings* settings)
{
	return true;
}

void EntityPipeline::destroy()
{
}

TypeInfoSet EntityPipeline::getAssetTypes() const
{
	TypeInfoSet typeSet;
	typeSet.insert(&type_of< EntityInstance >());
	typeSet.insert(&type_of< EntityData >());
	return typeSet;
}

bool EntityPipeline::buildDependencies(
	editor::IPipelineDepends* pipelineDepends,
	const db::Instance* sourceInstance,
	const ISerializable* sourceAsset,
	Ref< const Object >& outBuildParams
) const
{
	if (const EntityInstance* instance = dynamic_type_cast< const EntityInstance* >(sourceAsset))
		pipelineDepends->addDependency(instance->getEntityData());
	else if (const ExternalEntityData* externalEntityData = dynamic_type_cast< const ExternalEntityData* >(sourceAsset))
		pipelineDepends->addDependency(externalEntityData->getGuid(), editor::PdfBuild);
	else if (const ExternalSpatialEntityData* externalSpatialEntityData = dynamic_type_cast< const ExternalSpatialEntityData* >(sourceAsset))
		pipelineDepends->addDependency(externalSpatialEntityData->getGuid(), editor::PdfBuild);
	else if (const GroupEntityData* groupEntityData = dynamic_type_cast< const GroupEntityData* >(sourceAsset))
	{
		const RefArray< EntityInstance >& instances = groupEntityData->getInstances();
		for (RefArray< EntityInstance >::const_iterator i = instances.begin(); i != instances.end(); ++i)
			pipelineDepends->addDependency(*i);
	}
	else if (const SpatialGroupEntityData* spatialGroupEntityData = dynamic_type_cast< const SpatialGroupEntityData* >(sourceAsset))
	{
		const RefArray< EntityInstance >& instances = spatialGroupEntityData->getInstances();
		for (RefArray< EntityInstance >::const_iterator i = instances.begin(); i != instances.end(); ++i)
			pipelineDepends->addDependency(*i);
	}
	return true;
}

bool EntityPipeline::buildOutput(
	editor::IPipelineBuilder* pipelineBuilder,
	const ISerializable* sourceAsset,
	uint32_t sourceAssetHash,
	const Object* buildParams,
	const std::wstring& outputPath,
	const Guid& outputGuid,
	uint32_t reason
) const
{
	if ((reason & (editor::PbrSourceModified | editor::PbrForced)) == 0)
		return true;

	Ref< db::Instance > outputInstance = pipelineBuilder->createOutputInstance(outputPath, outputGuid);
	if (!outputInstance)
		return false;

	outputInstance->setObject(sourceAsset);

	if (!outputInstance->commit())
		return false;

	return true;
}

	}
}
