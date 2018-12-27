/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_render_TextureAsset_H
#define traktor_render_TextureAsset_H

#include "Editor/Asset.h"
#include "Render/Editor/Texture/TextureOutput.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_RENDER_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace render
	{

class T_DLLCLASS TextureAsset : public editor::Asset
{
	T_RTTI_CLASS;

public:
	TextureOutput m_output;

	virtual void serialize(ISerializer& s) override final;
};

	}
}

#endif	// traktor_render_TextureAsset_H
