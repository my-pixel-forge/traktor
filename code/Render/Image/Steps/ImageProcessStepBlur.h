#pragma once

#include <vector>
#include "Core/Containers/AlignedVector.h"
#include "Render/Types.h"
#include "Resource/Id.h"
#include "Resource/Proxy.h"
#include "Render/Image/ImageProcessStep.h"

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

/*! Post blur step.
 * \ingroup Render
 */
class T_DLLCLASS ImageProcessStepBlur : public ImageProcessStep
{
	T_RTTI_CLASS;

public:
	struct Source
	{
		std::wstring param;		/*!< Shader parameter name. */
		std::wstring source;	/*!< Render target set source. */

		Source();

		void serialize(ISerializer& s);
	};

	class InstanceBlur : public Instance
	{
	public:
		struct Source
		{
			handle_t param;
			handle_t source;
		};

		InstanceBlur(
			const resource::Proxy< Shader >& shader,
			const std::vector< Source >& sources,
			const Vector4& direction,
			const AlignedVector< Vector4 >& gaussianOffsetWeights
		);

		virtual void destroy() override final;

		virtual void render(
			ImageProcess* imageProcess,
			RenderContext* renderContext,
			ProgramParameters* sharedParams,
			const RenderParams& params
		) override final;

	private:
		resource::Proxy< Shader > m_shader;
		std::vector< Source > m_sources;
		Vector4 m_direction;
		AlignedVector< Vector4 > m_gaussianOffsetWeights;
		handle_t m_handleGaussianOffsetWeights;
		handle_t m_handleDirection;
		handle_t m_handleViewFar;
		handle_t m_handleNoiseOffset;
	};

	ImageProcessStepBlur();

	virtual Ref< Instance > create(
		resource::IResourceManager* resourceManager,
		IRenderSystem* renderSystem,
		uint32_t width,
		uint32_t height
	) const override final;

	virtual void serialize(ISerializer& s) override final;

	const resource::Id< Shader >& getShader() const { return m_shader; }

	const std::vector< Source >& getSources() const { return m_sources; }

private:
	enum BlurType
	{
		// 1D separable
		BtGaussian,
		BtSine,
		BtBox,
		// 2D combined
		BtBox2D,
		BtCircle2D
	};

	resource::Id< Shader > m_shader;
	std::vector< Source > m_sources;
	Vector4 m_direction;
	int32_t m_taps;
	BlurType m_blurType;
};

	}
}

