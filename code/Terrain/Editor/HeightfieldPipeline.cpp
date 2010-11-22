#include <limits>
#include "Core/Io/FileSystem.h"
#include "Core/Io/Writer.h"
#include "Core/Log/Log.h"
#include "Core/Math/Log2.h"
#include "Core/Settings/PropertyString.h"
#include "Database/Instance.h"
#include "Drawing/Image.h"
#include "Drawing/PixelFormat.h"
#include "Drawing/Filters/ScaleFilter.h"
#include "Drawing/Filters/ConvolutionFilter.h"
#include "Editor/IPipelineBuilder.h"
#include "Editor/IPipelineDepends.h"
#include "Editor/IPipelineSettings.h"
#include "Terrain/Heightfield.h"
#include "Terrain/HeightfieldResource.h"
#include "Terrain/Editor/HeightfieldAsset.h"
#include "Terrain/Editor/HeightfieldPipeline.h"

namespace traktor
{
	namespace terrain
	{
		namespace
		{

drawing::PixelFormat s_pfRaw16(16, 16, 0, 0, 0, 0, 0, 0, 0, false, false);

Ref< drawing::Image > readRawTerrain(const Path& fileName)
{
	Ref< IStream > file = FileSystem::getInstance().open(fileName, File::FmRead);
	if (!file)
		return 0;

	uint32_t fileSize = file->available();

	const uint32_t heightByteSize = 2;

	uint32_t heights = fileSize / heightByteSize;
	uint32_t size = uint32_t(std::sqrt(float(heights)));

	Ref< drawing::Image > image = new drawing::Image(
		s_pfRaw16,
		size,
		size
	);

	file->read(image->getData(), fileSize);
	file->close();

	return image;
}

		}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.terrain.HeightfieldPipeline", 1, HeightfieldPipeline, editor::IPipeline)

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
	Path fileName = FileSystem::getInstance().getAbsolutePath(m_assetPath, heightfieldAsset->getFileName());
	pipelineDepends->addDependency(fileName);
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
	Path fileName = FileSystem::getInstance().getAbsolutePath(m_assetPath, heightfieldAsset->getFileName());

	Ref< drawing::Image > image;
	if (fileName.getExtension() == L"raw")
	{
		// Load 16-raw file.
		image = readRawTerrain(fileName);
	}
	else
	{
		// Load source image.
		image = drawing::Image::load(fileName);
		image->convert(s_pfRaw16);

		// Smooth image, hack to filter 8-bit heights into 16-bit.
		image = image->applyFilter(drawing::ConvolutionFilter::createGaussianBlur());
	}
	if (!image)
		return false;

	// Rescale image to fit 2^x.
	uint32_t size = std::max< uint32_t >(image->getWidth(), image->getHeight());
	size = ((size / heightfieldAsset->m_detailSkip + heightfieldAsset->m_patchDim - 1) / heightfieldAsset->m_patchDim) * heightfieldAsset->m_patchDim * heightfieldAsset->m_detailSkip;
	if (size < heightfieldAsset->m_patchDim || (size / heightfieldAsset->m_detailSkip) % heightfieldAsset->m_patchDim != 0)
	{
		log::error << L"Invalid patch dimension or detail skip value in heightfield asset" << Endl;
		return false;
	}

	drawing::ScaleFilter scaleFilter(
		size,
		size,
		drawing::ScaleFilter::MnAverage,
		drawing::ScaleFilter::MgLinear
	);
	image = image->applyFilter(&scaleFilter);
	T_ASSERT (image);

	// Create height field resource.
	Ref< HeightfieldResource > resource = new HeightfieldResource();

	// Create instance's name.
	Ref< db::Instance > instance = pipelineBuilder->createOutputInstance(
		outputPath,
		outputGuid
	);
	if (!instance)
	{
		log::error << L"Failed to build heightfield, unable to create instance" << Endl;
		return false;
	}

	Ref< IStream > stream = instance->writeData(L"Data");
	if (!stream)
	{
		log::error << L"Failed to build heightfield, unable to create data stream" << Endl;
		instance->revert();
		return false;
	}

	// Convert image pixels into heights.
	Writer writer(stream);
	for (uint32_t y = 0; y < size; ++y)
	{
		for (uint32_t x = 0; x < size; ++x)
		{
			Color4f imagePixel;
			image->getPixel(x, y, imagePixel);

			Heightfield::height_t height = Heightfield::height_t(imagePixel.getRed() * std::numeric_limits< Heightfield::height_t >::max());
			writer << height;
		}
	}

	stream->close();
	
	resource->m_size = size;
	resource->m_worldExtent = heightfieldAsset->m_worldExtent;
	resource->m_patchDim = heightfieldAsset->m_patchDim;
	resource->m_detailSkip = heightfieldAsset->m_detailSkip;

	instance->setObject(resource);

	return instance->commit();
}

	}
}
