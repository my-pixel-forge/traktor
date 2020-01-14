#pragma once

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

/*! Repeat step any number of times.
 * \ingroup Render
 */
class T_DLLCLASS ImageProcessStepRepeat : public ImageProcessStep
{
	T_RTTI_CLASS;

public:
	class InstanceRepeat : public Instance
	{
	public:
		InstanceRepeat(uint32_t count, Instance* instance);

		virtual void destroy() override final;

		virtual void build(
			ImageProcess* imageProcess,
			RenderContext* renderContext,
			ProgramParameters* sharedParams,
			const RenderParams& params
		) override final;

	private:
		uint32_t m_count;
		Ref< Instance > m_instance;
	};

	ImageProcessStepRepeat();

	virtual Ref< Instance > create(
		resource::IResourceManager* resourceManager,
		IRenderSystem* renderSystem,
		uint32_t width,
		uint32_t height
	) const override final;

	virtual void serialize(ISerializer& s) override final;

	const Ref< ImageProcessStep >& getStep() const { return m_step; }

private:
	uint32_t m_count;
	Ref< ImageProcessStep > m_step;
};

	}
}

