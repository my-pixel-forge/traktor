/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_render_ProgramOpenGLES2_H
#define traktor_render_ProgramOpenGLES2_H

#include "Core/RefArray.h"
#include "Core/Containers/SmallMap.h"
#include "Render/IProgram.h"
#include "Render/OpenGL/ES2/TypesOpenGLES2.h"

namespace traktor
{
	namespace render
	{

class ContextOpenGLES2;
class ITexture;
class ProgramResource;
class StateCache;

/*!
 * \ingroup OGL
 */
class ProgramOpenGLES2 : public IProgram
{
	T_RTTI_CLASS;

public:
	virtual ~ProgramOpenGLES2();

	static Ref< ProgramOpenGLES2 > create(ContextOpenGLES2* resourceContext, const ProgramResource* resource);

	virtual void destroy() override final;

	virtual void setFloatParameter(handle_t handle, float param) override final;

	virtual void setFloatArrayParameter(handle_t handle, const float* param, int length) override final;
	
	virtual void setVectorParameter(handle_t handle, const Vector4& param) override final;

	virtual void setVectorArrayParameter(handle_t handle, const Vector4* param, int length) override final;

	virtual void setMatrixParameter(handle_t handle, const Matrix44& param) override final;

	virtual void setMatrixArrayParameter(handle_t handle, const Matrix44* param, int length) override final;

	virtual void setTextureParameter(handle_t handle, ITexture* texture) override final;

	virtual void setStencilReference(uint32_t stencilReference) override final;

	bool activate(StateCache* stateCache, float targetSize[2], float postTransform[4], bool invertCull, uint32_t instanceID);

private:
	struct Uniform
	{
		GLint location;
		GLenum type;
		uint32_t offset;
		uint32_t length;
		bool dirty;
	};

	struct Sampler
	{
		GLint locationTexture;
		uint32_t texture;
		uint32_t unit;
	};

	struct TextureSize
	{
		GLint location;
		uint32_t texture;

		TextureSize()
		:	location(0)
		,	texture(0)
		{
		}
	};

	struct TextureData
	{
		GLenum target;
		GLuint name;
		GLint mipCount;
		float offset[4];
	};

	static ProgramOpenGLES2* ms_current;
	Ref< ContextOpenGLES2 > m_resourceContext;
	GLuint m_program;
	RenderStateOpenGL m_renderState;
	GLint m_locationTargetSize;
	GLint m_locationPostTransform;
	GLint m_locationInstanceID;
	SmallMap< handle_t, uint32_t > m_parameterMap;			//!< Parameter to data map.
	AlignedVector< Uniform > m_uniforms;					//!< Scalar uniforms.
	AlignedVector< Sampler > m_samplers;					//!< Samplers.
	AlignedVector< TextureSize > m_textureSize;
	AlignedVector< float > m_uniformData;					//!< Scalar uniform data.
	RefArray< ITexture > m_textures;
	float m_targetSize[4];
	
	ProgramOpenGLES2(ContextOpenGLES2* resourceContext, GLuint program, const ProgramResource* resource);
};

	}
}

#endif	// traktor_render_ProgramOpenGLES2_H
