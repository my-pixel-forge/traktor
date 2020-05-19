#pragma once

#include "Core/Object.h"
#include "Core/Platform.h"
#include "Core/Math/Color4f.h"
#include "Render/Types.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_RENDER_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace render
	{

class IRenderSystem;
class IRenderTargetSet;
class VertexBuffer;
class IndexBuffer;
class ITexture;
class ISimpleTexture;
class ICubeTexture;
class IVolumeTexture;
class IProgram;

#define T_USE_RENDER_MARKERS

#if defined(T_USE_RENDER_MARKERS)
#	define T_RENDER_PUSH_MARKER(renderView, marker) \
	(renderView)->pushMarker(marker)
#	define T_RENDER_POP_MARKER(renderView) \
	(renderView)->popMarker()
#else
#	define T_RENDER_PUSH_MARKER(renderView, marker)
#	define T_RENDER_POP_MARKER(renderView)
#endif

/*! Render view.
 * \ingroup Render
 */
class T_DLLCLASS IRenderView : public Object
{
	T_RTTI_CLASS;

public:
	/*! \name View management. */
	//@{

	/*! Get next system event.
	 *
	 * When using non-embedded render view this must
	 * be called frequently in order to handle OS
	 * events. Significant events are translated into
	 * platform agnostic events, such as "toggle fullscreen
	 * requested", "window resized" etc.
	 *
	 * \return True if event returned; false if no event, ie. idle.
	 */
	virtual bool nextEvent(RenderEvent& outEvent) = 0;

	virtual void close() = 0;

	virtual bool reset(const RenderViewDefaultDesc& desc) = 0;

	virtual bool reset(int32_t width, int32_t height) = 0;

	virtual int getWidth() const = 0;

	virtual int getHeight() const = 0;

	virtual bool isActive() const = 0;

	virtual bool isMinimized() const = 0;

	virtual bool isFullScreen() const = 0;

	virtual void showCursor() = 0;

	virtual void hideCursor() = 0;

	virtual bool isCursorVisible() const = 0;

	virtual bool setGamma(float gamma) = 0;

	virtual void setViewport(const Viewport& viewport) = 0;

	virtual SystemWindow getSystemWindow() = 0;

	//@}

	/*! \name Rendering methods. */
	//@{

	/*! Begin rendering frame. */
	virtual bool beginFrame() = 0;

	/*! End rendering frame. */
	virtual void endFrame() = 0;

	/*! Swap back and front buffers. */
	virtual void present() = 0;

	/*! Begin rendering to back buffer.
	 *
	 * \param clear Optional clear parameters.
	 * \return True if successful.
	 */
	virtual bool beginPass(const Clear* clear) = 0;

	/*! Begin rendering to all render targets in set.
	 *
	 * \param renderTargetSet Set of render targets.
	 * \param clear Optional clear parameters.
	 * \param load Load flags.
	 * \param store Store flags.
	 * \return True if successful.
	 */
	virtual bool beginPass(IRenderTargetSet* renderTargetSet, const Clear* clear, uint32_t load, uint32_t store) = 0;

	/*! Begin rendering to a render target set.
	 *
	 * \param renderTargetSet Set of render targets.
	 * \param renderTarget Index of render target in set.
	 * \param clear Optional clear parameters.
	 * \param load Load flags.
	 * \param store Store flags.
	 * \return True if successful.
	 */
	virtual bool beginPass(IRenderTargetSet* renderTargetSet, int32_t renderTarget, const Clear* clear, uint32_t load, uint32_t store) = 0;

	/*! End rendering to pass. */
	virtual void endPass() = 0;

	/*! Draw primitives.
	 *
	 * \param vertexBuffer Vertex buffer containing geometry.
	 * \param indexBuffer Index buffer to be used; null if no indices should be used.
	 * \param program Program to be used.
	 * \param primitives Set of primitives to render.
	 */
	virtual void draw(VertexBuffer* vertexBuffer, IndexBuffer* indexBuffer, IProgram* program, const Primitives& primitives) = 0;

	/*! Draw primitives using instancing.
	 *
	 * \param vertexBuffer Vertex buffer containing geometry.
	 * \param indexBuffer Index buffer to be used; null if no indices should be used.
	 * \param program Program to be used.
	 * \param primitives Set of primitives to render.
	 * \param instanceCount Number of instances.
	 */
	virtual void draw(VertexBuffer* vertexBuffer, IndexBuffer* indexBuffer, IProgram* program, const Primitives& primitives, uint32_t instanceCount) = 0;

	/*! Enqueue compute task.
	 *
	 * \param workSize Work size, 3 dimensional size.
	 */
	virtual void compute(IProgram* program, const int32_t* workSize) = 0;

	/*! Copy texture.
	 *
	 * \param destinationTexture Destination texture.
	 * \param destinationRegion Destination region.
	 * \param sourceTexture Source texture.
	 * \param sourceRegion Source region.
	 */
	virtual bool copy(ITexture* destinationTexture, const Region& destinationRegion, ITexture* sourceTexture, const Region& sourceRegion) = 0;

	/*! \name Time queries. */
	//@{

	/*! Insert a beginning of time query into the recording command buffer. */
	virtual int32_t beginTimeQuery() = 0;

	/*! Insert an end of a time query into the recording command buffer. */
	virtual void endTimeQuery(int32_t query) = 0;

	/*! Get duration of a time query. */
	virtual bool getTimeQuery(int32_t query, bool wait, double& outDuration) const = 0;

	//@}

	/*! \name Statistics. */
	//@{

	/*! Write push debug marker to command buffer. */
	virtual void pushMarker(const char* const marker) = 0;

	/*! Write pop debug marker to command buffer. */
	virtual void popMarker() = 0;

	/*! Get render view statistics. */
	virtual void getStatistics(RenderViewStatistics& outStatistics) const = 0;

	/*! Get backbuffer content. */
	virtual bool getBackBufferContent(void* buffer) const = 0;

	//@}
};

	}
}
