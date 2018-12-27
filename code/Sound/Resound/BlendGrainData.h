/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_sound_BlendGrainData_H
#define traktor_sound_BlendGrainData_H

#include "Sound/Types.h"
#include "Sound/Resound/IGrainData.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_SOUND_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace sound
	{

class T_DLLCLASS BlendGrainData : public IGrainData
{
	T_RTTI_CLASS;

public:
	BlendGrainData();

	virtual Ref< IGrain > createInstance(IGrainFactory* grainFactory) const override final;

	virtual void serialize(ISerializer& s) override final;

	const std::wstring& getId() const { return m_id; }

	const float getResponse() const { return m_response; }

	const Ref< IGrainData >* getGrains() const { return m_grains; }

private:
	std::wstring m_id;
	float m_response;
	Ref< IGrainData > m_grains[2];
};

	}
}

#endif	// traktor_sound_BlendGrainData_H
