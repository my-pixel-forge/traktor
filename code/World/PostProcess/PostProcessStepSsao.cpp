#include "Core/Math/Const.h"
#include "Core/Math/RandomGeometry.h"
#include "Core/Misc/AutoPtr.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/MemberStl.h"
#include "Core/Serialization/MemberComposite.h"
#include "Render/IRenderSystem.h"
#include "Render/ISimpleTexture.h"
#include "Render/RenderTargetSet.h"
#include "Render/ScreenRenderer.h"
#include "Render/Shader.h"
#include "Resource/IResourceManager.h"
#include "Resource/Member.h"
#include "World/WorldRenderView.h"
#include "World/PostProcess/PostProcess.h"
#include "World/PostProcess/PostProcessStepSsao.h"

namespace traktor
{
	namespace world
	{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.world.PostProcessStepSsao", 1, PostProcessStepSsao, PostProcessStep)

Ref< PostProcessStep::Instance > PostProcessStepSsao::create(
	resource::IResourceManager* resourceManager,
	render::IRenderSystem* renderSystem,
	uint32_t width,
	uint32_t height
) const
{
	resource::Proxy< render::Shader > shader;
	if (!resourceManager->bind(m_shader, shader))
		return 0;

	std::vector< InstanceSsao::Source > sources(m_sources.size());
	for (uint32_t i = 0; i < m_sources.size(); ++i)
	{
		sources[i].param = render::getParameterHandle(m_sources[i].param);
		sources[i].source = render::getParameterHandle(m_sources[i].source);
		sources[i].index = m_sources[i].index;
	}

	RandomGeometry random;
	Vector4 offsets[64];

	for (int i = 0; i < sizeof_array(offsets); ++i)
	{
		float r = random.nextFloat() * (1.0f - 0.1f) + 0.1f;
		offsets[i] = random.nextUnit().xyz0() + Vector4(0.0f, 0.0f, 0.0f, r);
	}

	AutoArrayPtr< uint8_t > data(new uint8_t [256 * 256 * 4]);
	for (uint32_t y = 0; y < 256; ++y)
	{
		for (uint32_t x = 0; x < 256; ++x)
		{
			// Randomized normal; attenuated by horizon in order to reduce near surface noise.
			Vector4 normal = random.nextUnit();
			normal = normal * Scalar(0.5f) + Scalar(0.5f);

			data[(x + y * 256) * 4 + 0] = uint8_t(normal.x() * 255);
			data[(x + y * 256) * 4 + 1] = uint8_t(normal.y() * 255);
			data[(x + y * 256) * 4 + 2] = uint8_t(normal.z() * 255);
			data[(x + y * 256) * 4 + 3] = 0;
		}
	}

	render::SimpleTextureCreateDesc desc;
	desc.width = 256;
	desc.height = 256;
	desc.mipCount = 1;
	desc.format = render::TfR8G8B8A8;
	desc.immutable = true;
	desc.initialData[0].data = data.ptr();
	desc.initialData[0].pitch = 256 * 4;
	desc.initialData[0].slicePitch = 0;

	Ref< render::ISimpleTexture > randomNormals = renderSystem->createSimpleTexture(desc);
	if (!randomNormals)
		return 0;

	return new InstanceSsao(this, sources, offsets, shader, randomNormals);
}

bool PostProcessStepSsao::serialize(ISerializer& s)
{
	s >> resource::Member< render::Shader >(L"shader", m_shader);
	
	if (s.getVersion() >= 1)
		s >> MemberStlVector< Source, MemberComposite< Source > >(L"sources", m_sources);

	return true;
}

PostProcessStepSsao::Source::Source()
:	index(0)
{
}

bool PostProcessStepSsao::Source::serialize(ISerializer& s)
{
	s >> Member< std::wstring >(L"param", param);
	s >> Member< std::wstring >(L"source", source);
	s >> Member< uint32_t >(L"index", index);
	return true;
}

// Instance

PostProcessStepSsao::InstanceSsao::InstanceSsao(
	const PostProcessStepSsao* step,
	const std::vector< Source >& sources,
	const Vector4 offsets[64],
	const resource::Proxy< render::Shader >& shader,
	render::ISimpleTexture* randomNormals
)
:	m_step(step)
,	m_sources(sources)
,	m_shader(shader)
,	m_randomNormals(randomNormals)
,	m_handleInputColor(render::getParameterHandle(L"InputColor"))
,	m_handleInputDepth(render::getParameterHandle(L"InputDepth"))
,	m_handleViewEdgeTopLeft(render::getParameterHandle(L"ViewEdgeTopLeft"))
,	m_handleViewEdgeTopRight(render::getParameterHandle(L"ViewEdgeTopRight"))
,	m_handleViewEdgeBottomLeft(render::getParameterHandle(L"ViewEdgeBottomLeft"))
,	m_handleViewEdgeBottomRight(render::getParameterHandle(L"ViewEdgeBottomRight"))
,	m_handleProjection(render::getParameterHandle(L"Projection"))
,	m_handleOffsets(render::getParameterHandle(L"Offsets"))
,	m_handleRandomNormals(render::getParameterHandle(L"RandomNormals"))
,	m_handleMagicCoeffs(render::getParameterHandle(L"MagicCoeffs"))
{
	for (int i = 0; i < sizeof_array(m_offsets); ++i)
		m_offsets[i] = offsets[i];
}

void PostProcessStepSsao::InstanceSsao::destroy()
{
	m_randomNormals->destroy();
}

void PostProcessStepSsao::InstanceSsao::render(
	PostProcess* postProcess,
	render::IRenderView* renderView,
	render::ScreenRenderer* screenRenderer,
	const RenderParams& params
)
{
	Ref< render::RenderTargetSet > sourceColor = postProcess->getTargetRef(m_handleInputColor);
	Ref< render::RenderTargetSet > sourceDepth = postProcess->getTargetRef(m_handleInputDepth);
	if (!sourceColor || !sourceDepth)
		return;

	postProcess->prepareShader(m_shader);

	Vector4 sourceColorSize(
		float(sourceColor->getWidth()),
		float(sourceColor->getHeight()),
		0.0f,
		0.0f
	);
	Vector4 sourceDepthSize(
		float(sourceDepth->getWidth()),
		float(sourceDepth->getHeight()),
		0.0f,
		0.0f
	);

	Scalar p11 = params.projection.get(0, 0);
	Scalar p22 = params.projection.get(1, 1);
	Vector4 viewEdgeTopLeft = params.viewFrustum.corners[4];
	Vector4 viewEdgeTopRight = params.viewFrustum.corners[5];
	Vector4 viewEdgeBottomLeft = params.viewFrustum.corners[7];
	Vector4 viewEdgeBottomRight = params.viewFrustum.corners[6];

	m_shader->setVectorParameter(m_handleViewEdgeTopLeft, viewEdgeTopLeft);
	m_shader->setVectorParameter(m_handleViewEdgeTopRight, viewEdgeTopRight);
	m_shader->setVectorParameter(m_handleViewEdgeBottomLeft, viewEdgeBottomLeft);
	m_shader->setVectorParameter(m_handleViewEdgeBottomRight, viewEdgeBottomRight);
	m_shader->setMatrixParameter(m_handleProjection, params.projection);
	m_shader->setVectorArrayParameter(m_handleOffsets, m_offsets, sizeof_array(m_offsets));
	m_shader->setTextureParameter(m_handleRandomNormals, m_randomNormals);
	m_shader->setVectorParameter(m_handleMagicCoeffs, Vector4(1.0f / p11, 1.0f / p22, 0.0f, 0.0f));

	for (std::vector< Source >::const_iterator i = m_sources.begin(); i != m_sources.end(); ++i)
	{
		Ref< render::RenderTargetSet > source = postProcess->getTargetRef(i->source);
		if (source)
			m_shader->setTextureParameter(i->param, source->getColorTexture(i->index));
	}

	screenRenderer->draw(renderView, m_shader);
}

	}
}
