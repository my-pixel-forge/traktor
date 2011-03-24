#ifndef traktor_animation_ClothEntityFactory_H
#define traktor_animation_ClothEntityFactory_H

#include "World/Entity/IEntityFactory.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_ANIMATION_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace render
	{

class IRenderSystem;

	}

	namespace resource
	{

class IResourceManager;

	}

	namespace physics
	{

class PhysicsManager;

	}

	namespace animation
	{

/*! \brief Cloth entity factory.
 * \ingroup Animation
 */
class T_DLLCLASS ClothEntityFactory : public world::IEntityFactory
{
	T_RTTI_CLASS;

public:
	ClothEntityFactory(resource::IResourceManager* resourceManager, render::IRenderSystem* renderSystem, physics::PhysicsManager* physicsManager);

	virtual const TypeInfoSet getEntityTypes() const;

	virtual Ref< world::Entity > createEntity(world::IEntityBuilder* builder, const world::EntityData& entityData) const;

private:
	resource::IResourceManager* m_resourceManager;
	render::IRenderSystem* m_renderSystem;
	physics::PhysicsManager* m_physicsManager;
};

	}
}

#endif	// traktor_animation_ClothEntityFactory_H
