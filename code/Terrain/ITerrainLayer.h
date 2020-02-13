#pragma once

#include "Core/Object.h"

namespace traktor
{
	namespace world
	{

class IWorldRenderPass;
struct UpdateParams;
class WorldContext;
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
		const world::WorldContext& worldContext,
		const world::WorldRenderView& worldRenderView,
		const world::IWorldRenderPass& worldRenderPass
	) = 0;

	virtual void updatePatches(const TerrainComponent& terrainComponent) = 0;
};

	}
}

