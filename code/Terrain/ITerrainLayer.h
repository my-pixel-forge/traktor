#pragma once

#include "Core/Object.h"

namespace traktor
{
	namespace world
	{

class IWorldRenderPass;
struct UpdateParams;
class WorldBuildContext;
class WorldRenderView;

	}

	namespace terrain
	{

class TerrainComponent;

class ITerrainLayer : public Object
{
	T_RTTI_CLASS;

public:
	virtual void update(const world::UpdateParams& update) = 0;

	virtual void build(
		TerrainComponent& terrainComponent,
		const world::WorldBuildContext& context,
		const world::WorldRenderView& worldRenderView,
		const world::IWorldRenderPass& worldRenderPass
	) = 0;

	virtual void updatePatches(const TerrainComponent& terrainComponent) = 0;
};

	}
}

