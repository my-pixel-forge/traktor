/*
 * TRAKTOR
 * Copyright (c) 2024 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Ref.h"
#include "Core/RefArray.h"
#include "Core/Math/Transform.h"
#include "World/IWorldComponent.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_WORLD_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor::render
{

class Buffer;
class IAccelerationStructure;
class IProgram;
class IRenderSystem;

}

namespace traktor::world
{

class WorldBuildContext;

/*! Ray tracing world.
 * \ingroup World
 *
 * The RT world contains information about all instances
 * and ensure the top level (TLAS) structure is updated
 * as required.
 * 
 * The world renderer uses component's TLAS when
 * exposing to entity renderers during rasterization
 * passes.
 */
class T_DLLCLASS RTWorldComponent : public IWorldComponent
{
	T_RTTI_CLASS;

public:
	struct Part
	{
		Ref< const render::IAccelerationStructure > blas;
		Ref< const render::Buffer > perVertexData;
	};

	struct T_DLLCLASS Instance
	{
		RTWorldComponent* owner;
		Transform transform;
		AlignedVector< Part > parts;

		void destroy();

		/*! Set transform of instance, will automatically tag top level as dirty. */
		void setTransform(const Transform& transform);

		/*! Tag top level as dirty, might be necessary after bottom level has been updated. */
		void setDirty();
	};

	explicit RTWorldComponent(render::IRenderSystem* renderSystem);

	virtual void destroy() override final;

	virtual void update(World* world, const UpdateParams& update) override final;

	Instance* createInstance(const render::IAccelerationStructure* blas, const render::Buffer* perVertexData);

	Instance* createInstance(const AlignedVector< Part >& parts);

	void build(const WorldBuildContext& context);

	const render::IAccelerationStructure* getTopLevel() const { return m_tlas; }

private:
	Ref< render::IRenderSystem > m_renderSystem;
	Ref< render::IAccelerationStructure > m_tlas;
	AlignedVector< Instance* > m_instances;
	bool m_instanceBufferDirty = false;

	void destroyInstance(Instance* instance);
};

}
