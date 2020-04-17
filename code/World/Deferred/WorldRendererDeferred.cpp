#include <limits>
#include "Core/Log/Log.h"
#include "Core/Math/Log2.h"
#include "Core/Math/Float.h"
#include "Core/Misc/SafeDestroy.h"
#include "Core/Misc/String.h"
#include "Render/IRenderSystem.h"
#include "Render/IRenderTargetSet.h"
#include "Render/IRenderView.h"
#include "Render/ScreenRenderer.h"
#include "Render/Shader.h"
#include "Render/StructBuffer.h"
#include "Render/StructElement.h"
#include "Render/Context/RenderContext.h"
#include "Render/Frame/RenderGraph.h"
#include "Render/Image2/ImageGraph.h"
#include "Render/Image2/ImageGraphContext.h"
#include "Render/Image2/ImageGraphData.h"
#include "Resource/IResourceManager.h"
#include "World/Entity.h"
#include "World/IrradianceGrid.h"
#include "World/WorldBuildContext.h"
#include "World/WorldGatherContext.h"
#include "World/WorldHandles.h"
#include "World/WorldSetupContext.h"
#include "World/Deferred/WorldRendererDeferred.h"
#include "World/Deferred/WorldRenderPassDeferred.h"
#include "World/SMProj/UniformShadowProjection.h"

namespace traktor
{
	namespace world
	{
		namespace
		{

#if defined(__ANDROID__) || defined(__IOS__)
const int32_t c_maxLightCount = 16;
#else
const int32_t c_maxLightCount = 1024;
#endif

const resource::Id< render::Shader > c_lightShader(L"{707DE0B0-0E2B-A44A-9441-9B1FCFD428AA}");
const resource::Id< render::Shader > c_reflectionShader(L"{F04EEA34-85E0-974F-BE97-79D24C6ACFBD}");
const resource::Id< render::Shader > c_fogShader(L"{9453D74C-76C4-8748-9A5B-9E3D6D4F9406}");

const resource::Id< render::ImageGraph > c_ambientOcclusionLow(L"{416745F9-93C7-8D45-AE28-F2823DEE636A}");
const resource::Id< render::ImageGraph > c_ambientOcclusionMedium(L"{5A3B0260-32F9-B343-BBA4-88BD932F917A}");
const resource::Id< render::ImageGraph > c_ambientOcclusionHigh(L"{45F9CD9F-C700-9942-BB36-443629C88748}");
const resource::Id< render::ImageGraph > c_ambientOcclusionUltra(L"{302E57C8-711D-094F-A764-75F76553E81B}");
const resource::Id< render::ImageGraph > c_antiAliasLow(L"{71D385F1-8364-C849-927F-5F1249F5DF92}");
const resource::Id< render::ImageGraph > c_antiAliasMedium(L"{D03B9566-EFA3-7A43-B3AD-F59DB34DEE96}");
const resource::Id< render::ImageGraph > c_antiAliasHigh(L"{C0316981-FA73-A34E-8135-1F596425688F}");
const resource::Id< render::ImageGraph > c_antiAliasUltra(L"{88E329C8-A2F3-7443-B73E-4E85C6ECACBE}");
const resource::Id< render::ImageGraph > c_gammaCorrection(L"{B1E8367D-91DD-D648-A44F-B86492169771}");
const resource::Id< render::ImageGraph > c_motionBlurPrime(L"{CB34E98B-55C9-E447-BD59-5A1D91DCA88E}");
const resource::Id< render::ImageGraph > c_motionBlurLow(L"{E813C1A0-D27D-AE4F-9EE4-637529ECCD69}");
const resource::Id< render::ImageGraph > c_motionBlurMedium(L"{E813C1A0-D27D-AE4F-9EE4-637529ECCD69}");
const resource::Id< render::ImageGraph > c_motionBlurHigh(L"{E813C1A0-D27D-AE4F-9EE4-637529ECCD69}");
const resource::Id< render::ImageGraph > c_motionBlurUltra(L"{E813C1A0-D27D-AE4F-9EE4-637529ECCD69}");
const resource::Id< render::ImageGraph > c_toneMapFixed(L"{1F20DAB5-22EB-B84C-92B0-71E94C1CE261}");
const resource::Id< render::ImageGraph > c_toneMapAdaptive(L"{1F20DAB5-22EB-B84C-92B0-71E94C1CE261}");

resource::Id< render::ImageGraph > getAmbientOcclusionId(Quality quality)
{
	switch (quality)
	{
	default:
	case QuDisabled:
		return resource::Id< render::ImageGraph >();
	case QuLow:
		return c_ambientOcclusionLow;
	case QuMedium:
		return c_ambientOcclusionMedium;
	case QuHigh:
		return c_ambientOcclusionHigh;
	case QuUltra:
		return c_ambientOcclusionUltra;
	}
}

resource::Id< render::ImageGraph > getAntiAliasId(Quality quality)
{
	switch (quality)
	{
	default:
	case QuDisabled:
		return resource::Id< render::ImageGraph >();
	case QuLow:
		return c_antiAliasLow;
	case QuMedium:
		return c_antiAliasMedium;
	case QuHigh:
		return c_antiAliasHigh;
	case QuUltra:
		return c_antiAliasUltra;
	}
}

resource::Id< render::ImageGraph > getMotionBlurId(Quality quality)
{
	switch (quality)
	{
	default:
	case QuDisabled:
		return resource::Id< render::ImageGraph >();
	case QuLow:
		return c_motionBlurLow;
	case QuMedium:
		return c_motionBlurMedium;
	case QuHigh:
		return c_motionBlurHigh;
	case QuUltra:
		return c_motionBlurUltra;
	}
}

resource::Id< render::ImageGraph > getToneMapId(WorldRenderSettings::ExposureMode exposureMode)
{
	switch (exposureMode)
	{
	default:
	case WorldRenderSettings::EmFixed:
		return c_toneMapFixed;
	case WorldRenderSettings::EmAdaptive:
		return c_toneMapAdaptive;
	}
}

		}

#pragma pack(1)
struct LightShaderData
{
	float typeRangeRadius[4];
	float position[4];
	float direction[4];
	float color[4];
	float viewToLight0[4];
	float viewToLight1[4];
	float viewToLight2[4];
	float viewToLight3[4];
	float atlasTransform[4];
};
#pragma pack()

#pragma pack(1)
struct TileShaderData
{
	float lights[4];
	float lightCount[4];
};
#pragma pack()

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.world.WorldRendererDeferred", 0, WorldRendererDeferred, IWorldRenderer)

WorldRendererDeferred::WorldRendererDeferred()
:	m_toneMapQuality(QuDisabled)
,	m_motionBlurQuality(QuDisabled)
,	m_shadowsQuality(QuDisabled)
,	m_ambientOcclusionQuality(QuDisabled)
,	m_antiAliasQuality(QuDisabled)
,	m_count(0)
{
}

bool WorldRendererDeferred::create(
	resource::IResourceManager* resourceManager,
	render::IRenderSystem* renderSystem,
	const WorldCreateDesc& desc
)
{
	// Store settings.
	m_settings = *desc.worldRenderSettings;
	m_toneMapQuality = desc.toneMapQuality;
	m_motionBlurQuality = desc.motionBlurQuality;
	m_shadowSettings = m_settings.shadowSettings[desc.shadowsQuality];
	m_shadowsQuality = desc.shadowsQuality;
	m_reflectionsQuality = desc.reflectionsQuality;
	m_ambientOcclusionQuality = desc.ambientOcclusionQuality;
	m_antiAliasQuality = desc.antiAliasQuality;
	m_sharedDepthStencil = desc.sharedDepthStencil;

	// Allocate frames.
	m_frames.resize(desc.frameCount);

	// Pack fog parameters.
	m_fogDistanceAndDensity = Vector4(
		m_settings.fogDistanceY,
		m_settings.fogDistanceZ,
		m_settings.fogDensityY,
		m_settings.fogDensityZ
	);
	m_fogColor = m_settings.fogColor;

	// Create light, reflection and fog shaders.
	if (!resourceManager->bind(c_lightShader, m_lightShader))
		return false;
	if (!resourceManager->bind(c_reflectionShader, m_reflectionShader))
		return false;
	if (!resourceManager->bind(c_fogShader, m_fogShader))
		return false;

	// Create shadow screen projection processes.
	if (m_shadowsQuality > QuDisabled)
	{
		if (!resourceManager->bind(m_shadowSettings.maskProject, m_shadowMaskProject))
		{
			log::warning << L"Unable to create shadow project process; shadows disabled." << Endl;
			m_shadowsQuality = QuDisabled;
		}
	}

	// Create ambient occlusion processing.
	if (m_ambientOcclusionQuality > QuDisabled)
	{
		resource::Id< render::ImageGraph > ambientOcclusion = getAmbientOcclusionId(m_ambientOcclusionQuality);
		if (!resourceManager->bind(ambientOcclusion, m_ambientOcclusion))
			log::warning << L"Unable to create ambient occlusion process; AO disabled." << Endl;
	}

	// Create antialias processing.
	if (m_antiAliasQuality > QuDisabled)
	{
		resource::Id< render::ImageGraph > antiAlias = getAntiAliasId(m_antiAliasQuality);
		if (!resourceManager->bind(antiAlias, m_antiAlias))
			log::warning << L"Unable to create antialias process; AA disabled." << Endl;
	}

	// Create "visual" post processing filter.
	if (desc.imageProcessQuality > QuDisabled)
	{
		const auto& visualImageGraph = desc.worldRenderSettings->imageProcess[desc.imageProcessQuality];
		if (!resourceManager->bind(visualImageGraph, m_visual))
			log::warning << L"Unable to create visual post processing; post processing disabled." << Endl;
	}

	// Create gamma correction processing.
	if (
		m_settings.linearLighting &&
		std::abs(desc.gamma - 1.0f) > FUZZY_EPSILON
	)
	{
		if (resourceManager->bind(c_gammaCorrection, m_gammaCorrection))
		{
			m_gammaCorrection->setFloatParameter(s_handleGamma, desc.gamma);
			m_gammaCorrection->setFloatParameter(s_handleGammaInverse, 1.0f / desc.gamma);
		}
		else
			log::warning << L"Unable to create gamma correction process; gamma correction disabled." << Endl;
	}

	// Create motion blur prime processing.
	if (m_motionBlurQuality > QuDisabled)
	{
		if (!resourceManager->bind(c_motionBlurPrime, m_motionBlurPrime))
		{
			log::warning << L"Unable to create motion blur prime process; motion blur disabled." << Endl;
			m_motionBlurQuality = QuDisabled;
		}
	}

	// Create motion blur final processing.
	if (m_motionBlurQuality > QuDisabled)
	{
		resource::Id< render::ImageGraph > motionBlur = getMotionBlurId(desc.motionBlurQuality);
		if (!resourceManager->bind(motionBlur, m_motionBlur))
		{
			log::warning << L"Unable to create motion blur process; motion blur disabled." << Endl;
			m_motionBlurQuality = QuDisabled;
		}
	}

	// Create tone map processing.
	if (m_toneMapQuality > QuDisabled)
	{
		resource::Id< render::ImageGraph > toneMap = getToneMapId(m_settings.exposureMode);
		if (resourceManager->bind(toneMap, m_toneMap))
			m_toneMap->setFloatParameter(s_handleExposure, m_settings.exposure);
		else
		{
			log::warning << L"Unable to create tone map process." << Endl;
			m_toneMapQuality = QuDisabled;
		}
	}

	// Create shadow maps.
	if (m_shadowsQuality > QuDisabled)
	{
		const auto& shadowSettings = m_settings.shadowSettings[m_shadowsQuality];
		render::RenderTargetSetCreateDesc rtscd;

		// Cascading shadow map.
		rtscd.count = 0;
		rtscd.width = shadowSettings.resolution;
		rtscd.height = shadowSettings.cascadingSlices * shadowSettings.resolution;
		rtscd.multiSample = 0;
		rtscd.createDepthStencil = true;
		rtscd.usingDepthStencilAsTexture = true;
		rtscd.usingPrimaryDepthStencil = false;
		rtscd.ignoreStencil = true;
		m_shadowMapCascadeTargetSet = renderSystem->createRenderTargetSet(rtscd, nullptr, T_FILE_LINE_W);

		// Atlas shadow map.
		rtscd.count = 0;
		rtscd.width =
		rtscd.height = 4096;
		rtscd.multiSample = 0;
		rtscd.createDepthStencil = true;
		rtscd.usingDepthStencilAsTexture = true;
		rtscd.usingPrimaryDepthStencil = false;
		rtscd.ignoreStencil = true;
		m_shadowMapAtlasTargetSet = renderSystem->createRenderTargetSet(rtscd, nullptr, T_FILE_LINE_W);
	}

	// Allocate light lists.
	for (auto& frame : m_frames)
	{
		AlignedVector< render::StructElement > lightShaderDataStruct;
		lightShaderDataStruct.push_back(render::StructElement(render::DtFloat4, offsetof(LightShaderData, typeRangeRadius)));
		lightShaderDataStruct.push_back(render::StructElement(render::DtFloat4, offsetof(LightShaderData, position)));
		lightShaderDataStruct.push_back(render::StructElement(render::DtFloat4, offsetof(LightShaderData, direction)));
		lightShaderDataStruct.push_back(render::StructElement(render::DtFloat4, offsetof(LightShaderData, color)));
		lightShaderDataStruct.push_back(render::StructElement(render::DtFloat4, offsetof(LightShaderData, viewToLight0)));
		lightShaderDataStruct.push_back(render::StructElement(render::DtFloat4, offsetof(LightShaderData, viewToLight1)));
		lightShaderDataStruct.push_back(render::StructElement(render::DtFloat4, offsetof(LightShaderData, viewToLight2)));
		lightShaderDataStruct.push_back(render::StructElement(render::DtFloat4, offsetof(LightShaderData, viewToLight3)));
		lightShaderDataStruct.push_back(render::StructElement(render::DtFloat4, offsetof(LightShaderData, atlasTransform)));
		T_FATAL_ASSERT(sizeof(LightShaderData) == render::getStructSize(lightShaderDataStruct));

		frame.lightSBuffer = renderSystem->createStructBuffer(
			lightShaderDataStruct,
			render::getStructSize(lightShaderDataStruct) * c_maxLightCount
		);
		if (!frame.lightSBuffer)
			return false;

		frame.lightSBufferMemory = frame.lightSBuffer->lock();
		if (!frame.lightSBufferMemory)
			return false;

		AlignedVector< render::StructElement > tileShaderDataStruct;
		tileShaderDataStruct.push_back(render::StructElement(render::DtFloat4, offsetof(TileShaderData, lights)));
		tileShaderDataStruct.push_back(render::StructElement(render::DtFloat4, offsetof(TileShaderData, lightCount)));
		T_FATAL_ASSERT(sizeof(TileShaderData) == render::getStructSize(tileShaderDataStruct));

		frame.tileSBuffer = renderSystem->createStructBuffer(
			tileShaderDataStruct,
			render::getStructSize(tileShaderDataStruct) * 16 * 16
		);
		if (!frame.tileSBuffer)
			return false;

		frame.tileSBufferMemory = frame.tileSBuffer->lock();
		if (!frame.tileSBufferMemory)
			return false;
	}

	// Create irradiance grid.
	if (!m_settings.irradianceGrid.isNull())
	{
		if (!resourceManager->bind(m_settings.irradianceGrid, m_irradianceGrid))
			return false;
	}

	// Determine slice distances.
	for (int32_t i = 0; i < m_shadowSettings.cascadingSlices; ++i)
	{
		float ii = float(i) / m_shadowSettings.cascadingSlices;
		float log = powf(ii, m_shadowSettings.cascadingLambda);
		m_slicePositions[i] = lerp(m_settings.viewNearZ, m_shadowSettings.farZ, log);
	}
	m_slicePositions[m_shadowSettings.cascadingSlices] = m_shadowSettings.farZ;

	m_entityRenderers = desc.entityRenderers;

	// Create screen renderer.
	m_screenRenderer = new render::ScreenRenderer();
	if (!m_screenRenderer->create(renderSystem))
		return false;

	return true;
}

void WorldRendererDeferred::destroy()
{
	for (auto& frame : m_frames)
	{
		safeDestroy(frame.lightSBuffer);
		safeDestroy(frame.tileSBuffer);
	}
	m_frames.clear();

	safeDestroy(m_shadowMapCascadeTargetSet);
	safeDestroy(m_shadowMapAtlasTargetSet);
	safeDestroy(m_screenRenderer);

	m_irradianceGrid.clear();
}

void WorldRendererDeferred::setup(
	const WorldRenderView& worldRenderView,
	const Entity* rootEntity,
	render::RenderGraph& renderGraph,
	render::handle_t outputTargetSetId
)
{
	int32_t frame = m_count % (int32_t)m_frames.size();

	// Gather active lights.
	m_lights.resize(0);
	WorldGatherContext(m_entityRenderers, rootEntity).gather(rootEntity, m_lights);
	if (m_lights.size() > c_maxLightCount)
		m_lights.resize(c_maxLightCount);

	// Begun writing light shader data; written both in setup and build.
	LightShaderData* lightShaderData = (LightShaderData*)m_frames[frame].lightSBufferMemory;
	TileShaderData* tileShaderData = (TileShaderData*)m_frames[frame].tileSBufferMemory;

	// Write all lights to sbuffer; without shadow map information.
	const Matrix44& view = worldRenderView.getView();
	for (int32_t i = 0; i < (int32_t)m_lights.size(); ++i)
	{
		const auto& light = m_lights[i];
		auto* lsd = &lightShaderData[i];

		lsd->typeRangeRadius[0] = (float)light.type;
		lsd->typeRangeRadius[1] = light.range;
		lsd->typeRangeRadius[2] = light.radius / 2.0f;
		lsd->typeRangeRadius[3] = 0.0f;

		(view * light.position.xyz1()).storeUnaligned(lsd->position);
		(view * light.direction.xyz0()).storeUnaligned(lsd->direction);
		light.color.storeUnaligned(lsd->color);

		Vector4::zero().storeUnaligned(lsd->viewToLight0);
		Vector4::zero().storeUnaligned(lsd->viewToLight1);
		Vector4::zero().storeUnaligned(lsd->viewToLight2);
		Vector4::zero().storeUnaligned(lsd->viewToLight3);
	}

	// Find directional light for cascade shadow map.
	int32_t lightCascadeIndex = -1;
	if (m_shadowsQuality != QuDisabled)
	{
		for (int32_t i = 0; i < (int32_t)m_lights.size(); ++i)
		{
			const auto& light = m_lights[i];
			if (light.castShadow && light.type == LtDirectional)
			{
				lightCascadeIndex = i;
				break;
			}
		}
	}

	// Find spot lights for atlas shadow map.
	StaticVector< int32_t, 16 > lightAtlasIndices;
	if (m_shadowsQuality != QuDisabled)
	{
		for (int32_t i = 0; i < (int32_t)m_lights.size(); ++i)
		{
			const auto& light = m_lights[i];
			if (light.castShadow && light.type == LtSpot)
				lightAtlasIndices.push_back(i);
		}
	}

	// Add additional passes by entity renderers.
	{
		WorldSetupContext context(m_entityRenderers, rootEntity, renderGraph);
		context.setup(worldRenderView, rootEntity);
		context.flush();
	}
	
	// Add passes to render graph.
	auto gbufferTargetSetId = setupGBufferPass(
		worldRenderView,
		rootEntity,
		renderGraph,
		outputTargetSetId
	);

	auto velocityTargetSetId = setupVelocityPass(
		worldRenderView,
		rootEntity,
		renderGraph,
		outputTargetSetId,
		gbufferTargetSetId
	);

	auto ambientOcclusionTargetSetId = setupAmbientOcclusionPass(
		worldRenderView,
		rootEntity,
		renderGraph,
		outputTargetSetId,
		gbufferTargetSetId
	);

	auto shadowMapCascadeTargetSetId = setupCascadeShadowMapPass(
		worldRenderView,
		rootEntity,
		renderGraph,
		outputTargetSetId,
		lightCascadeIndex,
		lightShaderData
	);

	auto shadowMapAtlasTargetSetId = setupAtlasShadowMapPass(
		worldRenderView,
		rootEntity,
		renderGraph,
		outputTargetSetId,
		lightAtlasIndices,
		lightShaderData
	);

	setupTileDataPass(
		worldRenderView,
		rootEntity,
		renderGraph,
		outputTargetSetId,
		tileShaderData
	);

	auto shadowMaskTargetSetId = setupShadowMaskPass(
		worldRenderView,
		rootEntity,
		renderGraph,
		outputTargetSetId,
		gbufferTargetSetId,
		shadowMapCascadeTargetSetId,
		lightCascadeIndex
	);

	auto reflectionsTargetSetId = setupReflectionsPass(
		worldRenderView,
		rootEntity,
		renderGraph,
		outputTargetSetId,
		gbufferTargetSetId,
		0 // visualTargetSetId
	);

	auto visualTargetSetId = setupVisualPass(
		worldRenderView,
		rootEntity,
		renderGraph,
		outputTargetSetId,
		gbufferTargetSetId,
		ambientOcclusionTargetSetId,
		reflectionsTargetSetId,
		shadowMaskTargetSetId,
		shadowMapAtlasTargetSetId,
		frame
	);

	setupProcessPass(
		worldRenderView,
		rootEntity,
		renderGraph,
		outputTargetSetId,
		gbufferTargetSetId,
		velocityTargetSetId,
		visualTargetSetId
	);

	m_count++;
}

render::handle_t WorldRendererDeferred::setupGBufferPass(
	const WorldRenderView& worldRenderView,
	const Entity* rootEntity,
	render::RenderGraph& renderGraph,
	render::handle_t outputTargetSetId
) const
{
	const float clearZ = m_settings.viewFarZ;

	// Add GBuffer target set.
	render::RenderGraphTargetSetDesc rgtd = {};
	rgtd.count = 4;
	rgtd.createDepthStencil = false;
	rgtd.usingPrimaryDepthStencil = (m_sharedDepthStencil == nullptr) ? true : false;
	rgtd.targets[0].colorFormat = render::TfR16F;		// Depth (R)
	rgtd.targets[1].colorFormat = render::TfR16G16F;	// Normals (RG)
	rgtd.targets[2].colorFormat = render::TfR11G11B10F;	// Metalness (R), Roughness (G), Specular (B)
	rgtd.targets[3].colorFormat = render::TfR11G11B10F;	// Surface color (RGB)
	rgtd.referenceWidthDenom = 1;
	rgtd.referenceHeightDenom = 1;
	auto gbufferTargetSetId = renderGraph.addTargetSet(rgtd, m_sharedDepthStencil, outputTargetSetId);

	// Add GBuffer render pass.
	Ref< render::RenderPass > rp = new render::RenderPass(L"GBuffer");

	render::Clear clear;
	clear.mask = render::CfColor | render::CfDepth | render::CfStencil;
	clear.colors[0] = Color4f(clearZ, clearZ, clearZ, clearZ);	// depth
	clear.colors[1] = Color4f(0.0f, 0.0f, 1.0f, 0.0f);	// normal
	clear.colors[2] = Color4f(0.0f, 1.0f, 0.0f, 1.0f);	// misc
	clear.colors[3] = Color4f(0.0f, 0.0f, 0.0f, 0.0f);	// surface
	clear.depth = 1.0f;	
	clear.stencil = 0;
	rp->setOutput(gbufferTargetSetId, clear);

	rp->addBuild(
		[=](const render::RenderGraph& renderGraph, render::RenderContext* renderContext)
		{
			WorldBuildContext wc(
				m_entityRenderers,
				rootEntity,
				renderContext
			);

			auto sharedParams = renderContext->alloc< render::ProgramParameters >();
			sharedParams->beginParameters(renderContext);
			sharedParams->setFloatParameter(s_handleTime, worldRenderView.getTime());
			sharedParams->setMatrixParameter(s_handleView, worldRenderView.getView());
			sharedParams->setMatrixParameter(s_handleViewInverse, worldRenderView.getView().inverse());
			sharedParams->setMatrixParameter(s_handleProjection, worldRenderView.getProjection());
			sharedParams->endParameters(renderContext);

			WorldRenderPassDeferred gbufferPass(
				s_techniqueDeferredGBufferWrite,
				sharedParams,
				worldRenderView,
				IWorldRenderPass::PfFirst,
				false,
				false,
				false
			);

			T_ASSERT(!renderContext->havePendingDraws());
			wc.build(worldRenderView, gbufferPass, rootEntity);
			wc.flush(worldRenderView, gbufferPass);
			renderContext->merge(render::RpAll);
		}
	);

	renderGraph.addPass(rp);
	return gbufferTargetSetId;
}

render::handle_t WorldRendererDeferred::setupVelocityPass(
	const WorldRenderView& worldRenderView,
	const Entity* rootEntity,
	render::RenderGraph& renderGraph,
	render::handle_t outputTargetSetId,
	render::handle_t gbufferTargetSetId
) const
{
	if (m_motionBlurQuality == QuDisabled)
		return 0;

	// Add Velocity target set.
	render::RenderGraphTargetSetDesc rgtd = {};
	rgtd.count = 1;
	rgtd.createDepthStencil = false;
	rgtd.usingPrimaryDepthStencil = (m_sharedDepthStencil == nullptr) ? true : false;
	rgtd.targets[0].colorFormat = render::TfR16G16F;
	rgtd.referenceWidthDenom = 1;
	rgtd.referenceHeightDenom = 1;
	auto velocityTargetSetId = renderGraph.addTargetSet(rgtd, m_sharedDepthStencil, outputTargetSetId);

	// Add Velocity render pass.
	Ref< render::RenderPass > rp = new render::RenderPass(L"Velocity");
	
	if (m_motionBlurPrime)
	{
		render::ImageGraphParams ipd;
		ipd.viewFrustum = worldRenderView.getViewFrustum();
		ipd.lastView = worldRenderView.getLastView();
		ipd.view = worldRenderView.getView();
		ipd.projection = worldRenderView.getProjection();
		ipd.deltaTime = 1.0f / 60.0f;

		render::ImageGraphContext cx(m_screenRenderer);
		cx.associateTextureTargetSet(s_handleInputDepth, gbufferTargetSetId, 0);
		cx.setParams(ipd);

		m_motionBlurPrime->addPasses(renderGraph, rp, cx);
	}

	rp->setOutput(velocityTargetSetId);

	rp->addBuild(
		[=](const render::RenderGraph& renderGraph, render::RenderContext* renderContext)
		{
			WorldBuildContext wc(
				m_entityRenderers,
				rootEntity,
				renderContext
			);

			auto sharedParams = renderContext->alloc< render::ProgramParameters >();
			sharedParams->beginParameters(renderContext);
			sharedParams->setFloatParameter(s_handleTime, worldRenderView.getTime());
			sharedParams->setMatrixParameter(s_handleView, worldRenderView.getView());
			sharedParams->setMatrixParameter(s_handleViewInverse, worldRenderView.getView().inverse());
			sharedParams->setMatrixParameter(s_handleProjection, worldRenderView.getProjection());
			sharedParams->endParameters(renderContext);

			WorldRenderPassDeferred velocityPass(
				s_techniqueVelocityWrite,
				sharedParams,
				worldRenderView,
				IWorldRenderPass::PfNone,
				false,
				false,
				false
			);

			wc.build(worldRenderView, velocityPass, rootEntity);
			wc.flush(worldRenderView, velocityPass);
			renderContext->merge(render::RpAll);
		}
	);

	renderGraph.addPass(rp);
	return velocityTargetSetId;
}

render::handle_t WorldRendererDeferred::setupAmbientOcclusionPass(
	const WorldRenderView& worldRenderView,
	const Entity* rootEntity,
	render::RenderGraph& renderGraph,
	render::handle_t outputTargetSetId,
	render::handle_t gbufferTargetSetId
) const
{
	// Add ambient occlusion target set.
	render::RenderGraphTargetSetDesc rgtd;
	rgtd.count = 1;
	rgtd.createDepthStencil = false;
	rgtd.usingPrimaryDepthStencil = false;
	rgtd.targets[0].colorFormat = render::TfR8;			// Ambient occlusion (R)
	rgtd.referenceWidthDenom = 1;
	rgtd.referenceHeightDenom = 1;
	auto ambientOcclusionTargetSetId = renderGraph.addTargetSet(rgtd, nullptr, outputTargetSetId);

	// Add ambient occlusion render pass.
	Ref< render::RenderPass > rp = new render::RenderPass(L"Ambient occlusion");

	if (m_ambientOcclusion != nullptr)
	{
		render::ImageGraphParams ipd;
		ipd.viewFrustum = worldRenderView.getViewFrustum();
		ipd.view = worldRenderView.getView();
		ipd.projection = worldRenderView.getProjection();

		render::ImageGraphContext cx(m_screenRenderer);
		cx.associateTextureTargetSet(s_handleInputDepth, gbufferTargetSetId, 0);
		cx.associateTextureTargetSet(s_handleInputNormal, gbufferTargetSetId, 1);
		cx.setParams(ipd);

		m_ambientOcclusion->addPasses(renderGraph, rp, cx);
	}

	render::Clear clear;
	clear.mask = render::CfColor;
	clear.colors[0] = Color4f(1.0f, 1.0f, 1.0f, 1.0f);
	rp->setOutput(ambientOcclusionTargetSetId, clear);

	renderGraph.addPass(rp);
	return ambientOcclusionTargetSetId;
}

render::handle_t WorldRendererDeferred::setupCascadeShadowMapPass(
	const WorldRenderView& worldRenderView,
	const Entity* rootEntity,
	render::RenderGraph& renderGraph,
	render::handle_t outputTargetSetId,
	int32_t lightCascadeIndex,
	LightShaderData* lightShaderData
) const
{
	if (lightCascadeIndex < 0)
		return 0;

	const auto& shadowSettings = m_settings.shadowSettings[m_shadowsQuality];
	const UniformShadowProjection shadowProjection(shadowSettings.resolution);

	Matrix44 view = worldRenderView.getView();
	Matrix44 viewInverse = view.inverse();
	Frustum viewFrustum = worldRenderView.getViewFrustum();

	// Add cascading shadow map target.
	auto shadowMapCascadeTargetSetId = renderGraph.addTargetSet(m_shadowMapCascadeTargetSet);

	// Add cascading shadow map render pass.
	Ref< render::RenderPass > rp = new render::RenderPass(L"Shadow cascade");

	render::Clear clear;
	clear.mask = render::CfDepth;
	clear.depth = 1.0f;
	rp->setOutput(shadowMapCascadeTargetSetId, clear);

	rp->addBuild(
		[=](const render::RenderGraph& renderGraph, render::RenderContext* renderContext)
		{
			WorldBuildContext wc(
				m_entityRenderers,
				rootEntity,
				renderContext
			);

			const auto& light = m_lights[lightCascadeIndex];
			auto* lsd = &lightShaderData[lightCascadeIndex];

			for (int32_t slice = 0; slice < shadowSettings.cascadingSlices; ++slice)
			{
				Scalar zn(max(m_slicePositions[slice], m_settings.viewNearZ));
				Scalar zf(min(m_slicePositions[slice + 1], shadowSettings.farZ));

				// Create sliced view frustum.
				Frustum sliceViewFrustum = viewFrustum;
				sliceViewFrustum.setNearZ(zn);
				sliceViewFrustum.setFarZ(zf);

				// Calculate shadow map projection.
				Matrix44 shadowLightView;
				Matrix44 shadowLightProjection;
				Frustum shadowFrustum;

				shadowProjection.calculate(
					viewInverse,
					light.position,
					light.direction,
					sliceViewFrustum,
					shadowSettings.farZ,
					shadowSettings.quantizeProjection,
					shadowLightView,
					shadowLightProjection,
					shadowFrustum
				);

				// Render shadow map.
				WorldRenderView shadowRenderView;
				shadowRenderView.setProjection(shadowLightProjection);
				shadowRenderView.setView(shadowLightView, shadowLightView);
				shadowRenderView.setViewFrustum(shadowFrustum);
				shadowRenderView.setCullFrustum(shadowFrustum);
				shadowRenderView.setTimes(
					worldRenderView.getTime(),
					worldRenderView.getDeltaTime(),
					worldRenderView.getInterval()
				);

				// Set viewport to current cascade.
				auto svrb = renderContext->alloc< render::SetViewportRenderBlock >();
				svrb->viewport = render::Viewport(
					0,
					slice * shadowSettings.resolution,
					shadowSettings.resolution,
					shadowSettings.resolution,
					0.0f,
					1.0f
				);
				renderContext->enqueue(svrb);	

				// Render entities into shadow map.
				auto sharedParams = renderContext->alloc< render::ProgramParameters >();
				sharedParams->beginParameters(renderContext);
				sharedParams->setFloatParameter(s_handleTime, worldRenderView.getTime());
				sharedParams->setMatrixParameter(s_handleView, shadowLightView);
				sharedParams->setMatrixParameter(s_handleViewInverse, shadowLightView.inverse());
				sharedParams->setMatrixParameter(s_handleProjection, shadowLightProjection);
				sharedParams->endParameters(renderContext);

				WorldRenderPassDeferred shadowPass(
					s_techniqueShadow,
					sharedParams,
					shadowRenderView,
					IWorldRenderPass::PfNone,
					false,
					false,
					false
				);

				T_ASSERT(!renderContext->havePendingDraws());
				wc.build(shadowRenderView, shadowPass, rootEntity);
				wc.flush(shadowRenderView, shadowPass);
				renderContext->merge(render::RpAll);
			}
		}
	);

	renderGraph.addPass(rp);
	return shadowMapCascadeTargetSetId;
}

render::handle_t WorldRendererDeferred::setupAtlasShadowMapPass(
	const WorldRenderView& worldRenderView,
	const Entity* rootEntity,
	render::RenderGraph& renderGraph,
	render::handle_t outputTargetSetId,
	const StaticVector< int32_t, 16 >& lightAtlasIndices,
	LightShaderData* lightShaderData
) const
{
	if (lightAtlasIndices.empty())
		return 0;

	const auto shadowSettings = m_settings.shadowSettings[m_shadowsQuality];

	Matrix44 view = worldRenderView.getView();
	Matrix44 viewInverse = view.inverse();
	Frustum viewFrustum = worldRenderView.getViewFrustum();

	// Add atlas shadow map target.
	auto shadowMapAtlasTargetSetId = renderGraph.addTargetSet(m_shadowMapAtlasTargetSet);

	// Add atlas shadow map render pass.
	int32_t atlasIndex = 0;
	for (int32_t lightAtlasIndex : lightAtlasIndices)
	{
		Ref< render::RenderPass > rp = new render::RenderPass(L"Shadow atlas");

		render::Clear clear;
		clear.mask = render::CfDepth;
		clear.depth = 1.0f;
		rp->setOutput(shadowMapAtlasTargetSetId, clear);

		rp->addBuild(
			[=](const render::RenderGraph& renderGraph, render::RenderContext* renderContext)
			{
				WorldBuildContext wc(
					m_entityRenderers,
					rootEntity,
					renderContext
				);

				const auto& light = m_lights[lightAtlasIndex];
				auto* lsd = &lightShaderData[lightAtlasIndex];

				// Calculate shadow map projection.
				Matrix44 shadowLightView;
				Matrix44 shadowLightProjection;
				Frustum shadowFrustum;

				shadowFrustum.buildPerspective(
					light.radius,
					1.0f,
					0.1f,
					light.range
				);

				shadowLightProjection = perspectiveLh(light.radius, 1.0f, 0.1f, light.range);

				Vector4 lightAxisX, lightAxisY, lightAxisZ;
				lightAxisZ = -light.direction.xyz0().normalized();
				lightAxisX = cross(viewInverse.axisZ(), lightAxisZ).normalized();
				lightAxisY = cross(lightAxisX, lightAxisZ).normalized();

				shadowLightView = Matrix44(
					lightAxisX,
					lightAxisY,
					lightAxisZ,
					light.position.xyz1()
				);
				shadowLightView = shadowLightView.inverse();

				// Render shadow map.
				WorldRenderView shadowRenderView;
				shadowRenderView.setProjection(shadowLightProjection);
				shadowRenderView.setView(shadowLightView, shadowLightView);
				shadowRenderView.setViewFrustum(shadowFrustum);
				shadowRenderView.setCullFrustum(shadowFrustum);
				shadowRenderView.setTimes(
					worldRenderView.getTime(),
					worldRenderView.getDeltaTime(),
					worldRenderView.getInterval()
				);

				// Set viewport to light atlas slot.
				auto svrb = renderContext->alloc< render::SetViewportRenderBlock >();
				svrb->viewport = render::Viewport(
					(atlasIndex & 3) * 1024,
					(atlasIndex / 4) * 1024,
					1024,
					1024,
					0.0f,
					1.0f
				);
				renderContext->enqueue(svrb);	

				// Render entities into shadow map.
				auto sharedParams = renderContext->alloc< render::ProgramParameters >();
				sharedParams->beginParameters(renderContext);
				sharedParams->setFloatParameter(s_handleTime, worldRenderView.getTime());
				sharedParams->setMatrixParameter(s_handleView, shadowLightView);
				sharedParams->setMatrixParameter(s_handleViewInverse, shadowLightView.inverse());
				sharedParams->setMatrixParameter(s_handleProjection, shadowLightProjection);
				sharedParams->endParameters(renderContext);

				WorldRenderPassDeferred shadowPass(
					s_techniqueShadow,
					sharedParams,
					shadowRenderView,
					IWorldRenderPass::PfNone,
					false,
					false,
					false
				);

				T_ASSERT(!renderContext->havePendingDraws());
				wc.build(shadowRenderView, shadowPass, rootEntity);
				wc.flush(shadowRenderView, shadowPass);
				renderContext->merge(render::RpAll);

				// Write transposed matrix to shaders as shaders have row-major order.
				Matrix44 viewToLightSpace = shadowLightProjection * shadowLightView * viewInverse;
				Matrix44 vls = viewToLightSpace.transpose();
				vls.axisX().storeUnaligned(lsd->viewToLight0);
				vls.axisY().storeUnaligned(lsd->viewToLight1);
				vls.axisZ().storeUnaligned(lsd->viewToLight2);
				vls.translation().storeUnaligned(lsd->viewToLight3);

				// Write atlas coordinates to shaders.
				Vector4(
					(atlasIndex & 3) / 4.0f,
					(atlasIndex / 4) / 4.0f,
					1.0f / 4.0f,
					1.0f / 4.0f
				).storeUnaligned(lsd->atlasTransform);					
			}
		);

		renderGraph.addPass(rp);

		++atlasIndex;
	}

	return shadowMapAtlasTargetSetId;
}

void WorldRendererDeferred::setupTileDataPass(
	const WorldRenderView& worldRenderView,
	const Entity* rootEntity,
	render::RenderGraph& renderGraph,
	render::handle_t outputTargetSetId,
	TileShaderData* tileShaderData
) const
{
	const Frustum& viewFrustum = worldRenderView.getViewFrustum();

	// Update tile data.
	const float dx = 1.0f / 16.0f;
	const float dy = 1.0f / 16.0f;

	Vector4 nh = viewFrustum.corners[1] - viewFrustum.corners[0];
	Vector4 nv = viewFrustum.corners[3] - viewFrustum.corners[0];
	Vector4 fh = viewFrustum.corners[5] - viewFrustum.corners[4];
	Vector4 fv = viewFrustum.corners[7] - viewFrustum.corners[4];

	Frustum tileFrustum;
	for (int32_t y = 0; y < 16; ++y)
	{
		float fy = float(y) * dy;
		for (int32_t x = 0; x < 16; ++x)
		{
			float fx = float(x) * dx;

			Vector4 corners[] =
			{
				// Near
				viewFrustum.corners[0] + nh * Scalar(fx) + nv * Scalar(fy),				// l t
				viewFrustum.corners[0] + nh * Scalar(fx + dx) + nv * Scalar(fy),		// r t
				viewFrustum.corners[0] + nh * Scalar(fx + dx) + nv * Scalar(fy + dy),	// r b
				viewFrustum.corners[0] + nh * Scalar(fx) + nv * Scalar(fy + dy),		// l b
				// Far
				viewFrustum.corners[4] + fh * Scalar(fx) + fv * Scalar(fy),				// l t
				viewFrustum.corners[4] + fh * Scalar(fx + dx) + fv * Scalar(fy),		// r t
				viewFrustum.corners[4] + fh * Scalar(fx + dx) + fv * Scalar(fy + dy),	// r b
				viewFrustum.corners[4] + fh * Scalar(fx) + fv * Scalar(fy + dy)			// l b
			};

			tileFrustum.buildFromCorners(corners);

			int32_t count = 0;
			for (uint32_t i = 0; i < m_lights.size(); ++i)
			{
				const Light& light = m_lights[i];

				if (light.type == LtDirectional)
				{
					tileShaderData[x + y * 16].lights[count++] = float(i);
				}
				else if (light.type == LtPoint)
				{
					Vector4 lvp = worldRenderView.getView() * light.position.xyz1();
					if (tileFrustum.inside(lvp, Scalar(light.range)) != Frustum::IrOutside)
						tileShaderData[x + y * 16].lights[count++] = float(i);
				}
				else if (light.type == LtSpot)
				{
					tileShaderData[x + y * 16].lights[count++] = float(i);
				}

				if (count >= 4)
					break;
			}
			tileShaderData[x + y * 16].lightCount[0] = float(count);
		}
	}
}

render::handle_t WorldRendererDeferred::setupShadowMaskPass(
	const WorldRenderView& worldRenderView,
	const Entity* rootEntity,
	render::RenderGraph& renderGraph,
	render::handle_t outputTargetSetId,
	render::handle_t gbufferTargetSetId,
	render::handle_t shadowMapCascadeTargetSetId,
	int32_t lightCascadeIndex
) const
{
	if (m_shadowsQuality == QuDisabled || lightCascadeIndex < 0)
		return 0;

	const auto shadowSettings = m_settings.shadowSettings[m_shadowsQuality];
	const UniformShadowProjection shadowProjection(shadowSettings.resolution);
	const auto& light = m_lights[lightCascadeIndex];

	Matrix44 view = worldRenderView.getView();
	Matrix44 viewInverse = view.inverse();
	Frustum viewFrustum = worldRenderView.getViewFrustum();

	// Add screen space shadow mask target.
	render::RenderGraphTargetSetDesc rgtd = {};
	rgtd.count = 1;
	rgtd.width = 0;
	rgtd.height = 0;
	rgtd.createDepthStencil = false;
	rgtd.usingPrimaryDepthStencil = (m_sharedDepthStencil == nullptr) ? true : false;
	rgtd.ignoreStencil = true;
	rgtd.targets[0].colorFormat = render::TfR8;
	rgtd.referenceWidthDenom = m_shadowSettings.maskDenominator;
	rgtd.referenceHeightDenom = m_shadowSettings.maskDenominator;
	auto shadowMaskTargetSetId = renderGraph.addTargetSet(rgtd, m_sharedDepthStencil, outputTargetSetId);

	// Add screen space shadow mask render pass.
	Ref< render::RenderPass > rp = new render::RenderPass(L"Shadow mask");

	// Add sub-pass for each slice.
	for (int32_t slice = 0; slice < m_shadowSettings.cascadingSlices; ++slice)
	{
		Scalar zn(max(m_slicePositions[slice], m_settings.viewNearZ));
		Scalar zf(min(m_slicePositions[slice + 1], m_shadowSettings.farZ));

		// Create sliced view frustum.
		Frustum sliceViewFrustum = viewFrustum;
		sliceViewFrustum.setNearZ(zn);
		sliceViewFrustum.setFarZ(zf);

		// Calculate shadow map projection.
		Matrix44 shadowLightView;
		Matrix44 shadowLightProjection;
		Frustum shadowFrustum;

		shadowProjection.calculate(
			viewInverse,
			light.position,
			light.direction,
			sliceViewFrustum,
			shadowSettings.farZ,
			shadowSettings.quantizeProjection,
			shadowLightView,
			shadowLightProjection,
			shadowFrustum
		);

		render::ImageGraphParams ipd;
		ipd.viewFrustum = worldRenderView.getViewFrustum();
		ipd.viewToLight = shadowLightProjection * shadowLightView * viewInverse;
		ipd.projection = worldRenderView.getProjection();
		ipd.sliceCount = m_shadowSettings.cascadingSlices;
		ipd.sliceIndex = slice;
		ipd.sliceNearZ = zn;
		ipd.sliceFarZ = zf;
		ipd.shadowFarZ = m_shadowSettings.farZ;
		ipd.shadowMapBias = m_shadowSettings.bias + slice * m_shadowSettings.biasCoeff;
		ipd.shadowMapUvTransform = Vector4(
			0.0f, (float)slice / m_shadowSettings.cascadingSlices,
			1.0f, 1.0f / m_shadowSettings.cascadingSlices
		);
		ipd.deltaTime = 0.0f;
		ipd.time = 0.0f;

		render::ImageGraphContext cx(m_screenRenderer);
		cx.associateTextureTargetSetDepth(s_handleInputShadowMap, shadowMapCascadeTargetSetId);
		cx.associateTextureTargetSet(s_handleInputDepth, gbufferTargetSetId, 0);
		cx.associateTextureTargetSet(s_handleInputNormal, gbufferTargetSetId, 1);
		cx.setParams(ipd);

		m_shadowMaskProject->addPasses(renderGraph, rp, cx);
	}

	render::Clear clear;
	clear.mask = render::CfColor;
	clear.colors[0] = Color4f(1.0f, 1.0f, 1.0f, 1.0f);
	rp->setOutput(shadowMaskTargetSetId, clear);

	renderGraph.addPass(rp);
	return shadowMaskTargetSetId;
}

render::handle_t WorldRendererDeferred::setupReflectionsPass(
	const WorldRenderView& worldRenderView,
	const Entity* rootEntity,
	render::RenderGraph& renderGraph,
	render::handle_t outputTargetSetId,
	render::handle_t gbufferTargetSetId,
	render::handle_t visualTargetSetId
) const
{
	if (m_reflectionsQuality == QuDisabled)
		return 0;

	// Add Reflections target.
	render::RenderGraphTargetSetDesc rgtd = {};
	rgtd.count = 1;
	rgtd.createDepthStencil = false;
	rgtd.usingPrimaryDepthStencil = (m_sharedDepthStencil == nullptr) ? true : false;
	rgtd.ignoreStencil = true;
	rgtd.targets[0].colorFormat = render::TfR11G11B10F;
	rgtd.referenceWidthDenom = 1;
	rgtd.referenceHeightDenom = 1;
	auto reflectionsTargetSetId = renderGraph.addTargetSet(rgtd, m_sharedDepthStencil, outputTargetSetId);

	// Add Reflections render pass.
	Ref< render::RenderPass > rp = new render::RenderPass(L"Reflections");

	rp->addInput(gbufferTargetSetId);

	// if (m_reflectionsQuality >= QuHigh)
	// 	rp->addInput(visualTargetSetId, 0, true);

	render::Clear clear;
	clear.mask = render::CfColor;
	clear.colors[0] = Color4f(0.0f, 0.0f, 0.0f, 0.0f);
	rp->setOutput(reflectionsTargetSetId, clear);
	
	rp->addBuild(
		[=](const render::RenderGraph& renderGraph, render::RenderContext* renderContext)
		{
			WorldBuildContext wc(
				m_entityRenderers,
				rootEntity,
				renderContext
			);

			auto gbufferTargetSet = renderGraph.getTargetSet(gbufferTargetSetId);

			auto sharedParams = renderContext->alloc< render::ProgramParameters >();
			sharedParams->beginParameters(renderContext);
			sharedParams->setFloatParameter(s_handleTime, worldRenderView.getTime());
			sharedParams->setMatrixParameter(s_handleView, worldRenderView.getView());
			sharedParams->setMatrixParameter(s_handleViewInverse, worldRenderView.getView().inverse());
			sharedParams->setMatrixParameter(s_handleProjection, worldRenderView.getProjection());
			sharedParams->setTextureParameter(s_handleDepthMap, gbufferTargetSet->getColorTexture(0));
			sharedParams->setTextureParameter(s_handleNormalMap, gbufferTargetSet->getColorTexture(1));
			sharedParams->setTextureParameter(s_handleMiscMap, gbufferTargetSet->getColorTexture(2));
			sharedParams->setTextureParameter(s_handleColorMap, gbufferTargetSet->getColorTexture(3));
			sharedParams->endParameters(renderContext);

			WorldRenderPassDeferred reflectionsPass(
				s_techniqueReflectionWrite,
				sharedParams,
				worldRenderView,
				IWorldRenderPass::PfNone,
				false,
				false,
				false
			);

			T_ASSERT(!renderContext->havePendingDraws());
			wc.build(worldRenderView, reflectionsPass, rootEntity);
			wc.flush(worldRenderView, reflectionsPass);
			renderContext->merge(render::RpAll);

			// Render screenspace reflections.
			// if (m_reflectionsQuality >= QuHigh)
			// {
				//	auto visualTargetSet = renderGraph.getTargetSet(visualTargetSetId);

				// auto lrb = renderContext->alloc< render::LambdaRenderBlock >(L"Reflections");
				// lrb->lambda = [=](render::IRenderView* renderView)
				// {
				// 	Scalar p11 = projection.get(0, 0);
				// 	Scalar p22 = projection.get(1, 1);

				// 	m_reflectionShader->setVectorParameter(s_handleMagicCoeffs, Vector4(1.0f / p11, 1.0f / p22, 0.0f, 0.0f));
				// 	m_reflectionShader->setTextureParameter(s_handleScreenMap, visualTargetSet->getColorTexture(0));

				// 	m_reflectionShader->draw(renderView, m_vertexBufferQuad, 0, m_primitivesQuad);
				// };
				// renderContext->enqueue(lrb);
			// }
		}
	);

	renderGraph.addPass(rp);
	return reflectionsTargetSetId;
}

render::handle_t WorldRendererDeferred::setupVisualPass(
	const WorldRenderView& worldRenderView,
	const Entity* rootEntity,
	render::RenderGraph& renderGraph,
	render::handle_t outputTargetSetId,
	render::handle_t gbufferTargetSetId,
	render::handle_t ambientOcclusionTargetSetId,
	render::handle_t reflectionsTargetSetId,
	render::handle_t shadowMaskTargetSetId,
	render::handle_t shadowMapAtlasTargetSetId,
	int32_t frame
) const
{
	const bool shadowsEnable = (bool)(m_shadowsQuality != QuDisabled);
	int32_t lightCount = (int32_t)m_lights.size();

	// Add visual[0] target set.
	render::RenderGraphTargetSetDesc rgtd = {};
	rgtd.count = 1;
	rgtd.createDepthStencil = false;
	rgtd.usingPrimaryDepthStencil = (m_sharedDepthStencil == nullptr) ? true : false;
	rgtd.targets[0].colorFormat = render::TfR11G11B10F;
	rgtd.referenceWidthDenom = 1;
	rgtd.referenceHeightDenom = 1;
	auto visualTargetSetId = renderGraph.addTargetSet(rgtd, m_sharedDepthStencil, outputTargetSetId);

	// Add visual[0] render pass.
	Ref< render::RenderPass > rp = new render::RenderPass(L"Visual");
	rp->addInput(gbufferTargetSetId);
	rp->addInput(ambientOcclusionTargetSetId);
	rp->addInput(reflectionsTargetSetId);

	if (shadowsEnable)
	{
		rp->addInput(shadowMaskTargetSetId);
		rp->addInput(shadowMapAtlasTargetSetId, true);
	}

	render::Clear clear;
	clear.mask = render::CfColor;
	clear.colors[0] = Color4f(0.0f, 0.0f, 0.0f, 1.0f);
	rp->setOutput(visualTargetSetId, clear);

	rp->addBuild(
		[=](const render::RenderGraph& renderGraph, render::RenderContext* renderContext)
		{
			WorldBuildContext wc(
				m_entityRenderers,
				rootEntity,
				renderContext
			);

			auto gbufferTargetSet = renderGraph.getTargetSet(gbufferTargetSetId);
			auto ambientOcclusionTargetSet = renderGraph.getTargetSet(ambientOcclusionTargetSetId);
			auto reflectionsTargetSet = renderGraph.getTargetSet(reflectionsTargetSetId);
			auto shadowMaskTargetSet = renderGraph.getTargetSet(shadowMaskTargetSetId);
			auto shadowAtlasTargetSet = renderGraph.getTargetSet(shadowMapAtlasTargetSetId);

			const auto& view = worldRenderView.getView();
			const auto& projection = worldRenderView.getProjection();

			Scalar p11 = projection.get(0, 0);
			Scalar p22 = projection.get(1, 1);

			auto sharedParams = renderContext->alloc< render::ProgramParameters >();
			sharedParams->beginParameters(renderContext);
			sharedParams->setFloatParameter(s_handleTime, worldRenderView.getTime());
			sharedParams->setFloatParameter(s_handleLightCount, (float)lightCount);
			sharedParams->setVectorParameter(s_handleMagicCoeffs, Vector4(1.0f / p11, 1.0f / p22, 0.0f, 0.0f));
			sharedParams->setVectorParameter(s_handleFogDistanceAndDensity, m_fogDistanceAndDensity);
			sharedParams->setVectorParameter(s_handleFogColor, m_fogColor);
			sharedParams->setMatrixParameter(s_handleView, view);
			sharedParams->setMatrixParameter(s_handleViewInverse, view.inverse());
			sharedParams->setMatrixParameter(s_handleProjection, projection);
			sharedParams->setTextureParameter(s_handleDepthMap, gbufferTargetSet->getColorTexture(0));
			sharedParams->setTextureParameter(s_handleNormalMap, gbufferTargetSet->getColorTexture(1));
			sharedParams->setTextureParameter(s_handleMiscMap, gbufferTargetSet->getColorTexture(2));
			sharedParams->setTextureParameter(s_handleColorMap, gbufferTargetSet->getColorTexture(3));
			sharedParams->setTextureParameter(s_handleOcclusionMap, ambientOcclusionTargetSet->getColorTexture(0));
			if (shadowMaskTargetSet)
				sharedParams->setTextureParameter(s_handleShadowMask, shadowMaskTargetSet->getColorTexture(0));
			if (shadowAtlasTargetSet)
				sharedParams->setTextureParameter(s_handleShadowMapAtlas, shadowAtlasTargetSet->getDepthTexture());
			if (reflectionsTargetSet)
				sharedParams->setTextureParameter(s_handleReflectionMap, reflectionsTargetSet->getColorTexture(0));
			sharedParams->setStructBufferParameter(s_handleLightSBuffer, m_frames[frame].lightSBuffer);
			sharedParams->setStructBufferParameter(s_handleTileSBuffer, m_frames[frame].tileSBuffer);
			if (m_irradianceGrid)
			{
				const auto size = m_irradianceGrid->getSize();
				sharedParams->setVectorParameter(s_handleIrradianceGridSize, Vector4((float)size[0], (float)size[1], (float)size[2], 0.0f));
				sharedParams->setVectorParameter(s_handleIrradianceGridBoundsMin, m_irradianceGrid->getBoundingBox().mn);
				sharedParams->setVectorParameter(s_handleIrradianceGridBoundsMax, m_irradianceGrid->getBoundingBox().mx);
				sharedParams->setStructBufferParameter(s_handleIrradianceGridSBuffer, m_irradianceGrid->getBuffer());
			}
			sharedParams->endParameters(renderContext);

			// Irradiance
			WorldRenderPassDeferred irradiancePass(
				s_techniqueIrradianceWrite,
				sharedParams,
				worldRenderView,
				IWorldRenderPass::PfNone,
				false,
				false,
				(bool)(m_irradianceGrid != nullptr)
			);

			T_ASSERT(!renderContext->havePendingDraws());
			wc.build(worldRenderView, irradiancePass, rootEntity);
			wc.flush(worldRenderView, irradiancePass);
			renderContext->merge(render::RpAll);

			// Analytical lights; resolve with gbuffer.
			render::Shader::Permutation perm;
			m_lightShader->setCombination(s_handleShadowEnable, (bool)(shadowMaskTargetSet != nullptr), perm);
			m_lightShader->setCombination(s_handleReflectionsEnable, (bool)(reflectionsTargetSet != nullptr), perm);
			m_lightShader->setCombination(s_handleIrradianceEnable, (bool)(m_irradianceGrid != nullptr), perm);
			m_screenRenderer->draw(renderContext, m_lightShader, perm, sharedParams);

			// Module with fog.
			if (dot4(m_fogDistanceAndDensity, Vector4(0.0f, 0.0f, 1.0f, 1.0f)) > FUZZY_EPSILON)
				m_screenRenderer->draw(renderContext, m_fogShader, sharedParams);

			// Forward visuals; not included in GBuffer.
			WorldRenderPassDeferred deferredColorPass(
				s_techniqueDeferredColor,
				sharedParams,
				worldRenderView,
				IWorldRenderPass::PfLast,
				m_settings.fog,
				(bool)(gbufferTargetSet->getColorTexture(0) != nullptr),
				(bool)(m_irradianceGrid != nullptr)
			);

			T_ASSERT(!renderContext->havePendingDraws());
			wc.build(worldRenderView, deferredColorPass, rootEntity);
			wc.flush(worldRenderView, deferredColorPass);
			renderContext->merge(render::RpAll);
		}
	);

	renderGraph.addPass(rp);
	return visualTargetSetId;
}

void WorldRendererDeferred::setupProcessPass(
	const WorldRenderView& worldRenderView,
	const Entity* rootEntity,
	render::RenderGraph& renderGraph,
	render::handle_t outputTargetSetId,
	render::handle_t gbufferTargetSetId,
	render::handle_t velocityTargetSetId,
	render::handle_t visualTargetSetId
) const
{
	render::ImageGraphParams ipd;
	ipd.viewFrustum = worldRenderView.getViewFrustum();
	ipd.viewToLight = Matrix44::identity();
	ipd.view = worldRenderView.getView();
	ipd.projection = worldRenderView.getProjection();
	ipd.deltaTime = 1.0f / 60.0f;				

	render::ImageGraphContext cx(m_screenRenderer);
	cx.associateTextureTargetSet(s_handleInputColor, visualTargetSetId, 0);
	cx.associateTextureTargetSet(s_handleInputDepth, gbufferTargetSetId, 0);
	cx.associateTextureTargetSet(s_handleInputNormal, gbufferTargetSetId, 1);
	cx.associateTextureTargetSet(s_handleInputVelocity, velocityTargetSetId, 0);
	cx.setParams(ipd);

	StaticVector< render::ImageGraph*, 5 > processes;
	if (m_motionBlur)
		processes.push_back(m_motionBlur);
	if (m_toneMap)
		processes.push_back(m_toneMap);
	if (m_visual)
		processes.push_back(m_visual);
	if (m_gammaCorrection)
		processes.push_back(m_gammaCorrection);
	if (m_antiAlias)
		processes.push_back(m_antiAlias);

	render::handle_t intermediateTargetSetId = 0;
	for (size_t i = 0; i < processes.size(); ++i)
	{
		auto process = processes[i];
		bool next = (bool)((i + 1) < processes.size());

		Ref< render::RenderPass > rp = new render::RenderPass(L"Process");

		if (next)
		{
			render::RenderGraphTargetSetDesc rgtd = {};
			rgtd.count = 1;
			rgtd.createDepthStencil = false;
			rgtd.usingPrimaryDepthStencil = (m_sharedDepthStencil == nullptr) ? true : false;
			rgtd.targets[0].colorFormat = render::TfR11G11B10F;
			rgtd.referenceWidthDenom = 1;
			rgtd.referenceHeightDenom = 1;
			intermediateTargetSetId = renderGraph.addTargetSet(rgtd, m_sharedDepthStencil, outputTargetSetId);

			rp->setOutput(intermediateTargetSetId);
		}
		else
		{
			render::Clear cl;
			cl.mask = render::CfColor | render::CfDepth;
			cl.colors[0] = Color4f(0.0f, 0.0f, 0.0f, 0.0f);
			cl.depth = 1.0f;
			rp->setOutput(outputTargetSetId, cl);
		}

		process->addPasses(renderGraph, rp, cx);

		if (next)
			cx.associateTextureTargetSet(s_handleInputColor, intermediateTargetSetId, 0);

		renderGraph.addPass(rp);
	}
}

	}
}
