/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_render_TextureProxy_H
#define traktor_render_TextureProxy_H

#include "Render/ITexture.h"
#include "Resource/Proxy.h"

namespace traktor
{
	namespace render
	{

class TextureProxy : public ITexture
{
	T_RTTI_CLASS;

public:
	TextureProxy(const resource::Proxy< ITexture >& texture);

	virtual void destroy() override final;

	virtual ITexture* resolve() override final;

private:
	resource::Proxy< ITexture > m_texture;
};

	}
}

#endif	// traktor_render_TextureProxy_H
