#ifndef traktor_terrain_UndergrowthEntityData_H
#define traktor_terrain_UndergrowthEntityData_H

#include "Resource/Proxy.h"
#include "Terrain/UndergrowthEntity.h"
#include "World/Entity/EntityData.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_TERRAIN_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace resource
	{

class IResourceManager;

	}

	namespace render
	{

class IRenderSystem;
class Shader;

	}

	namespace terrain
	{

class Heightfield;
class MaterialMask;

class T_DLLCLASS UndergrowthEntityData : public world::EntityData
{
	T_RTTI_CLASS;

public:
	UndergrowthEntity* createEntity(resource::IResourceManager* resourceManager, render::IRenderSystem* renderSystem) const;

	virtual bool serialize(ISerializer& s);

	inline const resource::Proxy< Heightfield >& getHeightfield() const { return m_heightfield; }

	inline const resource::Proxy< MaterialMask >& getMaterialMask() const { return m_materialMask; }

	inline const resource::Proxy< render::Shader >& getShader() const { return m_shader; }

private:
	mutable resource::Proxy< Heightfield > m_heightfield;
	mutable resource::Proxy< MaterialMask > m_materialMask;
	mutable resource::Proxy< render::Shader > m_shader;
	UndergrowthEntity::Settings m_settings;
};

	}
}

#endif	// traktor_terrain_UndergrowthEntityData_H
