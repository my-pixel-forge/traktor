/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#include "Core/Log/Log.h"
#include "Core/Misc/Align.h"
#include "Core/Misc/SafeDestroy.h"
#include "Flash/Acc/AccShapeVertexPool.h"
#include "Render/IRenderSystem.h"
#include "Render/VertexBuffer.h"

namespace traktor
{
	namespace flash
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.flash.AccShapeVertexPool", AccShapeVertexPool, Object)

AccShapeVertexPool::AccShapeVertexPool(render::IRenderSystem* renderSystem, uint32_t frameCount, const AlignedVector< render::VertexElement >& vertexElements)
:	m_renderSystem(renderSystem)
,	m_vertexElements(vertexElements)
,	m_frame(0)
{
	m_garbageRanges.resize(frameCount);
}

bool AccShapeVertexPool::create()
{
	return true;
}

void AccShapeVertexPool::destroy()
{
	for (uint32_t i = 0; i < uint32_t(m_garbageRanges.size()); ++i)
		cycleGarbage();

	for (std::list< VertexRange >::iterator i = m_freeRanges.begin(); i != m_freeRanges.end(); ++i)
		safeDestroy(i->vertexBuffer);

	for (std::list< VertexRange >::iterator i = m_usedRanges.begin(); i != m_usedRanges.end(); ++i)
		safeDestroy(i->vertexBuffer);

	m_freeRanges.clear();
	m_usedRanges.clear();
	
	m_renderSystem = 0;
}

bool AccShapeVertexPool::acquireRange(int32_t vertexCount, Range& outRange)
{
	for (std::list< VertexRange >::iterator i = m_freeRanges.begin(); i != m_freeRanges.end(); ++i)
	{
		if (i->vertexCount >= vertexCount)
		{
			outRange.vertexBuffer = i->vertexBuffer;

			m_usedRanges.push_back(*i);
			m_freeRanges.erase(i);

			return true;
		}
	}

	Ref< render::VertexBuffer > vertexBuffer = m_renderSystem->createVertexBuffer(
		m_vertexElements,
		vertexCount * render::getVertexSize(m_vertexElements),
		false
	);
	if (!vertexBuffer)
		return false;

	VertexRange range;
	range.vertexBuffer = vertexBuffer;
	range.vertexCount = vertexCount;
	m_usedRanges.push_back(range);

	outRange.vertexBuffer = vertexBuffer;

	return true;
}

void AccShapeVertexPool::releaseRange(const Range& range)
{
	if (!range.vertexBuffer)
		return;

	for (std::list< VertexRange >::iterator i = m_usedRanges.begin(); i != m_usedRanges.end(); ++i)
	{
		if (i->vertexBuffer == range.vertexBuffer)
		{
			m_garbageRanges[m_frame].push_back(*i);
			m_usedRanges.erase(i);
			return;
		}
	}

	T_FATAL_ERROR;
}

void AccShapeVertexPool::cycleGarbage()
{
	m_frame = (m_frame + 1) % uint32_t(m_garbageRanges.size());
	m_freeRanges.insert(m_freeRanges.end(), m_garbageRanges[m_frame].begin(), m_garbageRanges[m_frame].end());
	m_garbageRanges[m_frame].clear();
}

	}
}
