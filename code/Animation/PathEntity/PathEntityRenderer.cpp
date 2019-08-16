#include "Animation/PathEntity/PathEntity.h"
#include "Animation/PathEntity/PathEntityRenderer.h"

namespace traktor
{
	namespace animation
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.animation.PathEntityRenderer", PathEntityRenderer, world::IEntityRenderer)

const TypeInfoSet PathEntityRenderer::getRenderableTypes() const
{
	return makeTypeInfoSet< PathEntity >();
}

void PathEntityRenderer::render(
	world::WorldContext& worldContext,
	world::WorldRenderView& worldRenderView,
	const world::IWorldRenderPass& worldRenderPass,
	Object* renderable
)
{
	PathEntity* pathEntity = checked_type_cast< PathEntity*, false >(renderable);
	pathEntity->render(worldContext, worldRenderView, worldRenderPass);
}

void PathEntityRenderer::flush(
	world::WorldContext& worldContext,
	world::WorldRenderView& worldRenderView,
	const world::IWorldRenderPass& worldRenderPass
)
{
}

	}
}
