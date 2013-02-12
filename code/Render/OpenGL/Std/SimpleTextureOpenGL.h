#ifndef traktor_render_SimpleTextureOpenGL_H
#define traktor_render_SimpleTextureOpenGL_H

#include "Core/Math/Vector4.h"
#include "Core/Misc/AutoPtr.h"
#include "Render/ISimpleTexture.h"
#include "Render/Types.h"
#include "Render/OpenGL/ITextureBinding.h"

namespace traktor
{
	namespace render
	{

class IContext;

/*!
 * \ingroup OGL
 */
class SimpleTextureOpenGL
:	public ISimpleTexture
,	public ITextureBinding
{
	T_RTTI_CLASS;

public:
	SimpleTextureOpenGL(IContext* resourceContext);

	virtual ~SimpleTextureOpenGL();
	
	bool create(const SimpleTextureCreateDesc& desc, GLfloat maxAnisotropy);

	virtual void destroy();

	virtual ITexture* resolve();

	virtual int getWidth() const;
	
	virtual int getHeight() const;

	virtual bool lock(int level, Lock& lock);

	virtual void unlock(int level);

	virtual void bindTexture(ContextOpenGL* renderContext, uint32_t samplerObject);

	virtual void bindSize(GLint locationSize);

private:
	Ref< IContext > m_resourceContext;
	GLuint m_textureName;
	int m_width;
	int m_height;
	int m_pixelSize;
	GLint m_components;
	GLenum m_format;
	GLenum m_type;
	uint32_t m_mipCount;
	GLfloat m_maxAnisotropy;
	uint32_t m_dataSize;
	AutoArrayPtr< uint8_t > m_data;
};
		
	}
}

#endif	// traktor_render_SimpleTextureOpenGL_H
