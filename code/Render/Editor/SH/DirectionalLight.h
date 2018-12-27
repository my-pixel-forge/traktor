/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#pragma once

#include "Render/SH/SHFunction.h"

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

/*!
 * \brief Directional light function.
 */
class T_DLLCLASS DirectionalLight : public SHFunction
{
	T_RTTI_CLASS;

public:
	DirectionalLight(const Vector4& direction, float cutOffAngle);

	virtual float evaluate(float phi, float theta, const Vector4& unit) const override final;

private:
	Vector4 m_direction;
	float m_cutOffAngle;
};
		
	}
}
