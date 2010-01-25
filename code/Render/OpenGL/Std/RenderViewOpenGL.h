#ifndef traktor_render_RenderViewOpenGL_H
#define traktor_render_RenderViewOpenGL_H

#include <stack>
#include "Render/IRenderView.h"
#include "Render/OpenGL/Platform.h"
#include "Render/OpenGL/Std/ContextOpenGL.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_RENDER_OPENGL_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace render
	{

class RenderSystemOpenGL;
class VertexBufferOpenGL;
class IndexBufferOpenGL;
class ProgramOpenGL;
class RenderTargetOpenGL;

/*!
 * \ingroup OGL
 */
class T_DLLCLASS RenderViewOpenGL : public IRenderView
{
	T_RTTI_CLASS;

public:
#if defined(_WIN32)

	RenderViewOpenGL(
		ContextOpenGL* context,
		ContextOpenGL* globalContext,
		HWND hWnd
	);

#elif defined(__APPLE__)

	RenderViewOpenGL(
		ContextOpenGL* context,
		ContextOpenGL* globalContext
	);

#else

	RenderViewOpenGL(
		ContextOpenGL* context,
		ContextOpenGL* globalContext
	);

#endif

	virtual ~RenderViewOpenGL();

	virtual void close();

	virtual void resize(int32_t width, int32_t height);

	virtual void setViewport(const Viewport& viewport);

	virtual Viewport getViewport();

	virtual bool getNativeAspectRatio(float& outAspectRatio) const;

	virtual bool begin();

	virtual bool begin(RenderTargetSet* renderTargetSet, int renderTarget, bool keepDepthStencil);

	virtual void clear(uint32_t clearMask, const float color[4], float depth, int32_t stencil);

	virtual void setVertexBuffer(VertexBuffer* vertexBuffer);

	virtual void setIndexBuffer(IndexBuffer* indexBuffer);

	virtual void setProgram(IProgram* program);

	virtual void draw(const Primitives& primitives);

	virtual void end();

	virtual void present();

	virtual void setMSAAEnable(bool msaaEnable);

private:
	Ref< RenderSystemOpenGL > m_renderSystem;
	Ref< ContextOpenGL > m_context;
	Ref< ContextOpenGL > m_globalContext;
	std::stack< RenderTargetOpenGL* > m_renderTargetStack;
	Ref< VertexBufferOpenGL > m_currentVertexBuffer;
	Ref< IndexBufferOpenGL > m_currentIndexBuffer;
	Ref< ProgramOpenGL > m_currentProgram;
	bool m_currentDirty;
};

	}
}

#endif	// traktor_render_RenderViewOpenGL_H
