/*
 * TRAKTOR
 * Copyright (c) 2022-2023 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Render/IRenderTargetSet.h"
#include "Render/ScreenRenderer.h"
#include "Render/Shader.h"
#include "Render/Context/RenderContext.h"
#include "Render/Frame/RenderGraph.h"
#include "Resource/IResourceManager.h"
#include "World/WorldRenderView.h"
#include "World/Editor/Overlays/GBufferNormalsOverlay.h"

namespace traktor::world
{
	namespace
	{

const resource::Id< render::Shader > c_debugShader(Guid(L"{949B3C96-0196-F24E-B36E-98DD504BCE9D}"));
const render::Handle c_handleDebugTechnique(L"Normals");
const render::Handle c_handleDebugAlpha(L"Scene_DebugAlpha");
const render::Handle c_handleDebugViewInverse(L"Scene_DebugViewInverse");
const render::Handle c_handleDebugTexture(L"Scene_DebugTexture");

render::handle_t findTargetByName(const render::RenderGraph& renderGraph, const wchar_t* name)
{
	for (const auto& tm : renderGraph.getTargets())
	{
		if (wcscmp(tm.second.name, name) == 0)
			return tm.first;
	}
	return 0;
}

	}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.world.GBufferNormalsOverlay", 0, GBufferNormalsOverlay, BaseOverlay)

bool GBufferNormalsOverlay::create(resource::IResourceManager* resourceManager)
{
	if (!BaseOverlay::create(resourceManager))
		return false;

	if (!resourceManager->bind(c_debugShader, m_shader))
		return false;

	return true;
}

void GBufferNormalsOverlay::setup(render::RenderGraph& renderGraph, render::ScreenRenderer* screenRenderer, World* world, IWorldRenderer* worldRenderer, const WorldRenderView& worldRenderView, float alpha, float mip) const
{
	render::handle_t gbufferId = findTargetByName(renderGraph, L"GBuffer");
	if (!gbufferId)
	{
		BaseOverlay::setup(renderGraph, screenRenderer, world, worldRenderer, worldRenderView, alpha, mip);
		return;
	}

	const Matrix44 viewInverse = worldRenderView.getView().inverse();

	Ref< render::RenderPass > rp = new render::RenderPass(L"GBuffer normals overlay");
	rp->setOutput(0, render::TfColor, render::TfColor);
	rp->addInput(gbufferId);
	rp->addBuild([=, this](const render::RenderGraph& renderGraph, render::RenderContext* renderContext) {
		auto gbufferTargetSet = renderGraph.getTargetSet(gbufferId);
		if (!gbufferTargetSet)
			return;

		const render::Shader::Permutation perm(c_handleDebugTechnique);

		auto pp = renderContext->alloc< render::ProgramParameters >();
		pp->beginParameters(renderContext);
		pp->setFloatParameter(c_handleDebugAlpha, alpha);
		pp->setMatrixParameter(c_handleDebugViewInverse, viewInverse);
		pp->setTextureParameter(c_handleDebugTexture, gbufferTargetSet->getColorTexture(0));
		pp->endParameters(renderContext);

		screenRenderer->draw(renderContext, m_shader, perm, pp);
	});
	renderGraph.addPass(rp);
}

}
