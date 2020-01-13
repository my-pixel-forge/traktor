#pragma once

#include "Core/Object.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_WORLD_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace render
	{

class RenderContext;

	}

	namespace world
	{

class Entity;
class IWorldRenderPass;
class WorldEntityRenderers;
class WorldRenderView;

/*! World render thread context.
 * \ingroup World
 */
class T_DLLCLASS WorldContext : public Object
{
	T_RTTI_CLASS;

public:
	explicit WorldContext(WorldEntityRenderers* entityRenderers, render::RenderContext* renderContext, Entity* rootEntity);

	void build(WorldRenderView& worldRenderView, const IWorldRenderPass& worldRenderPass, Object* renderable);

	void flush(WorldRenderView& worldRenderView, const IWorldRenderPass& worldRenderPass);

	void flush();

	WorldEntityRenderers* getEntityRenderers() const { return m_entityRenderers; }

	render::RenderContext* getRenderContext() const { return m_renderContext; }

	Entity* getRootEntity() const { return m_rootEntity; }

private:
	WorldEntityRenderers* m_entityRenderers;
	render::RenderContext* m_renderContext;
	Entity* m_rootEntity;
};

	}
}

