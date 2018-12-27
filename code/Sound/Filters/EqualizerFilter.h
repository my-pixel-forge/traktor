/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_sound_EqualizerFilter_H
#define traktor_sound_EqualizerFilter_H

#include "Sound/IFilter.h"

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

/*! \brief Equalizer filter.
 * \ingroup Sound
 */
class T_DLLCLASS EqualizerFilter : public IFilter
{
	T_RTTI_CLASS;

public:
	EqualizerFilter(float gain = 0.0f);

	virtual Ref< IFilterInstance > createInstance() const override final;

	virtual void apply(IFilterInstance* instance, SoundBlock& outBlock) const override final;

	virtual void serialize(ISerializer& s) override final;

private:
	float m_gain;
};

	}
}

#endif	// traktor_sound_EqualizerFilter_H
