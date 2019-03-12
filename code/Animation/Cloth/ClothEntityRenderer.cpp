#include "Animation/Cloth/ClothEntity.h"
#include "Animation/Cloth/ClothEntityRenderer.h"

namespace traktor
{
	namespace animation
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.animation.ClothEntityRenderer", ClothEntityRenderer, world::IEntityRenderer)

const TypeInfoSet ClothEntityRenderer::getRenderableTypes() const
{
	TypeInfoSet typeSet;
	typeSet.insert< ClothEntity >();
	return typeSet;
}

void ClothEntityRenderer::render(
	world::WorldContext& worldContext,
	world::WorldRenderView& worldRenderView,
	world::IWorldRenderPass& worldRenderPass,
	Object* renderable
)
{
	if (ClothEntity* clothEntity = dynamic_type_cast< ClothEntity* >(renderable))
		clothEntity->render(worldContext, worldRenderView, worldRenderPass);
}

void ClothEntityRenderer::flush(
	world::WorldContext& worldContext,
	world::WorldRenderView& worldRenderView,
	world::IWorldRenderPass& worldRenderPass
)
{
}

	}
}
