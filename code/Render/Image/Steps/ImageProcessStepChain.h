#pragma once

#include "Core/RefArray.h"
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

/*! Chain of sub-steps.
 * \ingroup Render
 */
class T_DLLCLASS ImageProcessStepChain : public ImageProcessStep
{
	T_RTTI_CLASS;

public:
	class InstanceChain : public Instance
	{
	public:
		InstanceChain(const RefArray< Instance >& instances);

		virtual void destroy() override final;

		virtual void build(
			ImageProcess* imageProcess,
			RenderContext* renderContext,
			ProgramParameters* sharedParams,
			const RenderParams& params
		) override final;

	private:
		RefArray< Instance > m_instances;
	};

	virtual Ref< Instance > create(
		resource::IResourceManager* resourceManager,
		IRenderSystem* renderSystem,
		uint32_t width,
		uint32_t height
	) const override final;

	virtual void serialize(ISerializer& s) override final;

	inline const RefArray< ImageProcessStep >& getSteps() const { return m_steps; }

private:
	RefArray< ImageProcessStep > m_steps;
};

	}
}

