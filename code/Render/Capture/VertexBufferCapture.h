/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_render_VertexBufferCapture_H
#define traktor_render_VertexBufferCapture_H

#include "Render/VertexBuffer.h"

namespace traktor
{
	namespace render
	{

/*!
 * \ingroup Capture
 */
class VertexBufferCapture : public VertexBuffer
{
	T_RTTI_CLASS;

public:
	VertexBufferCapture(VertexBuffer* vertexBuffer, uint32_t bufferSize, uint32_t vertexSize);

	virtual void destroy() override final;

	virtual void* lock() override final;

	virtual void* lock(uint32_t vertexOffset, uint32_t vertexCount) override final;

	virtual void unlock() override final;

	VertexBuffer* getVertexBuffer() const { return m_vertexBuffer; }

	uint32_t getVertexSize() const { return m_vertexSize; }

protected:
	Ref< VertexBuffer > m_vertexBuffer;
	uint32_t m_vertexSize;
	bool m_locked;
};
	
	}
}

#endif	// traktor_render_VertexBufferCapture_H

