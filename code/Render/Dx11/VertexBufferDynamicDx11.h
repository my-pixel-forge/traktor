#ifndef traktor_render_VertexBufferDynamicDx11_H
#define traktor_render_VertexBufferDynamicDx11_H

#include "Core/Misc/ComRef.h"
#include "Render/VertexElement.h"
#include "Render/Dx11/VertexBufferDx11.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_RENDER_DX11_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace render
	{

class ContextDx11;

/*!
 * \ingroup DX11
 */
class T_DLLCLASS VertexBufferDynamicDx11 : public VertexBufferDx11
{
	T_RTTI_CLASS;

public:
	static Ref< VertexBufferDynamicDx11 > create(
		ContextDx11* context,
		uint32_t bufferSize,
		const std::vector< VertexElement >& vertexElements
	);

	virtual ~VertexBufferDynamicDx11();

	virtual void destroy();

	virtual void* lock();

	virtual void* lock(uint32_t vertexOffset, uint32_t vertexCount);
	
	virtual void unlock();

	virtual void prepare();

private:
	Ref< ContextDx11 > m_context;
	bool m_locked;

	VertexBufferDynamicDx11(uint32_t bufferSize);
};
	
	}
}

#endif	// traktor_render_VertexBufferDynamicDx11_H
