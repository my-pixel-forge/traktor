/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#include <limits>
#include "Core/Math/Aabb3.h"
#include "Core/Misc/SafeDestroy.h"
#include "Render/IRenderSystem.h"
#include "Render/IRenderView.h"
#include "Render/Shader.h"
#include "Render/VertexBuffer.h"
#include "Render/VertexElement.h"
#include "Resource/IResourceManager.h"
#include "World/Deferred/LightRendererDeferred.h"

namespace traktor
{
	namespace world
	{
		namespace
		{

const resource::Id< render::Shader > c_lightDirectionalShader(Guid(L"{F4F7C11E-BCB3-1045-9C78-1F0658D34229}"));
const resource::Id< render::Shader > c_lightDirectionalsShader(Guid(L"{149BCECD-8CD9-491B-BD8A-21ABCA3C0940}"));
const resource::Id< render::Shader > c_lightPointShader(Guid(L"{E00C6B91-3266-A044-BE9E-56C8E70CA539}"));
const resource::Id< render::Shader > c_lightPointsShader(Guid(L"{7ED68109-2926-41E3-8899-16F1593CE78D}"));
const resource::Id< render::Shader > c_lightSpotShader(Guid(L"{3FD07AEE-3FB5-C84D-87E1-3F9A2F0CAD5E}"));
const resource::Id< render::Shader > c_lightSpotsShader(Guid(L"{8EB14BEA-97B1-4168-8937-AE3FDE4C6B91}"));
const resource::Id< render::Shader > c_lightProbeShader(Guid(L"{673434A8-2663-9141-8E23-91A2F38A7B69}"));
const resource::Id< render::Shader > c_finalColorShader(Guid(L"{AA7A6074-6CFC-1947-857C-8D9903D0880F}"));
const resource::Id< render::Shader > c_reflectionShader(Guid(L"{F04EEA34-85E0-974F-BE97-79D24C6ACFBD}"));
const resource::Id< render::Shader > c_fogShader(Guid(L"{9453D74C-76C4-8748-9A5B-9E3D6D4F9406}"));

const float c_pointLightScreenAreaThresholdDim = 4.0f;
const float c_pointLightScreenAreaThreshold = 4.0f * (c_pointLightScreenAreaThresholdDim * c_pointLightScreenAreaThresholdDim) / (1280.0f * 720.0f);

render::handle_t s_handleTime;
render::handle_t s_handleShadowEnable;
render::handle_t s_handleCloudShadowEnable;
render::handle_t s_handleTraceReflections;
render::handle_t s_handleExtent;
render::handle_t s_handleProjection;
render::handle_t s_handleView;
render::handle_t s_handleViewInverse;
render::handle_t s_handleMagicCoeffs;
render::handle_t s_handleScreenMap;
render::handle_t s_handleReflectionMap;
render::handle_t s_handleDepthMap;
render::handle_t s_handleNormalMap;
render::handle_t s_handleMiscMap;
render::handle_t s_handleColorMap;
render::handle_t s_handleLightMap;
render::handle_t s_handleShadowMaskSize;
render::handle_t s_handleShadowMask;
render::handle_t s_handleCloudShadow;
render::handle_t s_handleProbeDiffuseEnable;
render::handle_t s_handleProbeDiffuse;
render::handle_t s_handleLightPosition;
render::handle_t s_handleLightPositionAndRadius;
render::handle_t s_handleLightDirectionAndRange;
render::handle_t s_handleLightSunColor;
render::handle_t s_handleLightBaseColor;
render::handle_t s_handleLightShadowColor;
render::handle_t s_handleFogDistanceAndDensity;
render::handle_t s_handleFogColor;

#pragma pack(1)
struct LightVertex
{
	float pos[2];
	float texCoord[2];
};
#pragma pack()

		}

T_IMPLEMENT_RTTI_CLASS(L"traktor.world.LightRendererDeferred", LightRendererDeferred, Object)

LightRendererDeferred::LightRendererDeferred()
{
	s_handleTime = render::getParameterHandle(L"World_Time");
	s_handleShadowEnable = render::getParameterHandle(L"World_ShadowEnable");
	s_handleCloudShadowEnable = render::getParameterHandle(L"World_CloudShadowEnable");
	s_handleTraceReflections = render::getParameterHandle(L"World_TraceReflections");
	s_handleExtent = render::getParameterHandle(L"World_Extent");
	s_handleProjection = render::getParameterHandle(L"World_Projection");
	s_handleView = render::getParameterHandle(L"World_View");
	s_handleViewInverse = render::getParameterHandle(L"World_ViewInverse");
	s_handleMagicCoeffs = render::getParameterHandle(L"World_MagicCoeffs");
	s_handleScreenMap = render::getParameterHandle(L"World_ScreenMap");
	s_handleReflectionMap = render::getParameterHandle(L"World_ReflectionMap");
	s_handleDepthMap = render::getParameterHandle(L"World_DepthMap");
	s_handleNormalMap = render::getParameterHandle(L"World_NormalMap");
	s_handleMiscMap = render::getParameterHandle(L"World_MiscMap");
	s_handleColorMap = render::getParameterHandle(L"World_ColorMap");
	s_handleLightMap = render::getParameterHandle(L"World_LightMap");
	s_handleShadowMaskSize = render::getParameterHandle(L"World_ShadowMaskSize");
	s_handleShadowMask = render::getParameterHandle(L"World_ShadowMask");
	s_handleCloudShadow = render::getParameterHandle(L"World_CloudShadow");
	s_handleProbeDiffuseEnable = render::getParameterHandle(L"World_ProbeDiffuseEnable");
	s_handleProbeDiffuse = render::getParameterHandle(L"World_ProbeDiffuse");
	s_handleLightPosition = render::getParameterHandle(L"World_LightPosition");
	s_handleLightPositionAndRadius = render::getParameterHandle(L"World_LightPositionAndRadius");
	s_handleLightDirectionAndRange = render::getParameterHandle(L"World_LightDirectionAndRange");
	s_handleLightSunColor = render::getParameterHandle(L"World_LightSunColor");
	s_handleLightBaseColor = render::getParameterHandle(L"World_LightBaseColor");
	s_handleLightShadowColor = render::getParameterHandle(L"World_LightShadowColor");
	s_handleFogDistanceAndDensity = render::getParameterHandle(L"World_FogDistanceAndDensity");
	s_handleFogColor = render::getParameterHandle(L"World_FogColor");
}

bool LightRendererDeferred::create(
	resource::IResourceManager* resourceManager,
	render::IRenderSystem* renderSystem
)
{
	if (!resourceManager->bind(c_lightDirectionalShader, m_lightDirectionalShader))
		return false;
	if (!resourceManager->bind(c_lightDirectionalsShader, m_lightDirectionalsShader))
		return false;
	if (!resourceManager->bind(c_lightPointShader, m_lightPointShader))
		return false;
	if (!resourceManager->bind(c_lightPointsShader, m_lightPointsShader))
		return false;
	if (!resourceManager->bind(c_lightSpotShader, m_lightSpotShader))
		return false;
	if (!resourceManager->bind(c_lightSpotsShader, m_lightSpotsShader))
		return false;
	if (!resourceManager->bind(c_lightProbeShader, m_lightProbeShader))
		return false;
	if (!resourceManager->bind(c_finalColorShader, m_finalColorShader))
		return false;
	if (!resourceManager->bind(c_reflectionShader, m_reflectionShader))
		return false;
	if (!resourceManager->bind(c_fogShader, m_fogShader))
		return false;

	AlignedVector< render::VertexElement > vertexElements;
	vertexElements.push_back(render::VertexElement(render::DuPosition, render::DtFloat2, offsetof(LightVertex, pos)));
	vertexElements.push_back(render::VertexElement(render::DuCustom, render::DtFloat2, offsetof(LightVertex, texCoord)));

	m_vertexBufferQuad = renderSystem->createVertexBuffer(vertexElements, 6 * sizeof(LightVertex), false);
	if (!m_vertexBufferQuad)
		return false;

	LightVertex* vertex = reinterpret_cast< LightVertex* >(m_vertexBufferQuad->lock());
	T_ASSERT (vertex);

	vertex[0].pos[0] = -1.0f; vertex[0].pos[1] =  1.0f; vertex[0].texCoord[0] = 0.0f; vertex[0].texCoord[1] = 0.0f;
	vertex[1].pos[0] =  1.0f; vertex[1].pos[1] =  1.0f; vertex[1].texCoord[0] = 1.0f; vertex[1].texCoord[1] = 0.0f;
	vertex[2].pos[0] =  1.0f; vertex[2].pos[1] = -1.0f; vertex[2].texCoord[0] = 1.0f; vertex[2].texCoord[1] = 1.0f;

	vertex[3].pos[0] = -1.0f; vertex[3].pos[1] =  1.0f; vertex[3].texCoord[0] = 0.0f; vertex[3].texCoord[1] = 0.0f;
	vertex[4].pos[0] =  1.0f; vertex[4].pos[1] = -1.0f; vertex[4].texCoord[0] = 1.0f; vertex[4].texCoord[1] = 1.0f;
	vertex[5].pos[0] = -1.0f; vertex[5].pos[1] = -1.0f; vertex[5].texCoord[0] = 0.0f; vertex[5].texCoord[1] = 1.0f;

	m_vertexBufferQuad->unlock();

	m_primitivesQuad.setNonIndexed(render::PtTriangles, 0, 2);

	return true;
}

void LightRendererDeferred::destroy()
{
	safeDestroy(m_vertexBufferQuad);
}

void LightRendererDeferred::renderLight(
	render::IRenderView* renderView,
	float time,
	const Matrix44& projection,
	const Matrix44& view,
	const Light& light,
	render::ITexture* depthMap,
	render::ITexture* normalMap,
	render::ITexture* miscMap,
	render::ITexture* colorMap,
	float shadowMaskSize,
	render::ITexture* shadowMask
)
{
	Scalar p11 = projection.get(0, 0);
	Scalar p22 = projection.get(1, 1);

	Vector4 sunColorAndIntensity = light.sunColor;
	sunColorAndIntensity.set(3, dot3(light.sunColor, Vector4(0.2125f, 0.7154f, 0.0721f, 0.0f)));

	if (light.type == LtDirectional)
	{
		Vector4 lightDirectionAndRange = view * light.direction.xyz0() + Vector4(0.0f, 0.0f, 0.0f, light.range);

		m_lightDirectionalShader->setCombination(s_handleShadowEnable, shadowMask != 0);
		m_lightDirectionalShader->setCombination(s_handleCloudShadowEnable, light.cloudShadow != 0);
		m_lightDirectionalShader->setFloatParameter(s_handleTime, time);
		m_lightDirectionalShader->setFloatParameter(s_handleShadowMaskSize, 0.5f / shadowMaskSize);
		m_lightDirectionalShader->setVectorParameter(s_handleMagicCoeffs, Vector4(1.0f / p11, 1.0f / p22, 0.0f, 0.0f));
		m_lightDirectionalShader->setMatrixParameter(s_handleViewInverse, view.inverse());
		m_lightDirectionalShader->setTextureParameter(s_handleShadowMask, shadowMask);
		m_lightDirectionalShader->setTextureParameter(s_handleCloudShadow, light.cloudShadow);
		m_lightDirectionalShader->setTextureParameter(s_handleDepthMap, depthMap);
		m_lightDirectionalShader->setTextureParameter(s_handleNormalMap, normalMap);
		m_lightDirectionalShader->setTextureParameter(s_handleMiscMap, miscMap);
		m_lightDirectionalShader->setTextureParameter(s_handleColorMap, colorMap);
		m_lightDirectionalShader->setVectorParameter(s_handleLightDirectionAndRange, lightDirectionAndRange);
		m_lightDirectionalShader->setVectorParameter(s_handleLightSunColor, sunColorAndIntensity);
		m_lightDirectionalShader->setVectorParameter(s_handleLightBaseColor, light.baseColor);
		m_lightDirectionalShader->setVectorParameter(s_handleLightShadowColor, light.shadowColor);

		m_lightDirectionalShader->draw(renderView, m_vertexBufferQuad, 0, m_primitivesQuad);
	}
	else if (light.type == LtPoint)
	{
		Vector4 lightPosition = view * light.position.xyz1();
		Vector4 lightDirectionAndRange = view * light.direction.xyz0() + Vector4(0.0f, 0.0f, 0.0f, light.range);

		// Calculate screen bounding box of light.
		Aabb3 lightBoundingBox;
		lightBoundingBox.contain(light.position.xyz1(), light.range);

		Vector4 extents[8];
		lightBoundingBox.getExtents(extents);

		Vector4 mn(
			std::numeric_limits< float >::max(),
			std::numeric_limits< float >::max(),
			std::numeric_limits< float >::max(),
			std::numeric_limits< float >::max()
		);
		Vector4 mx(
			-std::numeric_limits< float >::max(),
			-std::numeric_limits< float >::max(),
			-std::numeric_limits< float >::max(),
			-std::numeric_limits< float >::max()
		);

		Matrix44 viewProj = projection * view;
		for (int i = 0; i < sizeof_array(extents); ++i)
		{
			Vector4 p = viewProj * extents[i];
			if (p.w() <= 0.0f)
			{
				// Bounding box clipped to view plane; clip edge with view plane.
				mn = Vector4(-1.0f, -1.0f, 0.0f, 0.0f);
				mx = Vector4(1.0f, 1.0f, 0.0f, 0.0f);
				break;
			}

			// Homogeneous divide.
			p /= p.w();

			// Track screen space extents.
			mn = min(mn, p);
			mx = max(mx, p);
		}

		// Ensure we're visible.
		if (mn.x() > 1.0f || mn.y() > 1.0f || mx.x() < -1.0f || mx.y() < -1.0f)
			return;

		// Calculate area of light quad; do before clipping to screen edges as
		// we don't want to accidentally cull near lights which are close to the edge.
		float area = (mx.x() - mn.x()) * (mx.y() - mn.y());
		if (area < c_pointLightScreenAreaThreshold)
			return;

		// Clip quad to screen edges.
		mn = max(mn, Vector4(-1.0f, -1.0f, 0.0f, 0.0f));
		mx = min(mx, Vector4(1.0f, 1.0f, 0.0f, 0.0f));

		// Render quad primitive.
		m_lightPointShader->setFloatParameter(s_handleTime, time);
		m_lightPointShader->setVectorParameter(s_handleExtent, Vector4(mn.x(), mn.y(), mx.x(), mx.y()));
		m_lightPointShader->setVectorParameter(s_handleMagicCoeffs, Vector4(1.0f / p11, 1.0f / p22, 0.0f, 0.0f));
		m_lightPointShader->setMatrixParameter(s_handleViewInverse, view.inverse());
		m_lightPointShader->setTextureParameter(s_handleDepthMap, depthMap);
		m_lightPointShader->setTextureParameter(s_handleNormalMap, normalMap);
		m_lightPointShader->setTextureParameter(s_handleMiscMap, miscMap);
		m_lightPointShader->setTextureParameter(s_handleColorMap, colorMap);
		m_lightPointShader->setVectorParameter(s_handleLightPosition, lightPosition);
		m_lightPointShader->setVectorParameter(s_handleLightDirectionAndRange, lightDirectionAndRange);
		m_lightPointShader->setVectorParameter(s_handleLightSunColor, sunColorAndIntensity);
		m_lightPointShader->setVectorParameter(s_handleLightBaseColor, light.baseColor);
		m_lightPointShader->setVectorParameter(s_handleLightShadowColor, light.shadowColor);

		m_lightPointShader->draw(renderView, m_vertexBufferQuad, 0, m_primitivesQuad);
	}
	else if (light.type == LtSpot)
	{
		Vector4 lightPositionAndRadius = (view * light.position.xyz1()).xyz0() + Vector4(0.0f, 0.0f, 0.0f, light.radius);
		Vector4 lightDirectionAndRange = view * light.direction.xyz0() + Vector4(0.0f, 0.0f, 0.0f, light.range);

		// Calculate screen bounding box of light.
		Aabb3 lightBoundingBox;
		lightBoundingBox.contain(light.position.xyz1(), light.range);

		Vector4 extents[8];
		lightBoundingBox.getExtents(extents);

		Vector4 mn(
			std::numeric_limits< float >::max(),
			std::numeric_limits< float >::max(),
			std::numeric_limits< float >::max(),
			std::numeric_limits< float >::max()
		);
		Vector4 mx(
			-std::numeric_limits< float >::max(),
			-std::numeric_limits< float >::max(),
			-std::numeric_limits< float >::max(),
			-std::numeric_limits< float >::max()
		);

		Matrix44 viewProj = projection * view;
		for (int i = 0; i < sizeof_array(extents); ++i)
		{
			Vector4 p = viewProj * extents[i];
			if (p.w() <= 0.0f)
			{
				// Bounding box clipped to view plane; clip edge with view plane.
				mn = Vector4(-1.0f, -1.0f, 0.0f, 0.0f);
				mx = Vector4(1.0f, 1.0f, 0.0f, 0.0f);
				break;
			}

			// Homogeneous divide.
			p /= p.w();

			// Track screen space extents.
			mn = min(mn, p);
			mx = max(mx, p);
		}

		// Ensure we're visible.
		if (mn.x() > 1.0f || mn.y() > 1.0f || mx.x() < -1.0f || mx.y() < -1.0f)
			return;

		// Calculate area of light quad; do before clipping to screen edges as
		// we don't want to accidentally cull near lights which are close to the edge.
		float area = (mx.x() - mn.x()) * (mx.y() - mn.y());
		if (area < c_pointLightScreenAreaThreshold)
			return;

		// Clip quad to screen edges.
		mn = max(mn, Vector4(-1.0f, -1.0f, 0.0f, 0.0f));
		mx = min(mx, Vector4(1.0f, 1.0f, 0.0f, 0.0f));

		// Render quad primitive.
		m_lightSpotShader->setCombination(s_handleShadowEnable, shadowMask != 0);
		m_lightSpotShader->setFloatParameter(s_handleTime, time);
		m_lightSpotShader->setFloatParameter(s_handleShadowMaskSize, 0.5f / shadowMaskSize);
		m_lightSpotShader->setTextureParameter(s_handleShadowMask, shadowMask);
		m_lightSpotShader->setVectorParameter(s_handleExtent, Vector4(mn.x(), mn.y(), mx.x(), mx.y()));
		m_lightSpotShader->setVectorParameter(s_handleMagicCoeffs, Vector4(1.0f / p11, 1.0f / p22, 0.0f, 0.0f));
		m_lightSpotShader->setMatrixParameter(s_handleViewInverse, view.inverse());
		m_lightSpotShader->setTextureParameter(s_handleDepthMap, depthMap);
		m_lightSpotShader->setTextureParameter(s_handleNormalMap, normalMap);
		m_lightSpotShader->setTextureParameter(s_handleMiscMap, miscMap);
		m_lightSpotShader->setTextureParameter(s_handleColorMap, colorMap);
		m_lightSpotShader->setVectorParameter(s_handleLightPositionAndRadius, lightPositionAndRadius);
		m_lightSpotShader->setVectorParameter(s_handleLightDirectionAndRange, lightDirectionAndRange);
		m_lightSpotShader->setVectorParameter(s_handleLightSunColor, sunColorAndIntensity);
		m_lightSpotShader->setVectorParameter(s_handleLightBaseColor, light.baseColor);
		m_lightSpotShader->setVectorParameter(s_handleLightShadowColor, light.shadowColor);

		m_lightSpotShader->draw(renderView, m_vertexBufferQuad, 0, m_primitivesQuad);
	}
	else if (light.type == LtProbe)
	{
		Vector4 lightDirectionAndRange = view * light.direction.xyz0() + Vector4(0.0f, 0.0f, 0.0f, light.range);

		m_lightProbeShader->setCombination(s_handleShadowEnable, shadowMask != 0);
		m_lightProbeShader->setFloatParameter(s_handleTime, time);
		m_lightProbeShader->setFloatParameter(s_handleShadowMaskSize, 0.5f / shadowMaskSize);
		m_lightProbeShader->setVectorParameter(s_handleMagicCoeffs, Vector4(1.0f / p11, 1.0f / p22, 0.0f, 0.0f));
		m_lightProbeShader->setMatrixParameter(s_handleViewInverse, view.inverse());
		m_lightProbeShader->setTextureParameter(s_handleShadowMask, shadowMask);
		m_lightProbeShader->setTextureParameter(s_handleProbeDiffuse, light.probeDiffuse);
		m_lightProbeShader->setTextureParameter(s_handleDepthMap, depthMap);
		m_lightProbeShader->setTextureParameter(s_handleNormalMap, normalMap);
		m_lightProbeShader->setTextureParameter(s_handleMiscMap, miscMap);
		m_lightProbeShader->setTextureParameter(s_handleColorMap, colorMap);
		m_lightProbeShader->setVectorParameter(s_handleLightDirectionAndRange, lightDirectionAndRange);
		m_lightProbeShader->setVectorParameter(s_handleLightSunColor, sunColorAndIntensity);
		m_lightProbeShader->setVectorParameter(s_handleLightShadowColor, light.shadowColor);
		m_lightProbeShader->draw(renderView, m_vertexBufferQuad, 0, m_primitivesQuad);
	}
}

void LightRendererDeferred::renderNonShadowLights(
	render::IRenderView* renderView,
	float time,
	const Matrix44& projection,
	const Matrix44& view,
	const AlignedVector< Light >& lights,
	render::ITexture* depthMap,
	render::ITexture* normalMap,
	render::ITexture* miscMap,
	render::ITexture* colorMap
)
{
	Scalar p11 = projection.get(0, 0);
	Scalar p22 = projection.get(1, 1);

	// Gather non-shadow lights by type.
	AlignedVector< uint32_t > directional;
	AlignedVector< uint32_t > point;
	AlignedVector< uint32_t > spot;

	for (uint32_t i = 0; i < lights.size(); ++i)
	{
		const Light& light = lights[i];
		if (light.castShadow)
			continue;

		if (light.cloudShadow == nullptr)
		{
			if (light.type == LtDirectional)
			{
				directional.push_back(i);
				continue;
			}
			else if (light.type == LtPoint)
			{
				point.push_back(i);
				continue;
			}
			else if (light.type == LtSpot)
			{
				spot.push_back(i);
				continue;
			}
		}

		// Unbatchable light; render one by one.
		renderLight(
			renderView,
			time,
			projection,
			view,
			light,
			depthMap,
			normalMap,
			miscMap,
			colorMap,
			0.0f,
			nullptr
		);
}

	// Render directional lights.
	for (uint32_t i = 0; i < directional.size(); i += 4)
	{
		uint32_t n = std::min< uint32_t >(directional.size() - i, 4);
		T_ASSERT(n > 0);

		Vector4 lightDirectionAndRange[4];
		Vector4 sunColorAndIntensity[4];
		Vector4 baseColor[4];
		Vector4 shadowColor[4];

		for (uint32_t j = 0; j < n; ++j)
		{
			const Light& light = lights[directional[i + j]];

			lightDirectionAndRange[j] = view * light.direction.xyz0() + Vector4(0.0f, 0.0f, 0.0f, light.range);

			sunColorAndIntensity[j] = light.sunColor;
			sunColorAndIntensity[j].set(3, dot3(light.sunColor, Vector4(0.2125f, 0.7154f, 0.0721f, 0.0f)));

			baseColor[j] = light.baseColor;
			shadowColor[j] = light.shadowColor;
		}
		for (uint32_t j = n; j < 4; ++j)
		{
			lightDirectionAndRange[j] = Vector4::zero();
			sunColorAndIntensity[j] = Vector4::zero();
			baseColor[j] = Vector4::zero();
			shadowColor[j] = Vector4::zero();
		}

		m_lightDirectionalsShader->setFloatParameter(s_handleTime, time);
		m_lightDirectionalsShader->setVectorParameter(s_handleMagicCoeffs, Vector4(1.0f / p11, 1.0f / p22, 0.0f, 0.0f));
		m_lightDirectionalsShader->setMatrixParameter(s_handleViewInverse, view.inverse());
		m_lightDirectionalsShader->setTextureParameter(s_handleDepthMap, depthMap);
		m_lightDirectionalsShader->setTextureParameter(s_handleNormalMap, normalMap);
		m_lightDirectionalsShader->setTextureParameter(s_handleMiscMap, miscMap);
		m_lightDirectionalsShader->setTextureParameter(s_handleColorMap, colorMap);
		m_lightDirectionalsShader->setVectorArrayParameter(s_handleLightDirectionAndRange, lightDirectionAndRange, 4);
		m_lightDirectionalsShader->setVectorArrayParameter(s_handleLightSunColor, sunColorAndIntensity, 4);
		m_lightDirectionalsShader->setVectorArrayParameter(s_handleLightBaseColor, baseColor, 4);
		m_lightDirectionalsShader->setVectorArrayParameter(s_handleLightShadowColor, shadowColor, 4);

		m_lightDirectionalsShader->draw(renderView, m_vertexBufferQuad, 0, m_primitivesQuad);		
	}

	// Render point lights.
	for (uint32_t i = 0; i < point.size(); i += 4)
	{
		uint32_t n = std::min< uint32_t >(point.size() - i, 4);
		T_ASSERT(n > 0);

		Vector4 lightPosition[4];
		Vector4 lightDirectionAndRange[4];
		Vector4 sunColorAndIntensity[4];
		Vector4 baseColor[4];
		Vector4 shadowColor[4];

		for (uint32_t j = 0; j < n; ++j)
		{
			const Light& light = lights[point[i + j]];

			lightPosition[j] = view * light.position.xyz1();
			lightDirectionAndRange[j] = view * light.direction.xyz0() + Vector4(0.0f, 0.0f, 0.0f, light.range);

			sunColorAndIntensity[j] = light.sunColor;
			sunColorAndIntensity[j].set(3, dot3(light.sunColor, Vector4(0.2125f, 0.7154f, 0.0721f, 0.0f)));

			baseColor[j] = light.baseColor;
			shadowColor[j] = light.shadowColor;
		}
		for (uint32_t j = n; j < 4; ++j)
		{
			lightPosition[j] = Vector4::zero();
			lightDirectionAndRange[j] = Vector4::zero();
			sunColorAndIntensity[j] = Vector4::zero();
			baseColor[j] = Vector4::zero();
			shadowColor[j] = Vector4::zero();
		}

		m_lightPointsShader->setFloatParameter(s_handleTime, time);
		m_lightPointsShader->setVectorParameter(s_handleMagicCoeffs, Vector4(1.0f / p11, 1.0f / p22, 0.0f, 0.0f));
		m_lightPointsShader->setMatrixParameter(s_handleViewInverse, view.inverse());
		m_lightPointsShader->setTextureParameter(s_handleDepthMap, depthMap);
		m_lightPointsShader->setTextureParameter(s_handleNormalMap, normalMap);
		m_lightPointsShader->setTextureParameter(s_handleMiscMap, miscMap);
		m_lightPointsShader->setTextureParameter(s_handleColorMap, colorMap);
		m_lightPointsShader->setVectorArrayParameter(s_handleLightPosition, lightPosition, 4);
		m_lightPointsShader->setVectorArrayParameter(s_handleLightDirectionAndRange, lightDirectionAndRange, 4);
		m_lightPointsShader->setVectorArrayParameter(s_handleLightSunColor, sunColorAndIntensity, 4);
		m_lightPointsShader->setVectorArrayParameter(s_handleLightBaseColor, baseColor, 4);
		m_lightPointsShader->setVectorArrayParameter(s_handleLightShadowColor, shadowColor, 4);

		m_lightPointsShader->draw(renderView, m_vertexBufferQuad, 0, m_primitivesQuad);		
	}

	// Render spot lights.
	for (uint32_t i = 0; i < spot.size(); i += 4)
	{
		uint32_t n = std::min< uint32_t >(spot.size() - i, 4);
		T_ASSERT(n > 0);

		Vector4 lightPositionAndRadius[4];
		Vector4 lightDirectionAndRange[4];
		Vector4 sunColorAndIntensity[4];
		Vector4 baseColor[4];
		Vector4 shadowColor[4];

		for (uint32_t j = 0; j < n; ++j)
		{
			const Light& light = lights[spot[i + j]];

			lightPositionAndRadius[j] = (view * light.position.xyz1()).xyz0() + Vector4(0.0f, 0.0f, 0.0f, light.radius);
			lightDirectionAndRange[j] = view * light.direction.xyz0() + Vector4(0.0f, 0.0f, 0.0f, light.range);

			sunColorAndIntensity[j] = light.sunColor;
			sunColorAndIntensity[j].set(3, dot3(light.sunColor, Vector4(0.2125f, 0.7154f, 0.0721f, 0.0f)));

			baseColor[j] = light.baseColor;
			shadowColor[j] = light.shadowColor;
		}
		for (uint32_t j = n; j < 4; ++j)
		{
			lightPositionAndRadius[j] = Vector4::zero();
			lightDirectionAndRange[j] = Vector4::zero();
			sunColorAndIntensity[j] = Vector4::zero();
			baseColor[j] = Vector4::zero();
			shadowColor[j] = Vector4::zero();
		}

		m_lightSpotsShader->setFloatParameter(s_handleTime, time);
		m_lightSpotsShader->setVectorParameter(s_handleMagicCoeffs, Vector4(1.0f / p11, 1.0f / p22, 0.0f, 0.0f));
		m_lightSpotsShader->setMatrixParameter(s_handleViewInverse, view.inverse());
		m_lightSpotsShader->setTextureParameter(s_handleDepthMap, depthMap);
		m_lightSpotsShader->setTextureParameter(s_handleNormalMap, normalMap);
		m_lightSpotsShader->setTextureParameter(s_handleMiscMap, miscMap);
		m_lightSpotsShader->setTextureParameter(s_handleColorMap, colorMap);
		m_lightSpotsShader->setVectorArrayParameter(s_handleLightPositionAndRadius, lightPositionAndRadius, 4);
		m_lightSpotsShader->setVectorArrayParameter(s_handleLightDirectionAndRange, lightDirectionAndRange, 4);
		m_lightSpotsShader->setVectorArrayParameter(s_handleLightSunColor, sunColorAndIntensity, 4);
		m_lightSpotsShader->setVectorArrayParameter(s_handleLightBaseColor, baseColor, 4);
		m_lightSpotsShader->setVectorArrayParameter(s_handleLightShadowColor, shadowColor, 4);

		m_lightSpotsShader->draw(renderView, m_vertexBufferQuad, 0, m_primitivesQuad);			
	}	
}

void LightRendererDeferred::renderFinalColor(
	render::IRenderView* renderView,
	float time,
	const Matrix44& projection,
	const Matrix44& view,
	render::ITexture* depthMap,
	render::ITexture* normalMap,
	render::ITexture* miscMap,
	render::ITexture* colorMap,
	render::ITexture* lightMap
)
{
	Scalar p11 = projection.get(0, 0);
	Scalar p22 = projection.get(1, 1);

	m_finalColorShader->setMatrixParameter(s_handleProjection, projection);
	m_finalColorShader->setMatrixParameter(s_handleViewInverse, view.inverse());
	m_finalColorShader->setVectorParameter(s_handleMagicCoeffs, Vector4(1.0f / p11, 1.0f / p22, 0.0f, 0.0f));
	m_finalColorShader->setTextureParameter(s_handleDepthMap, depthMap);
	m_finalColorShader->setTextureParameter(s_handleNormalMap, normalMap);
	m_finalColorShader->setTextureParameter(s_handleMiscMap, miscMap);
	m_finalColorShader->setTextureParameter(s_handleColorMap, colorMap);
	m_finalColorShader->setTextureParameter(s_handleLightMap, lightMap);

	m_finalColorShader->draw(renderView, m_vertexBufferQuad, 0, m_primitivesQuad);
}

void LightRendererDeferred::renderReflections(
	render::IRenderView* renderView,
	const Matrix44& projection,
	const Matrix44& view,
	const Vector4& fogDistanceAndDensity,
	const Vector4& fogColor,
	bool traceReflections,
	render::ITexture* screenMap,
	render::ITexture* reflectionMap,
	render::ITexture* depthMap,
	render::ITexture* normalMap,
	render::ITexture* miscMap,
	render::ITexture* colorMap
)
{
	Scalar p11 = projection.get(0, 0);
	Scalar p22 = projection.get(1, 1);

	m_reflectionShader->setCombination(s_handleTraceReflections, traceReflections);
	m_reflectionShader->setMatrixParameter(s_handleProjection, projection);
	m_reflectionShader->setMatrixParameter(s_handleViewInverse, view.inverse());
	m_reflectionShader->setVectorParameter(s_handleMagicCoeffs, Vector4(1.0f / p11, 1.0f / p22, 0.0f, 0.0f));
	m_reflectionShader->setVectorParameter(s_handleFogDistanceAndDensity, fogDistanceAndDensity);
	m_reflectionShader->setVectorParameter(s_handleFogColor, fogColor);
	m_reflectionShader->setTextureParameter(s_handleScreenMap, screenMap);
	m_reflectionShader->setTextureParameter(s_handleReflectionMap, reflectionMap);
	m_reflectionShader->setTextureParameter(s_handleDepthMap, depthMap);
	m_reflectionShader->setTextureParameter(s_handleNormalMap, normalMap);
	m_reflectionShader->setTextureParameter(s_handleMiscMap, miscMap);
	m_reflectionShader->setTextureParameter(s_handleColorMap, colorMap);

	m_reflectionShader->draw(renderView, m_vertexBufferQuad, 0, m_primitivesQuad);
}

void LightRendererDeferred::renderFog(
	render::IRenderView* renderView,
	const Matrix44& projection,
	const Matrix44& view,
	const Vector4& fogDistanceAndDensity,
	const Vector4& fogColor,
	render::ITexture* reflectionMap,
	render::ITexture* depthMap,
	render::ITexture* normalMap,
	render::ITexture* miscMap,
	render::ITexture* colorMap
)
{
	Scalar p11 = projection.get(0, 0);
	Scalar p22 = projection.get(1, 1);

	m_fogShader->setMatrixParameter(s_handleProjection, projection);
	m_fogShader->setMatrixParameter(s_handleView, view);
	m_fogShader->setMatrixParameter(s_handleViewInverse, view.inverse());
	m_fogShader->setVectorParameter(s_handleMagicCoeffs, Vector4(1.0f / p11, 1.0f / p22, 0.0f, 0.0f));
	m_fogShader->setVectorParameter(s_handleFogDistanceAndDensity, fogDistanceAndDensity);
	m_fogShader->setVectorParameter(s_handleFogColor, fogColor);
	m_fogShader->setTextureParameter(s_handleReflectionMap, reflectionMap);
	m_fogShader->setTextureParameter(s_handleDepthMap, depthMap);
	m_fogShader->setTextureParameter(s_handleNormalMap, normalMap);
	m_fogShader->setTextureParameter(s_handleMiscMap, miscMap);
	m_fogShader->setTextureParameter(s_handleColorMap, colorMap);

	m_fogShader->draw(renderView, m_vertexBufferQuad, 0, m_primitivesQuad);
}

	}
}
