#pragma once

#include "Core/Containers/AlignedVector.h"
#include "Render/Image2/ImagePassOp.h"
#include "Resource/Proxy.h"

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

class Shader;

/*!
 * \ingroup Render
 */
class T_DLLCLASS Simple : public ImagePassOp
{
	T_RTTI_CLASS;

public:
	virtual void setup(
		const ImageGraph* graph,
		const ImageGraphContext& context,
		RenderPass& pass
	) const override final;

	virtual void build(
		const ImageGraph* graph,
		const ImageGraphContext& context,
		const ImageGraphView& view,
		const RenderGraph& renderGraph,
		const ProgramParameters* sharedParams,
		RenderContext* renderContext,
		ScreenRenderer* screenRenderer
	) const override final;

private:
	friend class SimpleData;

	struct Source
	{
		handle_t textureId;
		handle_t parameter;
	};

	resource::Proxy< render::Shader > m_shader;
	AlignedVector< Source > m_sources;
};

	}
}