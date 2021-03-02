#include "Physics/Editor/PhysicsComponentEditor.h"
#include "Physics/Editor/PhysicsComponentEditorFactory.h"
#include "Physics/World/JointComponentData.h"
#include "Physics/World/RigidBodyComponentData.h"
#include "Physics/World/Character/CharacterComponentData.h"
#include "Physics/World/Vehicle/VehicleComponentData.h"

namespace traktor
{
	namespace physics
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.physics.PhysicsComponentEditorFactory", PhysicsComponentEditorFactory, scene::IComponentEditorFactory)

const TypeInfoSet PhysicsComponentEditorFactory::getComponentDataTypes() const
{
	TypeInfoSet typeInfoSet;
	typeInfoSet.insert< CharacterComponentData >();
	typeInfoSet.insert< JointComponentData >();
	typeInfoSet.insert< RigidBodyComponentData >();
	typeInfoSet.insert< VehicleComponentData >();
	return typeInfoSet;
}

Ref< scene::IComponentEditor > PhysicsComponentEditorFactory::createComponentEditor(scene::SceneEditorContext* context, scene::EntityAdapter* entityAdapter, world::IEntityComponentData* componentData) const
{
	return new PhysicsComponentEditor(context, entityAdapter, componentData);
}

	}
}
