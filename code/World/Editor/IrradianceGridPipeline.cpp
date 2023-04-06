/*
 * TRAKTOR
 * Copyright (c) 2022 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include <functional>
#include "Core/Io/FileSystem.h"
#include "Core/Io/IStream.h"
#include "Core/Io/Writer.h"
#include "Core/Log/Log.h"
#include "Core/Math/Float.h"
#include "Core/Math/Polar.h"
#include "Core/Math/Quasirandom.h"
#include "Core/Math/Random.h"
#include "Core/Misc/SafeDestroy.h"
#include "Core/Misc/String.h"
#include "Core/Serialization/DeepHash.h"
#include "Core/Settings/PropertyString.h"
#include "Database/Instance.h"
#include "Drawing/CubeMap.h"
#include "Drawing/Image.h"
#include "Editor/IPipelineBuilder.h"
#include "Editor/IPipelineSettings.h"
#include "Render/SH/SHEngine.h"
#include "Render/SH/SHFunction.h"
#include "World/Editor/IrradianceGridAsset.h"
#include "World/Editor/IrradianceGridPipeline.h"
#include "World/IrradianceGridResource.h"

namespace traktor
{
	namespace world
	{
		namespace
		{

class WrappedSHFunction : public render::SHFunction
{
public:
	WrappedSHFunction(const std::function< Vector4 (const Vector4&) >& fn)
	:	m_fn(fn)
	{
	}

	virtual Vector4 evaluate(float phi, float theta, const Vector4& unit) const override final
	{
		return m_fn(unit);
	}

private:
	std::function< Vector4 (const Vector4&) > m_fn;
};

		}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.world.IrradianceGridPipeline", 1, IrradianceGridPipeline, editor::IPipeline)

bool IrradianceGridPipeline::create(const editor::IPipelineSettings* settings)
{
	m_assetPath = settings->getPropertyExcludeHash< std::wstring >(L"Pipeline.AssetPath", L"");
	return true;
}

void IrradianceGridPipeline::destroy()
{
}

TypeInfoSet IrradianceGridPipeline::getAssetTypes() const
{
	return makeTypeInfoSet< IrradianceGridAsset >();
}

bool IrradianceGridPipeline::shouldCache() const
{
	return true;
}

uint32_t IrradianceGridPipeline::hashAsset(const ISerializable* sourceAsset) const
{
	return DeepHash(sourceAsset).get();
}

bool IrradianceGridPipeline::buildDependencies(
	editor::IPipelineDepends* pipelineDepends,
	const db::Instance* sourceInstance,
	const ISerializable* sourceAsset,
	const std::wstring& outputPath,
	const Guid& outputGuid
) const
{
	return true;
}

bool IrradianceGridPipeline::buildOutput(
	editor::IPipelineBuilder* pipelineBuilder,
	const editor::PipelineDependencySet* dependencySet,
	const editor::PipelineDependency* dependency,
	const db::Instance* sourceInstance,
	const ISerializable* sourceAsset,
	const std::wstring& outputPath,
	const Guid& outputGuid,
	const Object* buildParams,
	uint32_t reason
) const
{
	const IrradianceGridAsset* asset = mandatory_non_null_type_cast< const IrradianceGridAsset* >(sourceAsset);

	Path filePath = FileSystem::getInstance().getAbsolutePath(Path(m_assetPath) + asset->getFileName());
	Ref< IStream > file = FileSystem::getInstance().open(filePath, File::FmRead);
	if (!file)
	{
		log::error << L"Irradiance grid pipeline failed; unable to open source file \"" << filePath.getPathName() << L"\"." << Endl;
		return false;
	}

	Ref< drawing::Image > skyImage = drawing::Image::load(file, asset->getFileName().getExtension());
	if (!skyImage)
	{
		log::error << L"Irradiance grid pipeline failed; unable to read source image." << Endl;
		return false;
	}

	safeClose(file);

	Ref< drawing::CubeMap > cubeMap = drawing::CubeMap::createFromImage(skyImage);
	if (!cubeMap)
	{
		log::error << L"Irradiance grid pipeline failed; unable to create cube map from image." << Endl;
		return false;
	}

	// Figure out sun direction.
	float sunIntensity = -1.0f;
	Vector4 sunDirection;

	if (asset->shouldCancelSun())
	{
		for (int32_t y = 0; y <= 100; ++y)
		{
			const float phi = PI * ((y / 100.0f) * 2.0f - 1.0f);

			for (int32_t x = 0; x < 100; ++x)
			{
				const float theta = TWO_PI * (x / 100.0f);
				const Vector4 direction = Polar(phi, theta).toUnitCartesian();
				const float intensity = dot3(cubeMap->get(direction), Vector4(1.0f, 1.0f, 1.0f));
				if (intensity > sunIntensity)
				{
					sunIntensity = intensity;
					sunDirection = direction;
				}
			}
		}
	}

	// Convolve sky into irradiance probe.
	const Scalar intensity(asset->getIntensity());

	WrappedSHFunction shFunction([&] (const Vector4& unit) -> Vector4 {
		Color4f cl(0.0f, 0.0f, 0.0f, 0.0f);
		for (int32_t i = 0; i < 1000; ++i)
		{
			const Vector2 uv = Quasirandom::hammersley(i, 1000);
			const Vector4 direction = Quasirandom::uniformHemiSphere(uv, unit);
			Scalar w = dot3(direction, unit);

			// Reduce sun influence.
			if (sunIntensity > 0.0f)
			{
				const Scalar f = clamp(dot3(direction, sunDirection), 0.0_simd, 1.0_simd);
				w *= 1.0_simd - f;
			}

			cl += cubeMap->get(direction) * w;
		}
		return (cl * intensity * 2.0_simd) / 1000.0_simd;
	});

	Ref< render::SHCoeffs > shCoeffs = new render::SHCoeffs();
	
	render::SHEngine shEngine(3);
	shEngine.generateSamplePoints(10000);
	shEngine.generateCoefficients(&shFunction, true, *shCoeffs);

	Ref< world::IrradianceGridResource > outputResource = new world::IrradianceGridResource();
	Ref< db::Instance > outputInstance = pipelineBuilder->createOutputInstance(
		outputPath,
		outputGuid
	);
	if (!outputInstance)
	{
		log::error << L"Irradiance grid pipeline failed; unable to create output instance." << Endl;
		return false;
	}

	outputInstance->setObject(outputResource);

	// Create output data stream.
	Ref< IStream > stream = outputInstance->writeData(L"Data");
	if (!stream)
	{
		log::error << L"Irradiance grid pipeline failed; unable to create texture data stream." << Endl;
		outputInstance->revert();
		return false;
	}

	Writer writer(stream);

	writer << uint32_t(2);

	writer << (uint32_t)1;	// width
	writer << (uint32_t)1;	// height
	writer << (uint32_t)1;	// depth

	writer << (float)-10000.0f;
	writer << (float)-10000.0f;
	writer << (float)-10000.0f;
	writer << (float)10000.0f;
	writer << (float)10000.0f;
	writer << (float)10000.0f;

	for (int32_t i = 0; i < 9; ++i)
	{
		auto c = (*shCoeffs)[i];
		writer << c.x();
		writer << c.y();
		writer << c.z();
	}

	stream->close();

	if (!outputInstance->commit())
	{
		log::error << L"Irradiance grid pipeline failed; unable to commit output instance." << Endl;
		return false;
	}

	return true;
}

Ref< ISerializable > IrradianceGridPipeline::buildProduct(
	editor::IPipelineBuilder* pipelineBuilder,
	const db::Instance* sourceInstance,
	const ISerializable* sourceAsset,
	const Object* buildParams
) const
{
	T_FATAL_ERROR;
	return nullptr;
}

	}
}
