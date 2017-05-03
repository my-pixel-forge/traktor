/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#include "Flash/FlashText.h"
#include "Flash/FlashTextInstance.h"
#include "Flash/Debug/TextInstanceDebugInfo.h"

namespace traktor
{
	namespace flash
	{
	
T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.flash.TextInstanceDebugInfo", 0, TextInstanceDebugInfo, InstanceDebugInfo)

TextInstanceDebugInfo::TextInstanceDebugInfo()
{
}

TextInstanceDebugInfo::TextInstanceDebugInfo(const FlashTextInstance* instance)
{
	m_name = instance->getName();
	m_bounds = instance->getText()->getTextBounds();
	m_transform = instance->getFullTransform();
}

void TextInstanceDebugInfo::serialize(ISerializer& s)
{
	InstanceDebugInfo::serialize(s);
}

	}
}
