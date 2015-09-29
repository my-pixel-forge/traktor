#include "Render/ImageProcess/ImageProcessStepTemporal.h"
#include "Render/Editor/ImageProcess/ImageProcessStepTemporalFacade.h"

namespace traktor
{
	namespace render
	{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.render.ImageProcessStepTemporalFacade", 0, ImageProcessStepTemporalFacade, IImageProcessStepFacade)

int32_t ImageProcessStepTemporalFacade::getImage(const ImageProcessStep* step) const
{
	return 5;
}

std::wstring ImageProcessStepTemporalFacade::getText(const ImageProcessStep* step) const
{
	return L"Temporal";
}

void ImageProcessStepTemporalFacade::getSources(const ImageProcessStep* step, std::vector< std::wstring >& outSources) const
{
	const ImageProcessStepTemporal* Temporal = checked_type_cast< const ImageProcessStepTemporal*, false >(step);
	for (std::vector< ImageProcessStepTemporal::Source >::const_iterator i = Temporal->getSources().begin(); i != Temporal->getSources().end(); ++i)
		outSources.push_back(i->source);
}

bool ImageProcessStepTemporalFacade::canHaveChildren() const
{
	return false;
}

bool ImageProcessStepTemporalFacade::addChild(ImageProcessStep* parentStep, ImageProcessStep* childStep) const
{
	return false;
}

bool ImageProcessStepTemporalFacade::removeChild(ImageProcessStep* parentStep, ImageProcessStep* childStep) const
{
	return false;
}

bool ImageProcessStepTemporalFacade::getChildren(const ImageProcessStep* step, RefArray< ImageProcessStep >& outChildren) const
{
	return false;
}

	}
}
