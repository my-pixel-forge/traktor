/*
 * TRAKTOR
 * Copyright (c) 2022 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Core/Containers/StaticSet.h"
#include "Core/Log/Log.h"
#include "Core/Misc/SafeDestroy.h"
#include "Core/Misc/String.h"
#include "Core/Timer/Profiler.h"
#include "Render/IRenderSystem.h"
#include "Render/IRenderTargetSet.h"
#include "Render/Context/RenderBlock.h"
#include "Render/Context/RenderContext.h"
#include "Render/Frame/RenderGraph.h"
#include "Render/Frame/RenderGraphTargetSetPool.h"

namespace traktor::render
{
	namespace
	{

void traverse(const RefArray< const RenderPass >& passes, int32_t depth, int32_t index, const std::function< void(int32_t, int32_t) >& fn)
{
	// Traverse inputs first as we want to traverse passes depth-first.
	for (const auto& input : passes[index]->getInputs())
	{
		for (int32_t i = 0; i < passes.size(); ++i)
		{
			if (passes[i]->getOutput().resourceId == input.resourceId)
				traverse(passes, depth + 1, i, fn);
		}
	}

	// Call visitor for this pass.
	fn(depth, index);
}

	}

T_IMPLEMENT_RTTI_CLASS(L"traktor.render.RenderGraph", RenderGraph, Object)

RenderGraph::RenderGraph(
	IRenderSystem* renderSystem,
	uint32_t multiSample,
	const fn_profiler_t& profiler
)
:	m_pool(new RenderGraphTargetSetPool(renderSystem))
,	m_multiSample(multiSample)
,	m_nextResourceId(1)
,	m_profiler(profiler)
{
}

RenderGraph::~RenderGraph()
{
	T_FATAL_ASSERT_M(m_pool == nullptr, L"Forgot to destroy RenderGraph instance.");
}

void RenderGraph::destroy()
{
	m_targets.clear();
	m_buffers.clear();
	m_passes.clear();
	for (int32_t i = 0; i < sizeof_array(m_order); ++i)
		m_order[i].clear();
	safeDestroy(m_pool);
}

handle_t RenderGraph::addTargetSet(const wchar_t* const name, IRenderTargetSet* targetSet)
{
	handle_t resourceId = m_nextResourceId++;

	auto& tr = m_targets[resourceId];
	tr.name = name;
	tr.persistentHandle = 0;
	tr.rts = targetSet;
	tr.sizeReferenceTargetSetId = 0;
	tr.inputRefCount = 0;
	tr.outputRefCount = 0;
	tr.external = true;

	return resourceId;
}

handle_t RenderGraph::addTransientTargetSet(
	const wchar_t* const name,
	const RenderGraphTargetSetDesc& targetSetDesc,
	IRenderTargetSet* sharedDepthStencilTargetSet,
	handle_t sizeReferenceTargetSetId
)
{
	handle_t resourceId = m_nextResourceId++;

	auto& tr = m_targets[resourceId];
	tr.name = name;
	tr.persistentHandle = 0;
	tr.targetSetDesc = targetSetDesc;
	tr.sharedDepthStencilTargetSet = sharedDepthStencilTargetSet;
	tr.sizeReferenceTargetSetId = sizeReferenceTargetSetId;
	tr.inputRefCount = 0;
	tr.outputRefCount = 0;
	tr.external = false;

	return resourceId;
}

handle_t RenderGraph::addPersistentTargetSet(
	const wchar_t* const name,
	handle_t persistentHandle,
	const RenderGraphTargetSetDesc& targetSetDesc,
	IRenderTargetSet* sharedDepthStencilTargetSet,
	handle_t sizeReferenceTargetSetId
)
{
	handle_t resourceId = m_nextResourceId++;

	auto& tr = m_targets[resourceId];
	tr.name = name;
	tr.persistentHandle = persistentHandle;
	tr.targetSetDesc = targetSetDesc;
	tr.sharedDepthStencilTargetSet = sharedDepthStencilTargetSet;
	tr.sizeReferenceTargetSetId = sizeReferenceTargetSetId;
	tr.inputRefCount = 0;
	tr.outputRefCount = 0;
	tr.external = false;

	return resourceId;
}

handle_t RenderGraph::addBuffer(const wchar_t* const name, Buffer* buffer)
{
	handle_t resourceId = m_nextResourceId++;

	auto& br = m_buffers[resourceId];
	br.name = name;
	br.buffer = buffer;

	return resourceId;
}

IRenderTargetSet* RenderGraph::getTargetSet(handle_t resource) const
{
	auto it = m_targets.find(resource);
	return (it != m_targets.end()) ? it->second.rts : nullptr;
}

Buffer* RenderGraph::getBuffer(handle_t resource) const
{
	auto it = m_buffers.find(resource);
	return (it != m_buffers.end()) ? it->second.buffer : nullptr;
}

void RenderGraph::addPass(const RenderPass* pass)
{
	m_passes.push_back(pass);
}

bool RenderGraph::validate()
{
	// Find root passes, which are either of:
	// 1) Have no output resource.
	// 2) Writing to external targets.
	// 3) Writing to primary target.
	StaticVector< uint32_t, 32 > roots;
	for (uint32_t i = 0; i < (uint32_t)m_passes.size(); ++i)
	{
		const auto pass = m_passes[i];
		const auto& output = pass->getOutput();
		if (output.resourceId == ~0)
			roots.push_back(i);
		else if (output.resourceId != ~0 && output.resourceId != 0)
		{
			auto it = m_targets.find(output.resourceId);
			if (it->second.external)
				roots.push_back(i);
		}
		else if (output.resourceId == 0)
			roots.push_back(i);
	}

	// Determine maximum depths of each pass.
	StaticVector< int32_t, 512 > depths;
	depths.resize(m_passes.size(), -1);
	for (auto root : roots)
	{
		traverse(m_passes, 0, root, [&](int32_t depth, int32_t index) {
			T_ASSERT(depth < sizeof_array(m_order));
			depths[index] = std::max(depths[index], depth);
		});
	}

	// Gather passes in order for each depth.
	for (int32_t i = 0; i < sizeof_array(m_order); ++i)
		m_order[i].resize(0);
	for (uint32_t i = 0; i < (uint32_t)m_passes.size(); ++i)
	{
		if (depths[i] >= 0)
			m_order[depths[i]].push_back(i);
	}

	// Sort each depth based on output resource.
	for (int32_t i = 0; i < sizeof_array(m_order); ++i)
	{
		std::stable_sort(m_order[i].begin(), m_order[i].end(), [&](uint32_t lh, uint32_t rh) {
			const auto lt = m_passes[lh]->getOutput().resourceId;
			const auto rt = m_passes[rh]->getOutput().resourceId;
			return lt > rt;
		});
	}

	// Count input and output reference counts of all targets.
	for (int32_t i = 0; i < sizeof_array(m_order); ++i)
	{
		const auto& order = m_order[i];
		for (const auto index : order)
		{
			const auto pass = m_passes[index];
			const auto& output = pass->getOutput();
			if (output.resourceId != ~0)
			{
				auto it = m_targets.find(output.resourceId);
				if (it != m_targets.end())
					it->second.outputRefCount++;
			}
			for (const auto& input : pass->getInputs())
			{
				auto it = m_targets.find(input.resourceId);
				if (it != m_targets.end())
					it->second.inputRefCount++;
			}	
		}
	}
	return true;
}

bool RenderGraph::build(RenderContext* renderContext, int32_t width, int32_t height)
{
	T_FATAL_ASSERT(!renderContext->havePendingDraws());

	// Acquire all persistent targets first in case they are read from
	// before being used as an output.
	for (auto& it : m_targets)
	{
		auto& target = it.second;
		if (
			target.rts == nullptr &&
			target.persistentHandle != 0 &&
			(target.inputRefCount != 0 || target.outputRefCount != 0)
		)
		{
			if (!acquire(width, height, target))
			{
				cleanup();
				return false;
			}
		}
	}

#if !defined(__ANDROID__) && !defined(__IOS__)
	double referenceOffset = Profiler::getInstance().getTime();

	int32_t* queryHandles = nullptr;
	int32_t* referenceQueryHandle = nullptr;
	int32_t* passQueryHandles = nullptr;

	if (m_profiler)
	{
		// Allocate query handles from render context's heap since they get automatically
		// freed when the context is reset.
		queryHandles = (int32_t*)renderContext->alloc((uint32_t)(m_passes.size() + 1) * sizeof(int32_t), (uint32_t)alignOf< int32_t >());
		referenceQueryHandle = queryHandles;
		passQueryHandles = queryHandles + 1;

		auto pb = renderContext->alloc< ProfileBeginRenderBlock >();
		pb->queryHandle = referenceQueryHandle;
		renderContext->enqueue(pb);
	}
#endif

	// Render passes in dependency order.
	//
	// Since we don't want to load/store render passes, esp when using MSAA,
	// we track current output target and automatically merge render passes.
	//
	handle_t currentOutputTargetSetId = ~0U;
	for (int32_t i = sizeof_array(m_order) - 1; i >= 0; --i)
	{
		const auto& order = m_order[i];
		for (const auto index : order)
		{
			const auto pass = m_passes[index];
			const auto& inputs = pass->getInputs();
			const auto& output = pass->getOutput();

			// Begin render pass.
			if (pass->haveOutput())
			{
				if (output.resourceId != 0)
				{
					if (currentOutputTargetSetId != output.resourceId)
					{
						if (currentOutputTargetSetId != ~0U)
						{
							auto te = renderContext->alloc< EndPassRenderBlock >();
							renderContext->enqueue(te);
							renderContext->mergeCompute();
						}

						// Begin pass if resource is a target.
						auto it = m_targets.find(output.resourceId);
						if (it != m_targets.end())
						{
							auto& target = it->second;
							if (target.rts == nullptr)
							{
								T_ASSERT(!target.external);
								if (!acquire(width, height, target))
								{
									cleanup();
									return false;
								}
							}	

							auto tb = renderContext->alloc< BeginPassRenderBlock >(pass->getName());
							tb->renderTargetSet = target.rts;
							tb->clear = output.clear;
							tb->load = output.load;
							tb->store = output.store;
							renderContext->enqueue(tb);
						}

						currentOutputTargetSetId = output.resourceId;
					}
				}
				else
				{
					if (currentOutputTargetSetId != 0)
					{
						if (currentOutputTargetSetId != ~0U)
						{
							auto te = renderContext->alloc< EndPassRenderBlock >();
							renderContext->enqueue(te);
							renderContext->mergeCompute();
						}

						auto tb = renderContext->alloc< BeginPassRenderBlock >(pass->getName());
						tb->clear = output.clear;
						tb->load = output.load;
						tb->store = output.store;

						renderContext->enqueue(tb);

						currentOutputTargetSetId = 0;
					}	
				}
			}
			else if (currentOutputTargetSetId != ~0U)
			{
				auto te = renderContext->alloc< EndPassRenderBlock >();
				renderContext->enqueue(te);
				renderContext->mergeCompute();
				currentOutputTargetSetId = ~0U;
			}

#if !defined(__ANDROID__) && !defined(__IOS__)
			if (m_profiler)
			{
				auto pb = renderContext->alloc< ProfileBeginRenderBlock >();
				pb->queryHandle = &passQueryHandles[index];
				renderContext->enqueue(pb);
			}
#endif

			// Build this pass.
			T_PROFILER_BEGIN(L"RenderGraph build \"" + pass->getName() + L"\"");
			for (const auto& build : pass->getBuilds())
			{
				build(*this, renderContext);

				// Merge all pending draws after each build step.
				renderContext->mergeDraw(render::RpAll);
			}
			T_PROFILER_END();

#if !defined(__ANDROID__) && !defined(__IOS__)
			if (m_profiler)
			{
				auto pe = renderContext->alloc< ProfileEndRenderBlock >();
				pe->queryHandle = &passQueryHandles[index];
				renderContext->enqueue(pe);
			}
#endif

			// Decrement reference counts on input targets; release if last reference.
			for (const auto& input : inputs)
			{
				if (input.resourceId == 0)
					continue;

				auto it = m_targets.find(input.resourceId);
				if (it == m_targets.end())
					continue;

				auto& target = it->second;
				if (--target.inputRefCount <= 0)
				{
					if (!target.external)
						m_pool->release(target.rts);
					target.rts = nullptr;
				}
			}
		}
	}

	if (currentOutputTargetSetId != ~0U)
	{
		auto te = renderContext->alloc< EndPassRenderBlock >();
		renderContext->enqueue(te);
	}

	// Always merge any lingering compute jobs left at the end of frame.
	renderContext->mergeCompute();

	T_FATAL_ASSERT(!renderContext->havePendingComputes());
	T_FATAL_ASSERT(!renderContext->havePendingDraws());

#if !defined(__ANDROID__) && !defined(__IOS__)
	if (m_profiler)
	{
		auto pe = renderContext->alloc< ProfileEndRenderBlock >();
		pe->queryHandle = referenceQueryHandle;
		renderContext->enqueue(pe);

		// Report all queries last using reference query to calculate offset.
		int32_t ordinal = 0;
		for (int32_t i = sizeof_array(m_order) - 1; i >= 0; --i)
		{
			const auto& order = m_order[i];
			if (order.empty())
				continue;
			for (int32_t j = 0; j < (int32_t)order.size(); ++j)
			{
				uint32_t index = order[j];
				const auto pass = m_passes[index];
				auto pr = renderContext->alloc< ProfileReportRenderBlock >();
				pr->name = pass->getName();
				pr->queryHandle = &passQueryHandles[index];
				pr->referenceQueryHandle = referenceQueryHandle;
				pr->offset = referenceOffset;
				pr->sink = [=](const std::wstring& name, double start, double duration) { m_profiler(ordinal, i, name, start, duration); };
				renderContext->enqueue(pr);
				++ordinal;
			}
		}
	}
#endif

	// Ensure all persistent targets are returned to pool, since we're
	// manually acquiring all at the beginning.
	for (auto& it : m_targets)
	{
		auto& target = it.second;
		if (target.rts != nullptr && target.persistentHandle != 0)
		{
			m_pool->release(target.rts);
			target.rts = nullptr;
		}
	}

	// Cleanup pool data structure.
	m_pool->cleanup();

	// Remove all data; keeps memory allocated for arrays
	// since it's very likely this will be identically
	// re-populated next frame.
	cleanup();
	return true;
}

bool RenderGraph::acquire(int32_t width, int32_t height, TargetResource& inoutTarget)
{
	// Use size of reference target.
	if (inoutTarget.sizeReferenceTargetSetId != 0)
	{
		auto it = m_targets.find(inoutTarget.sizeReferenceTargetSetId);
		if (it == m_targets.end())
			return false;

		width = it->second.targetSetDesc.width;
		height = it->second.targetSetDesc.height;
	}

	// Use size of shared depth/stencil target since they must match.
	if (inoutTarget.sharedDepthStencilTargetSet != nullptr)
	{
		width = inoutTarget.sharedDepthStencilTargetSet->getWidth();
		height = inoutTarget.sharedDepthStencilTargetSet->getHeight();
	}

	inoutTarget.rts = m_pool->acquire(
		inoutTarget.name,
		inoutTarget.targetSetDesc,
		inoutTarget.sharedDepthStencilTargetSet,
		width,
		height,
		m_multiSample,
		inoutTarget.persistentHandle
	);
	if (!inoutTarget.rts)
		return false;

	return true;
}

void RenderGraph::cleanup()
{
	m_passes.resize(0);
	m_targets.reset();
	m_buffers.reset();
	for (int32_t i = 0; i < sizeof_array(m_order); ++i)
		m_order[i].resize(0);
}

}
