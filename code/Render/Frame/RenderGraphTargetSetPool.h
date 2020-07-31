#pragma once

#include "Core/RefArray.h"
#include "Core/Containers/AlignedVector.h"
#include "Render/Frame/RenderGraphTypes.h"

namespace traktor
{
	namespace render
	{

class IRenderSystem;
class IRenderTargetSet;

/*!
 * \ingroup Render
 */
class RenderGraphTargetSetPool
{
public:
	explicit RenderGraphTargetSetPool(IRenderSystem* renderSystem);

	/*! Acquire target from pool.
	 *
	 * \param targetSetDesc Description of target required.
	 * \param sharedDepthStencilTargetSet Optional shared depth/stencil target set.
	 * \param referenceWidth Reference width of target required.
	 * \param referenceHeight Reference height of target required.
	 * \param persistentHandle Persistent handle; used to track persistent targets in pool, 0 means not persistent target.
	 */
	IRenderTargetSet* acquire(
		const RenderGraphTargetSetDesc& targetSetDesc,
		IRenderTargetSet* sharedDepthStencilTargetSet,
		int32_t referenceWidth,
		int32_t referenceHeight,
		handle_t persistentHandle
	);

	/*! */
	void release(IRenderTargetSet* targetSet);

	/*! */
	void cleanup();

private:
	struct Pool
	{
		// Pool identification.
		RenderTargetSetCreateDesc rtscd;
		Ref< IRenderTargetSet > sharedDepthStencilTargetSet;
		handle_t persistentHandle;

		// Pool targets.
		RefArray< IRenderTargetSet > free;
		RefArray< IRenderTargetSet > acquired;
	};

	Ref< IRenderSystem > m_renderSystem;
	AlignedVector< Pool > m_pool;
};

	}
}
