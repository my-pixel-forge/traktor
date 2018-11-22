/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_world_WorldTypes_H
#define traktor_world_WorldTypes_H

#include "Core/Math/Vector4.h"

namespace traktor
{
	namespace render
	{

class ITexture;

	}

	namespace world
	{

enum
{
	MaxSliceCount = 4,
	MaxLightShadowCount = 2
};

enum CancelType
{
	CtImmediate = 0,
	CtEnd = 1
};

enum Quality
{
	QuDisabled = 0,
	QuLow = 1,
	QuMedium = 2,
	QuHigh = 3,
	QuUltra = 4,
	QuLast = 5
};

enum CameraType
{
	CtOrthographic = 0,
	CtPerspective = 1
};

enum LightType
{
	LtDisabled = 0,
	LtDirectional = 1,
	LtPoint = 2,
	LtSpot = 3,
	LtProbe = 4
};

struct Light
{
	LightType type;
	Vector4 position;
	Vector4 direction;
	Vector4 sunColor;
	Vector4 baseColor;
	Vector4 shadowColor;
	Scalar range;
	Scalar radius;
	render::ITexture* probeDiffuse;
	render::ITexture* cloudShadow;
	bool castShadow;
};

/*! \brief Update parameters. */
struct UpdateParams
{
	float totalTime;		/*! \brief Total time since first update. */
	float deltaTime;		/*! \brief Delta time since last update. */
	float alternateTime;	/*! \brief Alternative absolute time. */
};

	}
}

#endif	// traktor_world_WorldTypes_H
