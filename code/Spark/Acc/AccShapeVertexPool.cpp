#include "Core/Log/Log.h"
#include "Core/Misc/Align.h"
#include "Core/Misc/SafeDestroy.h"
#include "Spark/Acc/AccShapeVertexPool.h"
#include "Render/Buffer.h"
#include "Render/IRenderSystem.h"

namespace traktor
{
	namespace spark
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.spark.AccShapeVertexPool", AccShapeVertexPool, Object)

AccShapeVertexPool::AccShapeVertexPool(render::IRenderSystem* renderSystem)
:	m_renderSystem(renderSystem)
{
}

bool AccShapeVertexPool::create(const AlignedVector< render::VertexElement >& vertexElements)
{
	m_vertexLayout = m_renderSystem->createVertexLayout(vertexElements);
	m_vertexSize = render::getVertexSize(vertexElements);
	return true;
}

void AccShapeVertexPool::destroy()
{
	for (auto i = m_freeBuffers.begin(); i != m_freeBuffers.end(); ++i)
	{
		if (*i)
			i->destroy();
	}

	m_freeBuffers.clear();
	m_renderSystem = nullptr;
}

bool AccShapeVertexPool::acquire(int32_t vertexCount, Ref< render::Buffer >& outVertexBuffer)
{
	for (auto i = m_freeBuffers.begin(); i != m_freeBuffers.end(); ++i)
	{
		const uint32_t bufferVertexCount = i->getBufferSize() / m_vertexSize;
		if (bufferVertexCount >= vertexCount)
		{
			outVertexBuffer = *i;
			m_freeBuffers.erase(i);
			return true;
		}
	}

	Ref< render::Buffer > vertexBuffer = m_renderSystem->createBuffer(
		render::BuVertex,
		vertexCount,
		m_vertexSize,
		true
	);
	if (!vertexBuffer)
		return false;

	outVertexBuffer = vertexBuffer;
	return true;
}

void AccShapeVertexPool::release(render::Buffer* vertexBuffer)
{
	if (vertexBuffer)
		m_freeBuffers.push_back(vertexBuffer);
}

	}
}
