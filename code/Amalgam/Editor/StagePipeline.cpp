#include "Amalgam/Editor/StagePipeline.h"
#include "Amalgam/Engine/AudioLayerData.h"
#include "Amalgam/Engine/StageData.h"
#include "Amalgam/Engine/FlashLayerData.h"
#include "Amalgam/Engine/VideoLayerData.h"
#include "Amalgam/Engine/WorldLayerData.h"
#include "Editor/IPipelineDepends.h"

namespace traktor
{
	namespace amalgam
	{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.amalgam.StagePipeline", 3, StagePipeline, editor::DefaultPipeline)

TypeInfoSet StagePipeline::getAssetTypes() const
{
	TypeInfoSet typeSet;
	typeSet.insert(&type_of< StageData >());
	return typeSet;
}

bool StagePipeline::buildDependencies(
	editor::IPipelineDepends* pipelineDepends,
	const db::Instance* sourceInstance,
	const ISerializable* sourceAsset,
	const std::wstring& outputPath,
	const Guid& outputGuid
) const
{
	const StageData* stageData = checked_type_cast< const StageData*, false >(sourceAsset);

	pipelineDepends->addDependency(stageData->m_script, editor::PdfBuild);

	for (std::map< std::wstring, Guid >::const_iterator i = stageData->m_transitions.begin(); i != stageData->m_transitions.end(); ++i)
		pipelineDepends->addDependency(i->second, editor::PdfBuild);

	for (RefArray< LayerData >::const_iterator i = stageData->m_layers.begin(); i != stageData->m_layers.end(); ++i)
	{
		if (const AudioLayerData* audioLayer = dynamic_type_cast< const AudioLayerData* >(*i))
			pipelineDepends->addDependency(audioLayer->m_sound, editor::PdfBuild);
		else if (const FlashLayerData* flashLayer = dynamic_type_cast< const FlashLayerData* >(*i))
		{
			pipelineDepends->addDependency(flashLayer->m_movie, editor::PdfBuild);
			for (std::map< std::wstring, resource::Id< flash::FlashMovie > >::const_iterator i = flashLayer->m_externalMovies.begin(); i != flashLayer->m_externalMovies.end(); ++i)
				pipelineDepends->addDependency(i->second, editor::PdfBuild);
			pipelineDepends->addDependency(flashLayer->m_postProcess, editor::PdfBuild);
		}
		else if (const VideoLayerData* videoLayer = dynamic_type_cast< const VideoLayerData* >(*i))
		{
			pipelineDepends->addDependency(videoLayer->m_video, editor::PdfBuild);
			pipelineDepends->addDependency(videoLayer->m_shader, editor::PdfBuild | editor::PdfResource);
		}
		else if (const WorldLayerData* worldLayer = dynamic_type_cast< const WorldLayerData* >(*i))
		{
			pipelineDepends->addDependency(worldLayer->m_scene, editor::PdfBuild);
			for (std::map< std::wstring, resource::Id< world::EntityData > >::const_iterator j = worldLayer->m_entities.begin(); j != worldLayer->m_entities.end(); ++j)
				pipelineDepends->addDependency(j->second, editor::PdfBuild);
		}
	}

	pipelineDepends->addDependency(stageData->m_resourceBundle, editor::PdfBuild);
	pipelineDepends->addDependency(stageData->m_localizationDictionary, editor::PdfBuild);

	return true;
}

	}
}
