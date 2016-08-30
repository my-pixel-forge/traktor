#include "Core/Class/AutoRuntimeClass.h"
#include "Core/Class/IRuntimeClassRegistrar.h"
#include "Render/IRenderView.h"
#include "Render/ITexture.h"
#include "Render/ICubeTexture.h"
#include "Render/ISimpleTexture.h"
#include "Render/IVolumeTexture.h"
#include "Render/RenderClassFactory.h"
#include "Render/ImageProcess/ImageProcess.h"

namespace traktor
{
	namespace render
	{
		namespace
		{

void ImageProcess_setCombination(ImageProcess* this_, const std::wstring& name, bool value)
{
	this_->setCombination(render::getParameterHandle(name), value);
}

void ImageProcess_setParameter(ImageProcess* this_, const std::wstring& name, float value)
{
	this_->setFloatParameter(render::getParameterHandle(name), value);
}

		}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.render.RenderClassFactory", 0, RenderClassFactory, IRuntimeClassFactory)

void RenderClassFactory::createClasses(IRuntimeClassRegistrar* registrar) const
{
	Ref< AutoRuntimeClass< ITexture > > classITexture = new AutoRuntimeClass< ITexture >();
	registrar->registerClass(classITexture);

	Ref< AutoRuntimeClass< ICubeTexture > > classICubeTexture = new AutoRuntimeClass< ICubeTexture >();
	classICubeTexture->addMethod("getWidth", &ICubeTexture::getWidth);
	classICubeTexture->addMethod("getHeight", &ICubeTexture::getHeight);
	classICubeTexture->addMethod("getDepth", &ICubeTexture::getDepth);
	registrar->registerClass(classICubeTexture);

	Ref< AutoRuntimeClass< ISimpleTexture > > classISimpleTexture = new AutoRuntimeClass< ISimpleTexture >();
	classISimpleTexture->addMethod("getWidth", &ISimpleTexture::getWidth);
	classISimpleTexture->addMethod("getHeight", &ISimpleTexture::getHeight);
	registrar->registerClass(classISimpleTexture);

	Ref< AutoRuntimeClass< IVolumeTexture > > classIVolumeTexture = new AutoRuntimeClass< IVolumeTexture >();
	classIVolumeTexture->addMethod("getWidth", &IVolumeTexture::getWidth);
	classIVolumeTexture->addMethod("getHeight", &IVolumeTexture::getHeight);
	classIVolumeTexture->addMethod("getDepth", &IVolumeTexture::getDepth);
	registrar->registerClass(classIVolumeTexture);

	Ref< AutoRuntimeClass< IRenderView > > classIRenderView = new AutoRuntimeClass< IRenderView >();
	classIRenderView->addMethod("close", &IRenderView::close);
	classIRenderView->addMethod("getWidth", &IRenderView::getWidth);
	classIRenderView->addMethod("getHeight", &IRenderView::getHeight);
	classIRenderView->addMethod("isActive", &IRenderView::isActive);
	classIRenderView->addMethod("isMinimized", &IRenderView::isMinimized);
	classIRenderView->addMethod("isFullScreen", &IRenderView::isFullScreen);
	classIRenderView->addMethod("showCursor", &IRenderView::showCursor);
	classIRenderView->addMethod("hideCursor", &IRenderView::hideCursor);
	classIRenderView->addMethod("isCursorVisible", &IRenderView::isCursorVisible);
	classIRenderView->addMethod("setGamma", &IRenderView::setGamma);
	registrar->registerClass(classIRenderView);

	Ref< AutoRuntimeClass< ImageProcess > > classImageProcess = new AutoRuntimeClass< ImageProcess >();
	classImageProcess->addMethod("setCombination", &ImageProcess_setCombination);
	classImageProcess->addMethod("setParameter", &ImageProcess_setParameter);
	classImageProcess->addMethod("requireHighRange", &ImageProcess::requireHighRange);
	registrar->registerClass(classImageProcess);
}

	}
}
