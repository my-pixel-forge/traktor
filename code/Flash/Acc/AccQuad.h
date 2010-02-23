#ifndef traktor_flash_AccQuad_H
#define traktor_flash_AccQuad_H

#include "Core/Object.h"
#include "Core/Math/Matrix33.h"
#include "Resource/Proxy.h"

namespace traktor
{
	namespace resource
	{

class IResourceManager;

	}

	namespace render
	{

class IRenderSystem;
class ITexture;
class RenderContext;
class Shader;
class VertexBuffer;

	}

	namespace flash
	{

struct SwfRect;
struct SwfCxTransform;

/*! \brief Simple unit-quad shape.
 * \ingroup Flash
 */
class AccQuad : public Object
{
public:
	bool create(
		resource::IResourceManager* resourceManager,
		render::IRenderSystem* renderSystem
	);

	void destroy();

	void render(
		render::RenderContext* renderContext,
		const Vector4& frameSize,
		const Vector4& viewSize,
		float scaleX,
		const Matrix33& transform,
		const SwfRect& bounds,
		const SwfCxTransform& cxform,
		render::ITexture* texture
	);

private:
	resource::Proxy< render::Shader > m_shaderTextured;
	Ref< render::VertexBuffer > m_vertexBuffer;
};

	}
}

#endif	// traktor_flash_AccQuad_H
