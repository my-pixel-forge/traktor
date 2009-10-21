#include "Scene/Editor/DefaultEditorProfile.h"
#include "Scene/Editor/SceneEditorContext.h"
#include "Core/Serialization/Serializable.h"
#include "Core/Heap/GcNew.h"
#include "Ui/Command.h"

// Resource factories
#include "Render/TextureFactory.h"
#include "Render/ShaderFactory.h"
#include "World/PostProcess/PostProcessFactory.h"
#include "Mesh/Static/StaticMeshFactory.h"
#include "Mesh/Skinned/SkinnedMeshFactory.h"
#include "Mesh/Indoor/IndoorMeshFactory.h"
#include "Mesh/Instance/InstanceMeshFactory.h"
#include "Mesh/Blend/BlendMeshFactory.h"
#include "Weather/Clouds/CloudMaskFactory.h"

// Entity factories
#include "World/Entity/LightEntityFactory.h"
#include "World/Entity/ExternalEntityFactory.h"
#include "World/Entity/GroupEntityFactory.h"
#include "Mesh/MeshEntityFactory.h"
#include "Weather/WeatherEntityFactory.h"

// Entity renderers
#include "World/Entity/LightEntityRenderer.h"
#include "World/Entity/GroupEntityRenderer.h"
#include "Weather/WeatherEntityRenderer.h"
#include "Mesh/MeshEntityRenderer.h"
#include "Mesh/Instance/InstanceMeshEntityRenderer.h"

// Entity editor factories
#include "Scene/Editor/DefaultEntityEditorFactory.h"

namespace traktor
{
	namespace scene
	{

T_IMPLEMENT_RTTI_SERIALIZABLE_CLASS(L"traktor.scene.DefaultEditorProfile", DefaultEditorProfile, ISceneEditorProfile)

void DefaultEditorProfile::getCommands(
	std::list< ui::Command >& outCommands
) const
{
}

void DefaultEditorProfile::createEditorPlugins(
	SceneEditorContext* context,
	RefArray< ISceneEditorPlugin >& outEditorPlugins
) const
{
}

void DefaultEditorProfile::createResourceFactories(
	SceneEditorContext* context,
	RefArray< resource::IResourceFactory >& outResourceFactories
) const
{
	outResourceFactories.push_back(gc_new< render::TextureFactory >(context->getResourceDatabase(), context->getRenderSystem()));
	outResourceFactories.push_back(gc_new< render::ShaderFactory >(context->getResourceDatabase(), context->getRenderSystem()));
	outResourceFactories.push_back(gc_new< world::PostProcessFactory >(context->getResourceDatabase()));
	outResourceFactories.push_back(gc_new< mesh::StaticMeshFactory >(context->getResourceDatabase(), context->getRenderSystem()));
	outResourceFactories.push_back(gc_new< mesh::SkinnedMeshFactory >(context->getResourceDatabase(), context->getRenderSystem()));
	outResourceFactories.push_back(gc_new< mesh::IndoorMeshFactory >(context->getResourceDatabase(), context->getRenderSystem()));
	outResourceFactories.push_back(gc_new< mesh::InstanceMeshFactory >(context->getResourceDatabase(), context->getRenderSystem()));
	outResourceFactories.push_back(gc_new< mesh::BlendMeshFactory >(context->getResourceDatabase(), context->getRenderSystem()));
	outResourceFactories.push_back(gc_new< weather::CloudMaskFactory >(context->getResourceDatabase()));
}

void DefaultEditorProfile::createEntityFactories(
	SceneEditorContext* context,
	RefArray< world::IEntityFactory >& outEntityFactories
) const
{
	outEntityFactories.push_back(gc_new< world::LightEntityFactory >());
	outEntityFactories.push_back(gc_new< world::ExternalEntityFactory >(context->getSourceDatabase()));
	outEntityFactories.push_back(gc_new< world::GroupEntityFactory >());
	outEntityFactories.push_back(gc_new< mesh::MeshEntityFactory >(context->getResourceManager()));
	outEntityFactories.push_back(gc_new< weather::WeatherEntityFactory >(context->getResourceManager(), context->getRenderSystem()));
}

void DefaultEditorProfile::createEntityRenderers(
	SceneEditorContext* context,
	render::IRenderView* renderView,
	render::PrimitiveRenderer* primitiveRenderer,
	RefArray< world::IEntityRenderer >& outEntityRenderers
) const
{
	outEntityRenderers.push_back(gc_new< world::LightEntityRenderer >());
	outEntityRenderers.push_back(gc_new< world::GroupEntityRenderer >());
	outEntityRenderers.push_back(gc_new< weather::WeatherEntityRenderer >(primitiveRenderer));
	outEntityRenderers.push_back(gc_new< mesh::MeshEntityRenderer >());
	outEntityRenderers.push_back(gc_new< mesh::InstanceMeshEntityRenderer >());
}

void DefaultEditorProfile::createControllerEditorFactories(
	scene::SceneEditorContext* context,
	RefArray< scene::ISceneControllerEditorFactory >& outControllerEditorFactories
) const
{
}

void DefaultEditorProfile::createEntityEditorFactories(
	SceneEditorContext* context,
	RefArray< IEntityEditorFactory >& outEntityEditorFactories
) const
{
	outEntityEditorFactories.push_back(gc_new< DefaultEntityEditorFactory >());
}

	}
}
