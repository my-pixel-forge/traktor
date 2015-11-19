#ifndef traktor_flash_AccDisplayRenderer_H
#define traktor_flash_AccDisplayRenderer_H

#include "Core/RefArray.h"
#include "Core/Containers/SmallMap.h"
#include "Flash/IDisplayRenderer.h"
#include "Render/Types.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_FLASH_EXPORT)
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
class IRenderView;
class RenderContext;
class RenderTargetSet;

	}

	namespace flash
	{

class AccGlyph;
class AccQuad;
class AccShape;
class AccShapeRenderer;
class AccShapeResources;
class AccShapeVertexPool;
class AccTextureCache;

/*! \brief Accelerated display renderer.
 * \ingroup Flash
 *
 * This display renderer uses the render system
 * in order to accelerate rendering of SWF shapes.
 */
class T_DLLCLASS AccDisplayRenderer : public IDisplayRenderer
{
	T_RTTI_CLASS;

public:
	AccDisplayRenderer();

	virtual ~AccDisplayRenderer();

	bool create(
		resource::IResourceManager* resourceManager,
		render::IRenderSystem* renderSystem,
		uint32_t frameCount,
		uint32_t renderContextSize,
		bool clearBackground,
		float stereoscopicOffset
	);

	void destroy();

	void build(uint32_t frame);

	void build(render::RenderContext* renderContext, uint32_t frame);

	void render(render::IRenderView* renderView, uint32_t frame, render::EyeType eye, const Vector2& offset, float scale);

	void flush();

	void flushCaches();

	// \name IDisplayRenderer
	// \{
	
	virtual void begin(
		const FlashDictionary& dictionary,
		const SwfColor& backgroundColor,
		const Aabb2& frameBounds,
		float viewWidth,
		float viewHeight,
		const Vector4& viewOffset
	) T_OVERRIDE T_FINAL;

	virtual void beginSprite(const FlashSpriteInstance& sprite, const Matrix33& transform) T_OVERRIDE T_FINAL;

	virtual void endSprite(const FlashSpriteInstance& sprite, const Matrix33& transform) T_OVERRIDE T_FINAL;

	virtual void beginMask(bool increment) T_OVERRIDE T_FINAL;

	virtual void endMask() T_OVERRIDE T_FINAL;

	virtual void renderShape(const FlashDictionary& dictionary, const Matrix33& transform, const FlashShape& shape, const SwfCxTransform& cxform, uint8_t blendMode) T_OVERRIDE T_FINAL;

	virtual void renderMorphShape(const FlashDictionary& dictionary, const Matrix33& transform, const FlashMorphShape& shape, const SwfCxTransform& cxform) T_OVERRIDE T_FINAL;

	virtual void renderGlyph(const FlashDictionary& dictionary, const Matrix33& transform, const Vector2& fontMaxDimension, const FlashShape& shape, const SwfColor& color, const SwfCxTransform& cxform, uint8_t filter, const SwfColor& filterColor) T_OVERRIDE T_FINAL;

	virtual void renderQuad(const Matrix33& transform, const Aabb2& bounds, const SwfCxTransform& cxform) T_OVERRIDE T_FINAL;

	virtual void renderCanvas(const FlashDictionary& dictionary, const Matrix33& transform, const FlashCanvas& canvas, const SwfCxTransform& cxform) T_OVERRIDE T_FINAL;

	virtual void end() T_OVERRIDE T_FINAL;

	// \}

private:
	struct ShapeCache
	{
		Ref< AccShape > shape;
		int32_t unusedCount;
		int32_t tag;
	};

	struct GlyphCache
	{
		Ref< AccShape > shape;
		int32_t index;
	};

	resource::IResourceManager* m_resourceManager;
	render::IRenderSystem* m_renderSystem;
	RefArray< render::RenderContext > m_renderContexts;
	Ref< render::RenderContext > m_renderContext;
	Ref< render::RenderContext > m_globalContext;
	Ref< render::RenderTargetSet > m_renderTargetGlyphs;
	Ref< AccShapeResources > m_shapeResources;
	Ref< AccShapeVertexPool > m_vertexPool;
	Ref< AccShapeRenderer > m_shapeRenderer;
	Ref< AccTextureCache > m_textureCache;
	Ref< AccGlyph > m_glyph;
	Ref< AccQuad > m_quad;
	SmallMap< int32_t, ShapeCache > m_shapeCache;
	SmallMap< int32_t, GlyphCache > m_glyphCache;
	int32_t m_nextIndex;
	Vector4 m_frameSize;
	Vector4 m_viewSize;
	Vector4 m_viewOffset;
	bool m_clearBackground;
	float m_stereoscopicOffset;
	bool m_maskWrite;
	bool m_maskIncrement;
	uint8_t m_maskReference;
	uint8_t m_glyphFilter;
	SwfColor m_glyphColor;
	SwfColor m_glyphFilterColor;
	render::handle_t m_handleScreenOffset;
	int32_t m_cacheAsBitmapDepth;

	void renderEnqueuedGlyphs();
};

	}
}

#endif	// traktor_flash_AccDisplayRenderer_H
