#ifndef traktor_world_IWorldRenderer_H
#define traktor_world_IWorldRenderer_H

#include "Core/Object.h"
#include "Core/RefArray.h"
#include "Core/Math/Const.h"
#include "Render/Types.h"
#include "World/WorldTypes.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_WORLD_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{

class Color4f;

	namespace render
	{

class IRenderSystem;
class IRenderView;
class ITexture;
class RenderTargetSet;

	}

	namespace resource
	{

class IResourceManager;

	}

	namespace world
	{

class Entity;
class PostProcess;
class PostProcessSettings;
class WorldEntityRenderers;
class WorldRenderSettings;
class WorldRenderView;

/*! \brief World renderer creation description.
 * \ingroup World
 */
struct WorldCreateDesc
{
	const WorldRenderSettings* worldRenderSettings;
	const PostProcessSettings* postProcessSettings;
	WorldEntityRenderers* entityRenderers;
	Quality shadowsQuality;
	Quality ambientOcclusionQuality;
	Quality antiAliasQuality;
	uint32_t multiSample;
	uint32_t superSample;
	uint32_t frameCount;

	WorldCreateDesc()
	:	worldRenderSettings(0)
	,	postProcessSettings(0)
	,	entityRenderers(0)
	,	shadowsQuality(QuDisabled)
	,	ambientOcclusionQuality(QuDisabled)
	,	antiAliasQuality(QuDisabled)
	,	multiSample(0)
	,	superSample(0)
	,	frameCount(0)
	{
	}
};

/*! \brief Perspective view port.
 * \ingroup World
 */
struct WorldViewPerspective
{
	int32_t width;
	int32_t height;
	float aspect;
	float fov;

	WorldViewPerspective()
	:	width(0)
	,	height(0)
	,	aspect(1.0f)
	,	fov(deg2rad(65.0f))
	{
	}
};

/*! \brief Orthogonal view port.
 * \ingroup World
 */
struct WorldViewOrtho
{
	float width;
	float height;

	WorldViewOrtho()
	:	width(0.0f)
	,	height(0.0f)
	{
	}
};

/*! \brief World render flags.
 * \ingroup World
 */
enum WorldRenderFlags
{
	WrfDepthMap = 1,
	WrfNormalMap = 2,
	WrfShadowMap = 4,
	WrfLightMap = 8,
	WrfVisualOpaque = 16,
	WrfVisualAlphaBlend = 32
};

/*! \brief World renderer.
 * \ingroup World
 *
 * The world renderer is a high level renderer which render
 * entities through specialized entity renderer.
 * In order to maximize throughput the world renderer is designed with
 * threading and multiple cores in mind as the rendering is split
 * into two parts, one culling and collecting part and one actual rendering
 * part.
 */
class T_DLLCLASS IWorldRenderer : public Object
{
	T_RTTI_CLASS;

public:
	/*! \brief Create world renderer. */
	virtual bool create(
		resource::IResourceManager* resourceManager,
		render::IRenderSystem* renderSystem,
		render::IRenderView* renderView,
		const WorldCreateDesc& desc
	) = 0;

	/*! \brief Destroy world renderer. */
	virtual void destroy() = 0;

	/*! \brief Create a world render view.
	 *
	 * \param worldView World view.
	 * \param outRenderView Initialized world render view.
	 */
	virtual void createRenderView(const WorldViewPerspective& worldView, WorldRenderView& outRenderView) const = 0;

	/*! \brief Create a world render view.
	 *
	 * \param worldView World view.
	 * \param outRenderView Initialized world render view.
	 */
	virtual void createRenderView(const WorldViewOrtho& worldView, WorldRenderView& outRenderView) const = 0;

	/*! \name Build steps. */
	//@{

	/*! \brief Begin build "render contexts".
	 *
	 * \return True if building begun.
	 */
	virtual bool beginBuild() = 0;

	/*! \brief Build "render contexts".
	 *
	 * \param entity Root entity.
	 */
	virtual void build(Entity* entity) = 0;

	/*! \brief End build "render contexts".
	 *
	 * \param worldRenderView World render view.
	 * \param frame Multi threaded context frame.
	 */
	virtual void endBuild(WorldRenderView& worldRenderView, int frame) = 0;

	//@}

	/*! \name Render steps. */
	//@{

	/*! \brief Begin render "render contexts".
	 *
	 * \param frame Multi threaded context frame.
	 * \param eye Stereoscopic eye.
	 * \param clearColor Clear visual target color.
	 * \return True if rendering begun.
	 */
	virtual bool beginRender(int frame, render::EyeType eye, const Color4f& clearColor) = 0;

	/*! \brief Render "render contexts".
	 *
	 * \param flags Combination of world render flags.
	 * \param frame Multi threaded context frame.
	 * \param eye Stereoscopic eye.
	 */
	virtual void render(uint32_t flags, int frame, render::EyeType eye) = 0;

	/*! \brief End render "render contexts".
	 *
	 * \param frame Multi threaded context frame.
	 * \param eye Stereoscopic eye.
	 * \param deltaTime Last frame delta time.
	 */
	virtual void endRender(int frame, render::EyeType eye, float deltaTime) = 0;

	//@}

	/*! \name Target accessor. */
	//@{

	virtual PostProcess* getVisualPostProcess() = 0;

	virtual void getTargets(RefArray< render::ITexture >& outTargets) const = 0;

	//@}
};

	}
}

#endif	// traktor_world_IWorldRenderer_H
