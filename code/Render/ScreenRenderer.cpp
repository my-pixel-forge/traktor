#include "Render/ScreenRenderer.h"
#include "Render/RenderSystem.h"
#include "Render/RenderView.h"
#include "Render/RenderTargetSet.h"
#include "Render/VertexElement.h"
#include "Render/VertexBuffer.h"

namespace traktor
{
	namespace render
	{
		namespace
		{

struct ScreenVertex
{
	float pos[2];
	float texCoord[2];
};

		}

T_IMPLEMENT_RTTI_CLASS(L"traktor.render.ScreenRenderer", ScreenRenderer, Object)

ScreenRenderer::ScreenRenderer()
:	m_handleTargetSize(render::getParameterHandle(L"TargetSize"))
{
}

bool ScreenRenderer::create(RenderSystem* renderSystem)
{
	std::vector< VertexElement > vertexElements;
	vertexElements.push_back(VertexElement(DuPosition, DtFloat2, offsetof(ScreenVertex, pos)));
	vertexElements.push_back(VertexElement(DuCustom, DtFloat2, offsetof(ScreenVertex, texCoord)));

	m_vertexBuffer = renderSystem->createVertexBuffer(vertexElements, 6 * sizeof(ScreenVertex), false);
	if (!m_vertexBuffer)
		return false;

	ScreenVertex* vertex = reinterpret_cast< ScreenVertex* >(m_vertexBuffer->lock());
	T_ASSERT (vertex);

	vertex[0].pos[0] = -1.0f; vertex[0].pos[1] =  1.0f; vertex[0].texCoord[0] = 0.0f; vertex[0].texCoord[1] = 0.0f;
	vertex[1].pos[0] =  1.0f; vertex[1].pos[1] =  1.0f; vertex[1].texCoord[0] = 1.0f; vertex[1].texCoord[1] = 0.0f;
	vertex[2].pos[0] =  1.0f; vertex[2].pos[1] = -1.0f; vertex[2].texCoord[0] = 1.0f; vertex[2].texCoord[1] = 1.0f;

	vertex[3].pos[0] = -1.0f; vertex[3].pos[1] =  1.0f; vertex[3].texCoord[0] = 0.0f; vertex[3].texCoord[1] = 0.0f;
	vertex[4].pos[0] =  1.0f; vertex[4].pos[1] = -1.0f; vertex[4].texCoord[0] = 1.0f; vertex[4].texCoord[1] = 1.0f;
	vertex[5].pos[0] = -1.0f; vertex[5].pos[1] = -1.0f; vertex[5].texCoord[0] = 0.0f; vertex[5].texCoord[1] = 1.0f;

	m_vertexBuffer->unlock();

	m_primitives.setNonIndexed(PtTriangles, 0, 2);
	return true;
}

void ScreenRenderer::destroy()
{
	if (m_vertexBuffer)
	{
		m_vertexBuffer->destroy();
		m_vertexBuffer = 0;
	}
}

void ScreenRenderer::draw(RenderView* renderView, Shader* shader)
{
	Viewport viewport = renderView->getViewport();
	shader->setVectorParameter(m_handleTargetSize, Vector4(
		float(viewport.width),
		float(viewport.height),
		0.0f,
		0.0f
	));

	renderView->setVertexBuffer(m_vertexBuffer);
	shader->draw(renderView, m_primitives);
}

void ScreenRenderer::draw(RenderView* renderView, RenderTargetSet* renderTargetSet, int renderTarget, Shader* shader)
{
	shader->setVectorParameter(m_handleTargetSize, Vector4(
		float(renderTargetSet->getWidth()),
		float(renderTargetSet->getHeight()),
		0.0f,
		0.0f
	));

	renderView->begin(renderTargetSet, renderTarget, false);
	renderView->setVertexBuffer(m_vertexBuffer);
	shader->draw(renderView, m_primitives);
	renderView->end();
}

	}
}
