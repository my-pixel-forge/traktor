/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_spray_PointSetResource_H
#define traktor_spray_PointSetResource_H

#include "Core/Serialization/ISerializable.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_SPRAY_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace spray
	{
	
class T_DLLCLASS PointSetResource : public ISerializable
{
	T_RTTI_CLASS;
	
public:
	virtual void serialize(ISerializer& s) override final;
};
	
	}
}

#endif	// traktor_spray_PointSetResource_H
