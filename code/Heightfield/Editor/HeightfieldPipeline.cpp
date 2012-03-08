#include <limits>
#include "Core/Io/FileSystem.h"
#include "Core/Io/IStream.h"
#include "Core/Io/Writer.h"
#include "Core/Log/Log.h"
#include "Core/Settings/PropertyString.h"
#include "Database/Instance.h"
#include "Drawing/Image.h"
#include "Editor/IPipelineBuilder.h"
#include "Editor/IPipelineDepends.h"
#include "Editor/IPipelineSettings.h"
#include "Heightfield/Heightfield.h"
#include "Heightfield/HeightfieldResource.h"
#include "Heightfield/Editor/HeightfieldAsset.h"
#include "Heightfield/Editor/HeightfieldCompositor.h"
#include "Heightfield/Editor/HeightfieldLayer.h"
#include "Heightfield/Editor/HeightfieldPipeline.h"

namespace traktor
{
	namespace hf
	{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.hf.HeightfieldPipeline", 0, HeightfieldPipeline, editor::IPipeline)

bool HeightfieldPipeline::create(const editor::IPipelineSettings* settings)
{
	m_assetPath = settings->getProperty< PropertyString >(L"Pipeline.AssetPath", L"");
	return true;
}

void HeightfieldPipeline::destroy()
{
}

TypeInfoSet HeightfieldPipeline::getAssetTypes() const
{
	TypeInfoSet typeSet;
	typeSet.insert(&type_of< HeightfieldAsset >());
	return typeSet;
}

bool HeightfieldPipeline::buildDependencies(
	editor::IPipelineDepends* pipelineDepends,
	const db::Instance* sourceInstance,
	const ISerializable* sourceAsset,
	Ref< const Object >& outBuildParams
) const
{
	Ref< const HeightfieldAsset > heightfieldAsset = checked_type_cast< const HeightfieldAsset* >(sourceAsset);
	
	// Add dependency to file.
	Path fileName = FileSystem::getInstance().getAbsolutePath(m_assetPath, heightfieldAsset->getFileName());
	pipelineDepends->addDependency(fileName);

	// Pass source instance as parameter to build output.
	outBuildParams = sourceInstance;
	return true;
}

bool HeightfieldPipeline::buildOutput(
	editor::IPipelineBuilder* pipelineBuilder,
	const ISerializable* sourceAsset,
	uint32_t sourceAssetHash,
	const Object* buildParams,
	const std::wstring& outputPath,
	const Guid& outputGuid,
	uint32_t reason
) const
{
	Ref< const HeightfieldAsset > heightfieldAsset = checked_type_cast< const HeightfieldAsset* >(sourceAsset);

	Ref< const db::Instance > assetInstance = checked_type_cast< const db::Instance*, true >(buildParams);
	if (!assetInstance)
	{
		log::error << L"Failed to build heightfield; no asset instance" << Endl;
		return false;
	}

	Ref< HeightfieldCompositor > compositor = HeightfieldCompositor::createFromAsset(heightfieldAsset, m_assetPath);
	if (!compositor)
	{
		log::error << L"Failed to build heightfield; unable to create heightfield compositor from asset" << Endl;
		return false;
	}

	if (!compositor->readInstanceData(assetInstance))
	{
		log::error << L"Failed to build heightfield; unable to read asset layers" << Endl;
		return false;
	}

	const HeightfieldLayer* mergedLayer = compositor->getMergedLayer();
	if (!mergedLayer)
	{
		log::error << L"Failed to build heightfield; unable to get merged layers" << Endl;
		return false;
	}

	// Create height field resource.
	Ref< HeightfieldResource > resource = new HeightfieldResource();

	// Create instance's name.
	Ref< db::Instance > instance = pipelineBuilder->createOutputInstance(
		outputPath,
		outputGuid
	);
	if (!instance)
	{
		log::error << L"Failed to build heightfield; unable to create instance" << Endl;
		return false;
	}

	Ref< IStream > stream = instance->writeData(L"Data");
	if (!stream)
	{
		log::error << L"Failed to build heightfield; unable to create data stream" << Endl;
		instance->revert();
		return false;
	}

	uint32_t size = compositor->getSize();
	const height_t* heights = static_cast< const height_t* >(mergedLayer->getImage()->getData());

	Writer(stream).write(
		heights,
		size * size,
		sizeof(height_t)
	);

	stream->close();
	
	resource->m_size = size;
	resource->m_worldExtent = heightfieldAsset->getWorldExtent();
	resource->m_patchDim = heightfieldAsset->getPatchDim();
	resource->m_detailSkip = heightfieldAsset->getDetailSkip();

	instance->setObject(resource);

	return instance->commit();
}

Ref< ISerializable > HeightfieldPipeline::buildOutput(
	editor::IPipelineBuilder* pipelineBuilder,
	const ISerializable* sourceAsset
) const
{
	T_FATAL_ERROR;
	return 0;
}

	}
}
