#include "Core/Class/AutoRuntimeClass.h"
#include "Core/Class/BoxedAllocator.h"
#include "Core/Class/IRuntimeClassRegistrar.h"
#include "Core/Class/Boxes/BoxedAlignedVector.h"
#include "Core/Class/Boxes/BoxedMatrix44.h"
#include "Core/Class/Boxes/BoxedPointer.h"
#include "Core/Class/Boxes/BoxedRefArray.h"
#include "Core/Class/Boxes/BoxedVector4.h"
#include "Render/IRenderSystem.h"
#include "Render/IRenderView.h"
#include "Render/ITexture.h"
#include "Render/ICubeTexture.h"
#include "Render/ISimpleTexture.h"
#include "Render/IVolumeTexture.h"
#include "Render/RenderClassFactory.h"
#include "Render/StructBuffer.h"
#include "Render/StructElement.h"
#include "Render/Context/ProgramParameters.h"

namespace traktor
{
	namespace render
	{
		namespace
		{

class BoxedDataType : public Object
{
	T_RTTI_CLASS;
};

T_IMPLEMENT_RTTI_CLASS(L"traktor.render.DataType", BoxedDataType, Object)

class BoxedDisplayMode : public Object
{
	T_RTTI_CLASS;

public:
	BoxedDisplayMode(const DisplayMode& displayMode)
	:	m_displayMode(displayMode)
	{
	}

	uint32_t getWidth() const { return m_displayMode.width; }

	uint32_t getHeight() const { return m_displayMode.height; }

	uint16_t getRefreshRate() const { return m_displayMode.refreshRate; }

	uint16_t getColorBits() const { return m_displayMode.colorBits; }

private:
	DisplayMode m_displayMode;
};

T_IMPLEMENT_RTTI_CLASS(L"traktor.render.DisplayMode", BoxedDisplayMode, Object)

class BoxedStructElement : public Object
{
	T_RTTI_CLASS;

public:
	BoxedStructElement() = default;

	BoxedStructElement(const StructElement& structElement)
	:	m_value(structElement)
	{
	}

	explicit BoxedStructElement(uint32_t dataType, uint32_t offset)
	:	m_value((DataType)dataType, offset)
	{
	}

	uint32_t getSize() const { return m_value.getSize(); }

	uint32_t getDataType() const { return (uint32_t)m_value.getDataType(); }

	uint32_t getOffset() const { return m_value.getOffset(); }

	const StructElement& unbox() const { return m_value; }

private:
	StructElement m_value;
};

T_IMPLEMENT_RTTI_CLASS(L"traktor.render.StructElement", BoxedStructElement, Object)

class BoxedSimpleTextureCreateDesc : public Object
{
	T_RTTI_CLASS;

public:
	BoxedSimpleTextureCreateDesc()
	{
		// \fixme Expose texture formats to script.
		m_value.format = TfR8G8B8A8;
	}

	BoxedSimpleTextureCreateDesc(const SimpleTextureCreateDesc& stcd)
	:	m_value(stcd)
	{
	}

	void setWidth(int32_t width) { m_value.width = width; }

	int32_t getWidth() const { return m_value.width; }

	void setHeight(int32_t height) { m_value.height = height; }

	int32_t getHeight() const { return m_value.height; }

	void setMipCount(int32_t mipCount) { m_value.mipCount = mipCount; }

	int32_t getMipCount() const { return m_value.mipCount; }

	void set_sRGB(bool sRGB) { m_value.sRGB = sRGB; }

	bool get_sRGB() const { return m_value.sRGB; }

	void setImmutable(bool immutable) { m_value.immutable = immutable; }

	bool getImmutable() const { return m_value.immutable; }

	const SimpleTextureCreateDesc& unbox() const { return m_value; }

private:
	SimpleTextureCreateDesc m_value;
};

T_IMPLEMENT_RTTI_CLASS(L"traktor.render.SimpleTextureCreateDesc", BoxedSimpleTextureCreateDesc, Object)

BoxedAllocator< BoxedProgramParameters, 4096 > s_allocBoxedProgramParameters;

Ref< BoxedPointer > StructBuffer_lock(StructBuffer* self)
{
	void* ptr = self->lock();
	if (ptr)
		return new BoxedPointer(ptr);
	else
		return nullptr;
}

Ref< BoxedPointer > ISimpleTexture_lock(ISimpleTexture* self, int32_t level)
{
	ITexture::Lock lock;
	if (self->lock(level, lock))
		return new BoxedPointer(lock.bits);
	else
		return nullptr;
}

handle_t IRenderSystem_getHandle(const std::wstring& id)
{
	return getParameterHandle(id);
}

Ref< BoxedDisplayMode > IRenderSystem_getCurrentDisplayMode(IRenderSystem* self)
{
	return new BoxedDisplayMode(self->getCurrentDisplayMode());
}

Ref< BoxedDisplayMode > IRenderSystem_getDisplayMode(IRenderSystem* self, uint32_t index)
{
	return new BoxedDisplayMode(self->getDisplayMode(index));
}

Ref< StructBuffer > IRenderSystem_createStructBuffer(IRenderSystem* self, const RefArray< BoxedStructElement >& structElements, uint32_t bufferSize, bool dynamic)
{
	AlignedVector< StructElement > se;

	se.resize(structElements.size());
	for (size_t i = 0; i < structElements.size(); ++i)
		se[i] = structElements[i]->unbox();

	return self->createStructBuffer(se, bufferSize, dynamic);
}

Ref< ISimpleTexture > IRenderSystem_createSimpleTexture(IRenderSystem* self, const BoxedSimpleTextureCreateDesc* stcd)
{
	return self->createSimpleTexture(stcd->unbox(), T_FILE_LINE_W);
}

		}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.render.RenderClassFactory", 0, RenderClassFactory, IRuntimeClassFactory)

void RenderClassFactory::createClasses(IRuntimeClassRegistrar* registrar) const
{
	auto classBoxedDataType = new AutoRuntimeClass< BoxedDataType >();
	classBoxedDataType->addConstant("DtFloat1", Any::fromInt32(DtFloat1));
	classBoxedDataType->addConstant("DtFloat2", Any::fromInt32(DtFloat2));
	classBoxedDataType->addConstant("DtFloat3", Any::fromInt32(DtFloat3));
	classBoxedDataType->addConstant("DtFloat4", Any::fromInt32(DtFloat4));
	classBoxedDataType->addConstant("DtByte4", Any::fromInt32(DtByte4));
	classBoxedDataType->addConstant("DtByte4N", Any::fromInt32(DtByte4N));
	classBoxedDataType->addConstant("DtShort2", Any::fromInt32(DtShort2));
	classBoxedDataType->addConstant("DtShort4", Any::fromInt32(DtShort4));
	classBoxedDataType->addConstant("DtShort2N", Any::fromInt32(DtShort2N));
	classBoxedDataType->addConstant("DtShort4N", Any::fromInt32(DtShort4N));
	classBoxedDataType->addConstant("DtHalf2", Any::fromInt32(DtHalf2));
	classBoxedDataType->addConstant("DtHalf4", Any::fromInt32(DtHalf4));
	classBoxedDataType->addConstant("DtInteger1", Any::fromInt32(DtInteger1));
	classBoxedDataType->addConstant("DtInteger2", Any::fromInt32(DtInteger2));
	classBoxedDataType->addConstant("DtInteger3", Any::fromInt32(DtInteger3));
	classBoxedDataType->addConstant("DtInteger4", Any::fromInt32(DtInteger4));
	registrar->registerClass(classBoxedDataType);

	auto classBoxedDisplayMode = new AutoRuntimeClass< BoxedDisplayMode >();
	classBoxedDisplayMode->addProperty("width", &BoxedDisplayMode::getWidth);
	classBoxedDisplayMode->addProperty("height", &BoxedDisplayMode::getHeight);
	classBoxedDisplayMode->addProperty("refreshRate", &BoxedDisplayMode::getRefreshRate);
	classBoxedDisplayMode->addProperty("colorBits", &BoxedDisplayMode::getColorBits);
	registrar->registerClass(classBoxedDisplayMode);

	auto classBoxedStructElement = new AutoRuntimeClass< BoxedStructElement >();
	classBoxedStructElement->addConstructor< uint32_t, uint32_t >();
	classBoxedStructElement->addProperty("size", &BoxedStructElement::getSize);
	classBoxedStructElement->addProperty("dataType", &BoxedStructElement::getDataType);
	classBoxedStructElement->addProperty("offset", &BoxedStructElement::getOffset);
	registrar->registerClass(classBoxedStructElement);

	auto classBoxedSimpleTextureCreateDesc = new AutoRuntimeClass< BoxedSimpleTextureCreateDesc >();
	classBoxedSimpleTextureCreateDesc->addConstructor();
	classBoxedSimpleTextureCreateDesc->addProperty("width", &BoxedSimpleTextureCreateDesc::setWidth, &BoxedSimpleTextureCreateDesc::getWidth);
	classBoxedSimpleTextureCreateDesc->addProperty("height", &BoxedSimpleTextureCreateDesc::setHeight, &BoxedSimpleTextureCreateDesc::getHeight);
	classBoxedSimpleTextureCreateDesc->addProperty("mipCount", &BoxedSimpleTextureCreateDesc::setMipCount, &BoxedSimpleTextureCreateDesc::getMipCount);
	classBoxedSimpleTextureCreateDesc->addProperty("sRGB", &BoxedSimpleTextureCreateDesc::set_sRGB, &BoxedSimpleTextureCreateDesc::get_sRGB);
	classBoxedSimpleTextureCreateDesc->addProperty("immutable", &BoxedSimpleTextureCreateDesc::setImmutable, &BoxedSimpleTextureCreateDesc::getImmutable);
	registrar->registerClass(classBoxedSimpleTextureCreateDesc);

	auto classBoxedProgramParameters = new AutoRuntimeClass< BoxedProgramParameters >();
	classBoxedProgramParameters->addMethod("setFloatParameter", &BoxedProgramParameters::setFloatParameter);
	classBoxedProgramParameters->addMethod("setVectorParameter", &BoxedProgramParameters::setVectorParameter);
	classBoxedProgramParameters->addMethod("setVectorArrayParameter", &BoxedProgramParameters::setVectorArrayParameter);
	classBoxedProgramParameters->addMethod("setMatrixParameter", &BoxedProgramParameters::setMatrixParameter);
	classBoxedProgramParameters->addMethod("setMatrixArrayParameter", &BoxedProgramParameters::setMatrixArrayParameter);
	classBoxedProgramParameters->addMethod("setTextureParameter", &BoxedProgramParameters::setTextureParameter);
	classBoxedProgramParameters->addMethod("setStructBufferParameter", &BoxedProgramParameters::setStructBufferParameter);
	classBoxedProgramParameters->addMethod("setStencilReference", &BoxedProgramParameters::setStencilReference);
	registrar->registerClass(classBoxedProgramParameters);

	auto classStructBuffer = new AutoRuntimeClass< StructBuffer >();
	classStructBuffer->addProperty("bufferSize", &StructBuffer::getBufferSize);
	classStructBuffer->addMethod("lock", &StructBuffer_lock);
	classStructBuffer->addMethod("unlock", &StructBuffer::unlock);
	registrar->registerClass(classStructBuffer);

	auto classITexture = new AutoRuntimeClass< ITexture >();
	classITexture->addProperty("mips", &ITexture::getMips);
	registrar->registerClass(classITexture);

	auto classICubeTexture = new AutoRuntimeClass< ICubeTexture >();
	classICubeTexture->addProperty("side", &ICubeTexture::getSide);
	registrar->registerClass(classICubeTexture);

	auto classISimpleTexture = new AutoRuntimeClass< ISimpleTexture >();
	classISimpleTexture->addProperty("width", &ISimpleTexture::getWidth);
	classISimpleTexture->addProperty("height", &ISimpleTexture::getHeight);
	classISimpleTexture->addMethod("lock", &ISimpleTexture_lock);
	classISimpleTexture->addMethod("unlock", &ISimpleTexture::unlock);
	registrar->registerClass(classISimpleTexture);

	auto classIVolumeTexture = new AutoRuntimeClass< IVolumeTexture >();
	classIVolumeTexture->addProperty("width", &IVolumeTexture::getWidth);
	classIVolumeTexture->addProperty("height", &IVolumeTexture::getHeight);
	classIVolumeTexture->addProperty("depth", &IVolumeTexture::getDepth);
	registrar->registerClass(classIVolumeTexture);

	auto classIRenderSystem = new AutoRuntimeClass< IRenderSystem >();
	classIRenderSystem->addStaticMethod("getParameterHandle", &IRenderSystem_getHandle);
	classIRenderSystem->addProperty("displayModeCount", &IRenderSystem::getDisplayModeCount);
	classIRenderSystem->addProperty("currentDisplayMode", &IRenderSystem_getCurrentDisplayMode);
	classIRenderSystem->addProperty("displayAspectRatio", &IRenderSystem::getDisplayAspectRatio);
	classIRenderSystem->addMethod("getDisplayMode", &IRenderSystem_getDisplayMode);
	// classIRenderSystem->addMethod("createVertexBuffer", &IRenderSystem_createVertexBuffer);
	// classIRenderSystem->addMethod("createIndexBuffer", &IRenderSystem_createIndexBuffer);
	classIRenderSystem->addMethod("createStructBuffer", &IRenderSystem_createStructBuffer);
	classIRenderSystem->addMethod("createSimpleTexture", &IRenderSystem_createSimpleTexture);
	// classIRenderSystem->addMethod("createCubeTexture", &IRenderSystem_createCubeTexture);
	// classIRenderSystem->addMethod("createVolumeTexture", &IRenderSystem_createVolumeTexture);
	registrar->registerClass(classIRenderSystem);

	auto classIRenderView = new AutoRuntimeClass< IRenderView >();
	classIRenderView->addProperty("width", &IRenderView::getWidth);
	classIRenderView->addProperty("height", &IRenderView::getHeight);
	classIRenderView->addProperty("isActive", &IRenderView::isActive);
	classIRenderView->addProperty("isMinimized", &IRenderView::isMinimized);
	classIRenderView->addProperty("isFullScreen", &IRenderView::isFullScreen);
	classIRenderView->addMethod("close", &IRenderView::close);
	classIRenderView->addMethod("showCursor", &IRenderView::showCursor);
	classIRenderView->addMethod("hideCursor", &IRenderView::hideCursor);
	classIRenderView->addMethod("isCursorVisible", &IRenderView::isCursorVisible);
	classIRenderView->addMethod("setGamma", &IRenderView::setGamma);
	registrar->registerClass(classIRenderView);
}

T_IMPLEMENT_RTTI_CLASS(L"traktor.render.ProgramParameters", BoxedProgramParameters, Object)

BoxedProgramParameters::BoxedProgramParameters(ProgramParameters* programParameters)
:	m_programParameters(programParameters)
{
}

void BoxedProgramParameters::setProgramParameters(ProgramParameters* programParameters)
{
	m_programParameters = programParameters;
}

void BoxedProgramParameters::setFloatParameter(const render::handle_t handle, float param)
{
	m_programParameters->setFloatParameter(handle, param);
}

void BoxedProgramParameters::setVectorParameter(const render::handle_t handle, const Vector4& param)
{
	m_programParameters->setVectorParameter(handle, param);
}

void BoxedProgramParameters::setVectorArrayParameter(const render::handle_t handle, const AlignedVector< Vector4 >& param)
{
	m_programParameters->setVectorArrayParameter(handle, param.c_ptr(), (int)param.size());
}

void BoxedProgramParameters::setMatrixParameter(handle_t handle, const Matrix44& param)
{
	m_programParameters->setMatrixParameter(handle, param);
}

void BoxedProgramParameters::setMatrixArrayParameter(handle_t handle, const AlignedVector< Matrix44 >& param)
{
	m_programParameters->setMatrixArrayParameter(handle, param.c_ptr(), (int)param.size());
}

void BoxedProgramParameters::setTextureParameter(const render::handle_t handle, render::ITexture* texture)
{
	m_programParameters->setTextureParameter(handle, texture);
}

void BoxedProgramParameters::setStructBufferParameter(const render::handle_t handle, render::StructBuffer* structBuffer)
{
	m_programParameters->setStructBufferParameter(handle, structBuffer);
}

void BoxedProgramParameters::setStencilReference(uint32_t stencilReference)
{
	m_programParameters->setStencilReference(stencilReference);
}

void* BoxedProgramParameters::operator new (size_t size)
{
	return s_allocBoxedProgramParameters.alloc();
}

void BoxedProgramParameters::operator delete (void* ptr)
{
	s_allocBoxedProgramParameters.free(ptr);
}

	}
}
