#include "Core/Log/Log.h"
#include "Core/Math/Const.h"
#include "Core/Math/Random.h"
#include "Core/Serialization/AttributeDirection.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/MemberEnum.h"
#include "Core/Serialization/MemberStl.h"
#include "Core/Serialization/MemberComposite.h"
#include "Render/IRenderTargetSet.h"
#include "Render/ScreenRenderer.h"
#include "Render/Shader.h"
#include "Render/Context/RenderContext.h"
#include "Resource/IResourceManager.h"
#include "Resource/Member.h"
#include "Render/Image/ImageProcess.h"
#include "Render/Image/Steps/ImageProcessStepBlur.h"

namespace traktor
{
	namespace render
	{
		namespace
		{

Random s_random;

		}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.render.ImageProcessStepBlur", 3, ImageProcessStepBlur, ImageProcessStep)

ImageProcessStepBlur::ImageProcessStepBlur()
:	m_direction(1.0f, 0.0f, 0.0f, 0.0f)
,	m_taps(15)
,	m_blurType(BtGaussian)
{
}

Ref< ImageProcessStepBlur::Instance > ImageProcessStepBlur::create(
	resource::IResourceManager* resourceManager,
	IRenderSystem* renderSystem,
	uint32_t width,
	uint32_t height
) const
{
	resource::Proxy< Shader > shader;
	if (!resourceManager->bind(m_shader, shader))
		return nullptr;

	std::vector< InstanceBlur::Source > sources(m_sources.size());
	for (uint32_t i = 0; i < m_sources.size(); ++i)
	{
		sources[i].param = getParameterHandle(m_sources[i].param);
		sources[i].source = getParameterHandle(m_sources[i].source);
	}

	AlignedVector< Vector4 > gaussianOffsetWeights(m_taps);
	float totalWeight = 0.0f;

	if (m_blurType == BtGaussian)
	{
		float sigma = m_taps / 4.73f;

		// Iterate to prevent under or over sigma.
		for (int32_t i = 0; i < 10; ++i)
		{
			const float x = m_taps / 2.0f;
			float a = 1.0f / sqrtf(TWO_PI * sigma * sigma);
			float weight = a * std::exp(-((x * x) / (2.0f * sigma * sigma)));
			if (weight > 0.01f)
				sigma -= 0.1f;
			else if (weight < 0.001f)
				sigma += 0.01f;
			else
				break;
		}

		const float a = 1.0f / sqrtf(TWO_PI * sigma * sigma);
		for (int32_t i = 0; i < m_taps; ++i)
		{
			float x = i - m_taps / 2.0f;

			float weight = a * std::exp(-((x * x) / (2.0f * sigma * sigma)));
			totalWeight += weight;

			gaussianOffsetWeights[i].set(
				i - (m_taps - 1.0f)/ 2.0f,
				weight,
				0.0f,
				0.0f
			);
		}
	}
	else if (m_blurType == BtSine)
	{
		float step = 1.0f / (m_taps - 1.0f);
		float angleMin = (PI * step) / 2.0f;
		float angleMax = PI - angleMin;

		for (int i = 0; i < m_taps; ++i)
		{
			float phi = (float(i) * step) * (angleMax - angleMin) + angleMin;

			float weight = sinf(phi);
			totalWeight += weight;

			gaussianOffsetWeights[i].set(
				i - (m_taps - 1.0f) / 2.0f,
				weight,
				0.0f,
				0.0f
			);
		}
	}
	else if (m_blurType == BtBox)
	{
		for (int i = 0; i < m_taps; ++i)
		{
			gaussianOffsetWeights[i].set(
				i - (m_taps - 1.0f) / 2.0f,
				1.0f,
				0.0f,
				0.0f
			);
		}
		totalWeight = float(m_taps);
	}
	else if (m_blurType == BtBox2D)
	{
		for (int i = 0; i < m_taps; ++i)
		{
			float x = s_random.nextFloat() * 2.0f - 1.0f;
			float y = s_random.nextFloat() * 2.0f - 1.0f;
			gaussianOffsetWeights[i].set(
				x,
				y,
				0.0f,
				0.0f
			);
		}
	}
	else if (m_blurType == BtCircle2D)
	{
		for (int i = 0; i < m_taps; )
		{
			float x = s_random.nextFloat() * 2.0f - 1.0f;
			float y = s_random.nextFloat() * 2.0f - 1.0f;
			if (std::sqrt(x * x + y * y) <= 1.0f)
			{
				gaussianOffsetWeights[i].set(
					x,
					y,
					0.0f,
					0.0f
				);
				++i;
			}
		}
	}

	if (totalWeight > FUZZY_EPSILON)
	{
		Vector4 invWeight(1.0f, 1.0f / totalWeight, 1.0f, 1.0f);
		for (int i = 0; i < m_taps; ++i)
			gaussianOffsetWeights[i] *= invWeight;
	}

	return new InstanceBlur(
		shader,
		sources,
		m_direction,
		gaussianOffsetWeights
	);
}

void ImageProcessStepBlur::serialize(ISerializer& s)
{
	s >> resource::Member< Shader >(L"shader", m_shader);
	s >> MemberStlVector< Source, MemberComposite< Source > >(L"sources", m_sources);
	s >> Member< Vector4 >(L"direction", m_direction, AttributeDirection());
	if (s.getVersion() >= 1)
		s >> Member< int32_t >(L"taps", m_taps);
	if (s.getVersion() >= 2)
	{
		const MemberEnum< BlurType >::Key c_BlurType_Keys[] =
		{
			{ L"BtGaussian", BtGaussian },
			{ L"BtSine", BtSine },
			{ L"BtBox", BtBox },
			{ L"BtBox2D", BtBox2D },
			{ L"BtCircle2D", BtCircle2D },
			{ 0 }
		};
		s >> MemberEnum< BlurType >(L"blurType", m_blurType, c_BlurType_Keys);
	}
}

ImageProcessStepBlur::Source::Source()
{
}

void ImageProcessStepBlur::Source::serialize(ISerializer& s)
{
	s >> Member< std::wstring >(L"param", param);
	s >> Member< std::wstring >(L"source", source);

	if (s.getVersion() < 3)
	{
		uint32_t index = 0;
		s >> Member< uint32_t >(L"index", index);
		T_FATAL_ASSERT_M (index == 0, L"Index must be zero, update binding");
	}
}

// Instance

ImageProcessStepBlur::InstanceBlur::InstanceBlur(
	const resource::Proxy< Shader >& shader,
	const std::vector< Source >& sources,
	const Vector4& direction,
	const AlignedVector< Vector4 >& gaussianOffsetWeights
)
:	m_shader(shader)
,	m_sources(sources)
,	m_direction(direction)
,	m_gaussianOffsetWeights(gaussianOffsetWeights)
{
	m_handleGaussianOffsetWeights = getParameterHandle(L"GaussianOffsetWeights");
	m_handleDirection = getParameterHandle(L"Direction");
	m_handleViewFar = getParameterHandle(L"ViewFar");
	m_handleNoiseOffset = getParameterHandle(L"NoiseOffset");
}

void ImageProcessStepBlur::InstanceBlur::destroy()
{
}

void ImageProcessStepBlur::InstanceBlur::render(
	ImageProcess* imageProcess,
	RenderContext* renderContext,
	ProgramParameters* sharedParams,
	const RenderParams& params
)
{
	auto pp = renderContext->alloc< ProgramParameters >();
	pp->beginParameters(renderContext);
	pp->attachParameters(sharedParams);
	pp->setVectorArrayParameter(m_handleGaussianOffsetWeights, &m_gaussianOffsetWeights[0], (uint32_t)m_gaussianOffsetWeights.size());
	pp->setVectorParameter(m_handleDirection, m_direction * Scalar(0.5f));
	pp->setFloatParameter(m_handleViewFar, params.viewFrustum.getFarZ());
	pp->setVectorParameter(m_handleNoiseOffset, Vector4(
		s_random.nextFloat(),
		s_random.nextFloat(),
		0.0f,
		0.0f
	));

	for (const auto& s : m_sources)
	{
		ISimpleTexture* source = imageProcess->getTarget(s.source);
		if (source)
			pp->setTextureParameter(s.param, source);		
	}
	pp->endParameters(renderContext);

	imageProcess->getScreenRenderer()->draw(renderContext, m_shader, pp);
}

	}
}
