/*
 * TRAKTOR
 * Copyright (c) 2022-2024 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "World/AbstractEntityFactory.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_WORLD_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor::world
{

/*!
 * \ingroup World
 */
class T_DLLCLASS WorldEditorEntityFactory : public AbstractEntityFactory
{
	T_RTTI_CLASS;

public:
	virtual const TypeInfoSet getEntityComponentTypes() const override final;

	virtual Ref< IEntityComponent > createEntityComponent(const world::IEntityBuilder* builder, const IEntityComponentData& entityComponentData) const override final;
};

}
