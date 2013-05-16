#include "Core/Serialization/DeepClone.h"
#include "Editor/IPipelineBuilder.h"
#include "Theater/ActData.h"
#include "Theater/TheaterControllerData.h"
#include "Theater/TrackData.h"
#include "Theater/Editor/TheaterControllerPipeline.h"
#include "World/EntityData.h"

namespace traktor
{
	namespace theater
	{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.theater.TheaterControllerPipeline", 0, TheaterControllerPipeline, editor::IPipeline)

bool TheaterControllerPipeline::create(const editor::IPipelineSettings* settings)
{
	return true;
}

void TheaterControllerPipeline::destroy()
{
}

TypeInfoSet TheaterControllerPipeline::getAssetTypes() const
{
	TypeInfoSet typeSet;
	typeSet.insert(&type_of< TheaterControllerData >());
	return typeSet;
}

bool TheaterControllerPipeline::buildDependencies(
	editor::IPipelineDepends* pipelineDepends,
	const db::Instance* sourceInstance,
	const ISerializable* sourceAsset,
	const std::wstring& outputPath,
	const Guid& outputGuid
) const
{
	return true;
}

bool TheaterControllerPipeline::buildOutput(
	editor::IPipelineBuilder* pipelineBuilder,
	const editor::IPipelineDependencySet* dependencySet,
	const editor::PipelineDependency* dependency,
	const db::Instance* sourceInstance,
	const ISerializable* sourceAsset,
	uint32_t sourceAssetHash,
	const std::wstring& outputPath,
	const Guid& outputGuid,
	const Object* buildParams,
	uint32_t reason
) const
{
	return false;
}

Ref< ISerializable > TheaterControllerPipeline::buildOutput(
	editor::IPipelineBuilder* pipelineBuilder,
	const ISerializable* sourceAsset
) const
{
	const TheaterControllerData* sourceControllerData = checked_type_cast< const TheaterControllerData*, false >(sourceAsset);
	
	Ref< TheaterControllerData > controllerData = new TheaterControllerData(*sourceControllerData);
	if (!controllerData)
		return 0;

	RefArray< ActData >& acts = controllerData->getActs();
	for (uint32_t i = 0; i < acts.size(); ++i)
	{
		RefArray< TrackData >& tracks = acts[i]->getTracks();
		for (uint32_t j = 0; j < tracks.size(); ++j)
		{
			Ref< world::EntityData > entityData = checked_type_cast< world::EntityData* >(pipelineBuilder->buildOutput(tracks[j]->getEntityData()));
			Ref< world::EntityData > lookAtEntityData = checked_type_cast< world::EntityData* >(pipelineBuilder->buildOutput(tracks[j]->getLookAtEntityData()));

			tracks[j] = new TrackData(*tracks[j]);
			tracks[j]->setEntityData(entityData);
			tracks[j]->setLookAtEntityData(lookAtEntityData);
		}
	}

	return controllerData;
}

	}
}
