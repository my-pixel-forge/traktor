/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_sound_MuteGrainFacade_H
#define traktor_sound_MuteGrainFacade_H

#include "Sound/Editor/Resound/IGrainFacade.h"

namespace traktor
{
	namespace sound
	{

class MuteGrainFacade : public IGrainFacade
{
	T_RTTI_CLASS;

public:
	virtual ui::Widget* createView(IGrainData* grain, ui::Widget* parent) override final;

	virtual int32_t getImage(const IGrainData* grain) const override final;

	virtual std::wstring getText(const IGrainData* grain) const override final;

	virtual bool getProperties(const IGrainData* grain, std::set< std::wstring >& outProperties) const override final;

	virtual bool canHaveChildren() const override final;

	virtual bool addChild(IGrainData* parentGrain, IGrainData* childGrain) override final;

	virtual bool removeChild(IGrainData* parentGrain, IGrainData* childGrain) override final;

	virtual bool getChildren(IGrainData* grain, RefArray< IGrainData >& outChildren) override final;
};

	}
}

#endif	// traktor_sound_MuteGrainFacade_H
