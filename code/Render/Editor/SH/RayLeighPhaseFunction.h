/*
 * TRAKTOR
 * Copyright (c) 2022 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
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

namespace traktor::render
{

/*!
 * Ray-Leigh scattering phase function.
 */
class T_DLLCLASS RayLeighPhaseFunction : public SHFunction
{
	T_RTTI_CLASS;

public:
	virtual Vector4 evaluate(const Polar& direction) const override final;
};

}
