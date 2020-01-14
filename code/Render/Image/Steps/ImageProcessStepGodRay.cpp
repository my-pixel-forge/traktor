#include "Core/Misc/AutoPtr.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/MemberStl.h"
#include "Core/Serialization/MemberComposite.h"
#include "Render/IRenderSystem.h"
#include "Render/IRenderTargetSet.h"
#include "Render/ScreenRenderer.h"
#include "Render/Shader.h"
#include "Render/Context/RenderContext.h"
#include "Resource/IResourceManager.h"
#include "Resource/Member.h"
#include "Render/Image/ImageProcess.h"
#include "Render/Image/Steps/ImageProcessStepGodRay.h"

namespace traktor
{
	namespace render
	{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.render.ImageProcessStepGodRay", 1, ImageProcessStepGodRay, ImageProcessStep)

ImageProcessStepGodRay::ImageProcessStepGodRay()
:	m_lightDistance(100.0f)
{
}

Ref< ImageProcessStep::Instance > ImageProcessStepGodRay::create(
	resource::IResourceManager* resourceManager,
	IRenderSystem* renderSystem,
	uint32_t width,
	uint32_t height
) const
{
	resource::Proxy< Shader > shader;
	if (!resourceManager->bind(m_shader, shader))
		return nullptr;

	std::vector< InstanceGodRay::Source > sources(m_sources.size());
	for (uint32_t i = 0; i < m_sources.size(); ++i)
	{
		sources[i].param = getParameterHandle(m_sources[i].param);
		sources[i].source = getParameterHandle(m_sources[i].source);
	}

	return new InstanceGodRay(this, shader, sources);
}

void ImageProcessStepGodRay::serialize(ISerializer& s)
{
	s >> resource::Member< Shader >(L"shader", m_shader);
	s >> MemberStlVector< Source, MemberComposite< Source > >(L"sources", m_sources);
	s >> Member< float >(L"lightDistance", m_lightDistance);
}

ImageProcessStepGodRay::Source::Source()
{
}

void ImageProcessStepGodRay::Source::serialize(ISerializer& s)
{
	s >> Member< std::wstring >(L"param", param);
	s >> Member< std::wstring >(L"source", source);

	if (s.getVersion() < 1)
	{
		uint32_t index = 0;
		s >> Member< uint32_t >(L"index", index);
		T_FATAL_ASSERT_M (index == 0, L"Index must be zero, update binding");
	}
}

// Instance

ImageProcessStepGodRay::InstanceGodRay::InstanceGodRay(const ImageProcessStepGodRay* step, const resource::Proxy< Shader >& shader, const std::vector< Source >& sources)
:	m_step(step)
,	m_shader(shader)
,	m_sources(sources)
,	m_time(0.0f)
{
	m_handleTime = getParameterHandle(L"Time");
	m_handleDeltaTime = getParameterHandle(L"DeltaTime");
	m_handleAlpha = getParameterHandle(L"Alpha");
	m_handleScreenLightPosition = getParameterHandle(L"ScreenLightPosition");
}

void ImageProcessStepGodRay::InstanceGodRay::destroy()
{
}

void ImageProcessStepGodRay::InstanceGodRay::render(
	ImageProcess* imageProcess,
	RenderContext* renderContext,
	ProgramParameters* sharedParams,
	const RenderParams& params
)
{
	Vector4 lightPositionView = params.view * (params.godRayDirection * Scalar(m_step->m_lightDistance)).xyz1();
	Vector4 lightPosition = params.projection * lightPositionView;

	// \tbd
	// If light is behind camera near plane then skip god rays; but
	// ensure target is cleared so no lingering rays are kept.
	// if (params.godRayDirection.length2() <= FUZZY_EPSILON || lightPosition.w() <= 0.0f)
	// {
	// 	const static Color4f c_clearColor(0.0f, 0.0f, 0.0f, 0.0f);
	// 	renderView->clear(CfColor, &c_clearColor, 0.0f, 0);
	// 	return;
	// }

	lightPosition /= lightPosition.w();

	auto pp = renderContext->alloc< ProgramParameters >();
	pp->beginParameters(renderContext);
	pp->attachParameters(sharedParams);
	pp->setFloatParameter(m_handleTime, m_time);
	pp->setFloatParameter(m_handleDeltaTime, params.deltaTime);
	pp->setFloatParameter(m_handleAlpha, abs(lightPositionView.xyz0().normalized().z()));
	pp->setVectorParameter(m_handleScreenLightPosition, lightPosition * Vector4(1.0f, -1.0f, 0.0f, 0.0f));
	for (const auto& s : m_sources)
	{
		ISimpleTexture* source = imageProcess->getTarget(s.source);
		if (source)
			pp->setTextureParameter(s.param, source);		
	}
	pp->endParameters(renderContext);

	imageProcess->getScreenRenderer()->draw(renderContext, m_shader, pp);

	m_time += params.deltaTime;
}

	}
}
