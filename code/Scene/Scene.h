#pragma once

#include "Core/Object.h"
#include "Core/Containers/SmallMap.h"
#include "Render/Types.h"
#include "Resource/Proxy.h"
#include "World/WorldTypes.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_SCENE_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace render
	{

class ITexture;

	}

	namespace world
	{

class Entity;
class EntitySchema;
class IWorldRenderer;
struct UpdateParams;
class WorldRenderSettings;
class WorldRenderView;

	}

	namespace scene
	{

class ISceneController;

/*! Scene
 * \ingroup Scene
 *
 * A scene holds information about entities,
 * world renderer configuration and post processing.
 */
class T_DLLCLASS Scene : public Object
{
	T_RTTI_CLASS;

public:
	Scene(
		ISceneController* controller,
		world::EntitySchema* entitySchema,
		world::Entity* rootEntity,
		world::WorldRenderSettings* worldRenderSettings,
		const SmallMap< render::handle_t, resource::Proxy< render::ITexture > >& imageProcessParams
	);

	explicit Scene(ISceneController* controller, Scene* scene);

	virtual ~Scene();

	void destroy();

	void updateController(const world::UpdateParams& update);

	void updateEntity(const world::UpdateParams& update);

	world::EntitySchema* getEntitySchema() const;

	world::Entity* getRootEntity() const;

	ISceneController* getController() const;

	world::WorldRenderSettings* getWorldRenderSettings() const;

	const SmallMap< render::handle_t, resource::Proxy< render::ITexture > >& getImageProcessParams() const;

private:
	Ref< world::EntitySchema > m_entitySchema;
	Ref< world::Entity > m_rootEntity;
	Ref< ISceneController > m_controller;
	Ref< world::WorldRenderSettings > m_worldRenderSettings;
	SmallMap< render::handle_t, resource::Proxy< render::ITexture > > m_imageProcessParams;
};

	}
}

