/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_sound_RepeatGrain_H
#define traktor_sound_RepeatGrain_H

#include "Core/Ref.h"
#include "Core/Math/Random.h"
#include "Sound/Resound/IGrain.h"

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

/*! \brief
 * \ingroup Sound
 */
class T_DLLCLASS RepeatGrain : public IGrain
{
	T_RTTI_CLASS;

public:
	RepeatGrain(
		uint32_t count,
		IGrain* grain
	);

	virtual Ref< ISoundBufferCursor > createCursor() const override final;

	virtual void updateCursor(ISoundBufferCursor* cursor) const override final;

	virtual const IGrain* getCurrentGrain(const ISoundBufferCursor* cursor) const override final;

	virtual void getActiveGrains(const ISoundBufferCursor* cursor, RefArray< const IGrain >& outActiveGrains) const override final;

	virtual bool getBlock(ISoundBufferCursor* cursor, const ISoundMixer* mixer, SoundBlock& outBlock) const override final;

private:
	uint32_t m_count;
	Ref< IGrain > m_grain;
};

	}
}

#endif	// traktor_sound_RepeatGrain_H
