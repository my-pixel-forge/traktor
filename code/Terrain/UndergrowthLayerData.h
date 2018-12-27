/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_terrain_UndergrowthLayerData_H
#define traktor_terrain_UndergrowthLayerData_H

#include <vector>
#include "Resource/Id.h"
#include "Terrain/ITerrainLayerData.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_TERRAIN_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace terrain
	{

class T_DLLCLASS UndergrowthLayerData : public ITerrainLayerData
{
	T_RTTI_CLASS;

public:
	UndergrowthLayerData();

	virtual Ref< ITerrainLayer > createLayerInstance(
		resource::IResourceManager* resourceManager,
		render::IRenderSystem* renderSystem,
		const TerrainComponent& terrainComponent
	) const override final;

	virtual void serialize(ISerializer& s) override final;

private:
	friend class UndergrowthLayer;
	friend class TerrainEntityPipeline;

	struct Plant
	{
		uint8_t material;
		int32_t density;
		int32_t plant;
		float scale;

		Plant();

		void serialize(ISerializer& s);
	};

	resource::Id< render::Shader > m_shader;
	float m_spreadDistance;
	std::vector< Plant > m_plants;
};

	}
}

#endif	// traktor_terrain_UndergrowthLayerData_H
