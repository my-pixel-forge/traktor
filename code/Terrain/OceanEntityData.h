#ifndef traktor_terrain_OceanEntityData_H
#define traktor_terrain_OceanEntityData_H

#include "Core/Math/Color4f.h"
#include "Core/Math/Vector2.h"
#include "Resource/Id.h"
#include "World/EntityData.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_TERRAIN_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace render
	{

class IRenderSystem;
class ITexture;
class Shader;

	}

	namespace terrain
	{

class OceanEntity;

class T_DLLCLASS OceanEntityData : public world::EntityData
{
	T_RTTI_CLASS;

public:
	OceanEntityData();

	virtual void serialize(ISerializer& s);

	const resource::Id< render::Shader >& getShader() const { return m_shader; }

	const resource::Id< render::ITexture >& getReflectionMap() const { return m_reflectionMap; }

private:
	friend class OceanEntity;

	struct Wave
	{
		Vector2 center;
		float amplitude;
		float frequency;
		float phase;
		float pinch;
		float rate;

		Wave();

		void serialize(ISerializer& s);
	};

	resource::Id< render::Shader > m_shader;
	resource::Id< render::ITexture > m_reflectionMap;
	Color4f m_shallowTint;
	Color4f m_reflectionTint;
	Color4f m_deepColor;
	float m_opacity;
	bool m_allowSSReflections;
	Wave m_waves[4];
};

	}
}

#endif	// traktor_terrain_OceanEntityData_H
