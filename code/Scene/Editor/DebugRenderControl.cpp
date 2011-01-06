#include <limits>
#include "Scene/Editor/DebugRenderControl.h"
#include "Scene/Editor/SceneEditorContext.h"
#include "Render/IRenderSystem.h"
#include "Render/IRenderView.h"
#include "Render/PrimitiveRenderer.h"
#include "Ui/MethodHandler.h"
#include "Ui/Widget.h"
#include "Ui/Events/SizeEvent.h"
#include "Ui/Itf/IWidget.h"

namespace traktor
{
	namespace scene
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.render.DebugRenderControl", DebugRenderControl, ui::Widget)

DebugRenderControl::DebugRenderControl()
:	m_dirtySize(0, 0)
{
}

bool DebugRenderControl::create(ui::Widget* parent, SceneEditorContext* context)
{
	m_context = context;
	T_ASSERT (m_context);

	m_renderWidget = new ui::Widget();
	if (!m_renderWidget->create(parent))
		return false;

	render::RenderViewEmbeddedDesc desc;
	desc.depthBits = 0;
	desc.stencilBits = 0;
	desc.multiSample = 4;
	desc.waitVBlank = false;
	desc.nativeWindowHandle = m_renderWidget->getIWidget()->getSystemHandle();

	m_renderView = m_context->getRenderSystem()->createRenderView(desc);
	if (!m_renderView)
		return false;

	m_primitiveRenderer = new render::PrimitiveRenderer();
	if (!m_primitiveRenderer->create(
		m_context->getResourceManager(),
		m_context->getRenderSystem()
	))
		return false;

	m_renderWidget->addSizeEventHandler(ui::createMethodHandler(this, &DebugRenderControl::eventSize));
	m_renderWidget->addPaintEventHandler(ui::createMethodHandler(this, &DebugRenderControl::eventPaint));

	return true;
}

void DebugRenderControl::destroy()
{
	if (m_primitiveRenderer)
	{
		m_primitiveRenderer->destroy();
		m_primitiveRenderer = 0;
	}

	if (m_renderView)
	{
		m_renderView->close();
		m_renderView = 0;
	}

	if (m_renderWidget)
	{
		m_renderWidget->destroy();
		m_renderWidget = 0;
	}
}

void DebugRenderControl::updateWorldRenderer()
{
}

bool DebugRenderControl::handleCommand(const ui::Command& command)
{
	return false;
}

void DebugRenderControl::update()
{
	m_renderWidget->update();
}

void DebugRenderControl::eventSize(ui::Event* event)
{
	if (!m_renderView || !m_renderWidget->isVisible(true))
		return;

	ui::SizeEvent* s = static_cast< ui::SizeEvent* >(event);
	ui::Size sz = s->getSize();

	if (sz.cx == m_dirtySize.cx && sz.cy == m_dirtySize.cy)
		return;

	m_renderView->resize(sz.cx, sz.cy);
	m_renderView->setViewport(render::Viewport(0, 0, sz.cx, sz.cy, 0, 1));

	m_dirtySize = sz;
}

void DebugRenderControl::eventPaint(ui::Event* event)
{
	if (!m_renderView || !m_primitiveRenderer)
		return;

	if (m_renderView->begin(render::EtCyclop))
	{
		const float clearColor[] = { 0.7f, 0.7f, 0.7f, 0.0f };
		m_renderView->clear(
			render::CfColor | render::CfDepth,
			clearColor,
			1.0f,
			128
		);

		float ratio = float(m_dirtySize.cx) / m_dirtySize.cy;

		Matrix44 projection;
		if (ratio < 1.0f)
		{
			projection = orthoLh(
				4.0f,
				4.0f / ratio,
				-1.0f,
				1.0f
			);
		}
		else
		{
			projection = orthoLh(
				4.0f * ratio,
				4.0f,
				-1.0f,
				1.0f
			);
		}

		m_primitiveRenderer->begin(m_renderView);
		m_primitiveRenderer->setClipDistance(100.0f);
		m_primitiveRenderer->pushProjection(projection);
		m_primitiveRenderer->pushView(Matrix44::identity());
		m_primitiveRenderer->pushDepthEnable(false);

		for (uint32_t i = 0; i < 2; ++i)
		{
			float ox = float(i * 2) - 1.0f;

			m_primitiveRenderer->drawWireQuad(
				Vector4(-1.0f + ox,  1.0f, 0.0f, 1.0f),
				Vector4( 1.0f + ox,  1.0f, 0.0f, 1.0f),
				Vector4( 1.0f + ox, -1.0f, 0.0f, 1.0f),
				Vector4(-1.0f + ox, -1.0f, 0.0f, 1.0f),
				Color4ub(0, 0, 0)
			);

			if (m_context->getDebugTexture(i))
			{
				m_primitiveRenderer->drawTextureQuad(
					Vector4(-1.0f + ox,  1.0f, 0.0f, 1.0f), Vector2(0.0f, 0.0f),
					Vector4( 1.0f + ox,  1.0f, 0.0f, 1.0f), Vector2(1.0f, 0.0f),
					Vector4( 1.0f + ox, -1.0f, 0.0f, 1.0f), Vector2(1.0f, 1.0f),
					Vector4(-1.0f + ox, -1.0f, 0.0f, 1.0f), Vector2(0.0f, 1.0f),
					Color4ub(255, 255, 255),
					m_context->getDebugTexture(i)
				);
			}
		}

		m_primitiveRenderer->end(m_renderView);

		m_renderView->end();
		m_renderView->present();
	}

	event->consume();
}

	}
}
