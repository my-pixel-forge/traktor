/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_render_SequenceTexture_H
#define traktor_render_SequenceTexture_H

#include "Core/Containers/AlignedVector.h"
#include "Core/Timer/Timer.h"
#include "Render/ITexture.h"
#include "Resource/Proxy.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_RENDER_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace render
	{

/*! \brief Sequence, flipbook, texture.
 * \ingroup Render
 */
class T_DLLCLASS SequenceTexture : public ITexture
{
	T_RTTI_CLASS;

public:
	virtual void destroy() override final;

	virtual ITexture* resolve() override final;

private:
	friend class SequenceTextureFactory;

	Timer m_time;
	float m_rate;
	AlignedVector< resource::Proxy< ITexture > > m_textures;
};
	
	}
}

#endif	// traktor_render_SequenceTexture_H
