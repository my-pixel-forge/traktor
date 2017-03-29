#ifndef traktor_render_ImageProcess_H
#define traktor_render_ImageProcess_H

#include "Core/Object.h"
#include "Core/RefArray.h"
#include "Core/Containers/SmallMap.h"
#include "Core/Math/Color4f.h"
#include "Core/Math/Frustum.h"
#include "Core/Math/Matrix44.h"
#include "Core/Thread/Semaphore.h"
#include "Render/Types.h"
#include "Resource/Proxy.h"
#include "World/WorldTypes.h"
#include "Render/ImageProcess/ImageProcessStep.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_RENDER_EXPORT)
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

class ImageProcessSettings;
class ImageProcessTargetPool;
class IRenderSystem;
class IRenderView;
class ITexture;
class Shader;
class RenderTargetSet;
class ScreenRenderer;

/*! \brief Frame buffer post processing system.
 * \ingroup Render
 *
 * Predefined targets:
 * "Output" - Frame buffer, write only.
 * "InputColor" - Source color buffer, read only.
 * "InputDepth" - Source depth buffer, read only.
 * "InputShadowMask" - Source shadow mask, read only.
 */
class T_DLLCLASS ImageProcess : public Object
{
	T_RTTI_CLASS;

public:
	ImageProcess();

	bool create(
		const ImageProcessSettings* settings,
		ImageProcessTargetPool* targetPool,
		resource::IResourceManager* resourceManager,
		IRenderSystem* renderSystem,
		uint32_t width,
		uint32_t height,
		bool allTargetsPersistent
	);

	void destroy();

	bool render(
		IRenderView* renderView,
		RenderTargetSet* colorBuffer,
		RenderTargetSet* depthBuffer,
		RenderTargetSet* velocityBuffer,
		RenderTargetSet* shadowMask,
		const ImageProcessStep::Instance::RenderParams& params
	);

	void defineTarget(const std::wstring& name, handle_t id, const RenderTargetSetCreateDesc& rtscd, const Color4f& clearColor, bool persistent);

	void setTarget(IRenderView* renderView, handle_t id);

	RenderTargetSet* getTarget(handle_t id);

	void swapTargets(handle_t id0, handle_t id1);

	void discardTarget(handle_t id);

	void setCombination(handle_t handle, bool value);

	void setFloatParameter(handle_t handle, float value);

	void setVectorParameter(handle_t handle, const Vector4& value);

	void setTextureParameter(handle_t handle, const resource::Proxy< ITexture >& value);

	void prepareShader(Shader* shader) const;

	bool requireHighRange() const;

	void getDebugTargets(std::vector< DebugTarget >& outTargets) const;

private:
	struct Target
	{
		std::wstring name;
		RenderTargetSetCreateDesc rtscd;
		Ref< RenderTargetSet > rts;
		float clearColor[4];
		bool shouldClear;
		bool persistent;

		Target()
		:	shouldClear(false)
		,	persistent(false)
		{
			clearColor[0] =
			clearColor[1] =
			clearColor[2] =
			clearColor[3] = 0.0f;
		}
	};

	Ref< ScreenRenderer > m_screenRenderer;
	Ref< ImageProcessTargetPool > m_targetPool;
	SmallMap< handle_t, Target > m_targets;
	RefArray< ImageProcessStep::Instance > m_instances;
	SmallMap< handle_t, bool > m_booleanParameters;
	SmallMap< handle_t, float > m_scalarParameters;
	SmallMap< handle_t, Vector4 > m_vectorParameters;
	SmallMap< handle_t, resource::Proxy< ITexture > > m_textureParameters;
	Ref< RenderTargetSet > m_currentTarget;
	bool m_requireHighRange;
	bool m_allTargetsPersistent;
	Semaphore m_lock;
};

	}
}

#endif	// traktor_render_ImageProcess_H
