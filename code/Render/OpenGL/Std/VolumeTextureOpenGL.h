#ifndef traktor_render_VolumeTextureOpenGL_H
#define traktor_render_VolumeTextureOpenGL_H

#include "Core/Misc/AutoPtr.h"
#include "Render/IVolumeTexture.h"
#include "Render/Types.h"
#include "Render/OpenGL/ITextureBinding.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_RENDER_OPENGL_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace render
	{
		
class IContext;

/*!
 * \ingroup OGL
 */
class T_DLLCLASS VolumeTextureOpenGL
:	public IVolumeTexture
,	public ITextureBinding
{
	T_RTTI_CLASS;
	
public:
	VolumeTextureOpenGL(IContext* resourceContext);

	virtual ~VolumeTextureOpenGL();

	bool create(const VolumeTextureCreateDesc& desc);

	virtual void destroy();

	virtual ITexture* resolve();

	virtual int getWidth() const;
	
	virtual int getHeight() const;
	
	virtual int getDepth() const;

	virtual void bindSampler(ContextOpenGL* renderContext, GLuint unit, const GLuint sampler[], GLint locationTexture);

	virtual void bindSize(GLint locationSize);

private:
	Ref< IContext > m_resourceContext;
	GLuint m_textureName;
	int m_width;
	int m_height;
	int m_depth;
	int32_t m_pixelSize;
	GLint m_components;
	GLenum m_format;
	GLenum m_type;
};
		
	}
}

#endif	// traktor_render_VolumeTextureOpenGL_H
