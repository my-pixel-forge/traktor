/*
 * TRAKTOR
 * Copyright (c) 2022 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Core/Misc/SafeDestroy.h"
#include "Render/ITexture.h"
#include "Render/ScreenRenderer.h"
#include "Render/Context/RenderContext.h"
#include "Render/Frame/RenderGraph.h"
#include "Render/Image2/ImageGraph.h"
#include "Render/Image2/ImageGraphContext.h"
#include "Render/Image2/ImagePass.h"
#include "Render/Image2/ImagePassStep.h"
#include "Render/Image2/ImageStructBuffer.h"
#include "Render/Image2/ImageTargetSet.h"
#include "Render/Image2/ImageTexture.h"

namespace traktor::render
{

T_IMPLEMENT_RTTI_CLASS(L"traktor.render.ImageGraph", ImageGraph, Object)

ImageGraph::ImageGraph(const std::wstring& name)
:	m_name(name)
{
}

void ImageGraph::addPasses(
	ScreenRenderer* screenRenderer,
	RenderGraph& renderGraph,
	RenderPass* pass,
	const ImageGraphContext& cx,
	const ImageGraphView& view,
	const std::function< void(ProgramParameters*) >& parametersFn
) const
{
	ImagePass::targetSetVector_t sbufferIds;
	ImagePass::targetSetVector_t targetSetIds;

	// Copy context and append our internal textures and targets so
	// steps can have a single method of accessing input textures.
	ImageGraphContext context = cx;

	sbufferIds.resize(m_sbuffers.size());
	for (int32_t i = 0; i < (int32_t)m_sbuffers.size(); ++i)
	{
		sbufferIds[i] = renderGraph.addPersistentBuffer(
			m_sbuffers[i]->getName().c_str(),
			m_sbuffers[i]->getPersistentHandle(),
			m_sbuffers[i]->getElementCount(),
			m_sbuffers[i]->getElementSize()
		);
		context.associateSBuffer(
			m_sbuffers[i]->getId(),
			sbufferIds[i]
		);
	}

	for (int32_t i = 0; i < (int32_t)m_textures.size(); ++i)
	{
		context.associateTexture(
			m_textures[i]->getTextureId(),
			m_textures[i]->getTexture()
		);
	}

	targetSetIds.resize(m_targetSets.size());
	for (int32_t i = 0; i < (int32_t)m_targetSets.size(); ++i)
	{
		if (m_targetSets[i]->getPersistentHandle() != 0)
		{
			targetSetIds[i] = renderGraph.addPersistentTargetSet(
				m_targetSets[i]->getName().c_str(),
				m_targetSets[i]->getPersistentHandle(),
				true,
				m_targetSets[i]->getTargetSetDesc()
			);
		}
		else
		{
			targetSetIds[i] = renderGraph.addTransientTargetSet(
				m_targetSets[i]->getName().c_str(),
				m_targetSets[i]->getTargetSetDesc()
			);
		}

		// Associate each target in the set.
		const auto& desc = m_targetSets[i]->getTargetSetDesc();
		for (int32_t j = 0; j < desc.count; ++j)
		{
			context.associateTextureTargetSet(
				m_targetSets[i]->getTextureId(j),
				targetSetIds[i],
				j
			);
		}
	}

	// Add all steps to render graph.
	for (auto imagePass : m_passes)
		imagePass->addRenderGraphPasses(this, context, view, targetSetIds, sbufferIds, parametersFn, screenRenderer, renderGraph);

	// Override pass name with our root node's name.
	pass->setName(m_name);

	// Add render pass inputs from each step.
	for (auto step : m_steps)
		step->addRenderPassInputs(this, context, *pass);

	pass->addBuild(
		[=](const RenderGraph& renderGraph, RenderContext* renderContext)
		{
			auto sharedParams = renderContext->alloc< ProgramParameters >();
			sharedParams->beginParameters(renderContext);
			if (parametersFn)
				parametersFn(sharedParams);
			//for (const auto& it : context.getFloatParameters())
			//	sharedParams->setFloatParameter(it.first, it.second);
			//for (const auto& it : context.getVectorParameters())
			//	sharedParams->setVectorParameter(it.first, it.second);
			//for (const auto& it : context.getTextureParameters())
			//	sharedParams->setTextureParameter(it.first, it.second);
			sharedParams->endParameters(renderContext);

			for (auto step : m_steps)
				step->build(
					this,
					context,
					view,
					renderGraph,
					sharedParams,
					renderContext,
					screenRenderer
				);
		}
	);
}

}
