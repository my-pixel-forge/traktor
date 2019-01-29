#include <cstring>
#include "Render/OpenGL/Std/Platform.h"
#include "Render/OpenGL/Std/CubeTextureOpenGL.h"
#include "Render/OpenGL/Std/RenderContextOpenGL.h"
#include "Render/OpenGL/Std/ResourceContextOpenGL.h"
#include "Render/OpenGL/Std/UtilitiesOpenGL.h"

namespace traktor
{
	namespace render
	{
		namespace
		{

#if !defined(__APPLE__)
const GLenum c_cubeFaces[] =
{
	GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT
};
#else
const GLenum c_cubeFaces[] =
{
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};
#endif

struct DeleteTextureCallback : public ResourceContextOpenGL::IDeleteCallback
{
	GLuint m_textureName;

	DeleteTextureCallback(GLuint textureName)
	:	m_textureName(textureName)
	{
	}

	virtual void deleteResource()
	{
		T_OGL_SAFE(glDeleteTextures(1, &m_textureName));
		delete this;
	}
};

		}

T_IMPLEMENT_RTTI_CLASS(L"traktor.render.CubeTextureOpenGL", CubeTextureOpenGL, ICubeTexture)

CubeTextureOpenGL::CubeTextureOpenGL(ResourceContextOpenGL* resourceContext)
:	m_resourceContext(resourceContext)
,	m_textureName(0)
,	m_side(0)
,	m_pixelSize(0)
,	m_mipCount(0)
,	m_components(0)
,	m_format(0)
,	m_type(0)
{
}

CubeTextureOpenGL::~CubeTextureOpenGL()
{
	destroy();
}

bool CubeTextureOpenGL::create(const CubeTextureCreateDesc& desc)
{
	m_side = desc.side;

	if (desc.sRGB)
	{
		if (!convertTextureFormat_sRGB(desc.format, m_pixelSize, m_components, m_format, m_type))
			return false;
	}
	else
	{
		if (!convertTextureFormat(desc.format, m_pixelSize, m_components, m_format, m_type))
			return false;
	}

	T_OGL_SAFE(glGenTextures(1, &m_textureName));
	T_OGL_SAFE(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
	T_OGL_SAFE(glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureName));

	T_OGL_SAFE(glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	T_OGL_SAFE(glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST));

	m_data.reset(new uint8_t [m_side * m_side * m_pixelSize]);

	if (desc.immutable)
	{
		for (int face = 0; face < 6; ++face)
		{
			for (int i = 0; i < desc.mipCount; ++i)
			{
				uint32_t side = getTextureMipSize(m_side, i);
				if (desc.format >= TfDXT1 && desc.format <= TfDXT5)
				{
					uint32_t mipPitch = getTextureMipPitch(desc.format, side, side);
					T_OGL_SAFE(glCompressedTexImage2D(
						c_cubeFaces[face],
						i,
						m_components,
						side,
						side,
						0,
						mipPitch,
						desc.initialData[face * desc.mipCount + i].data
					));
				}
				else
				{
					T_OGL_SAFE(glTexImage2D(
						c_cubeFaces[face],
						i,
						m_components,
						side,
						side,
						0,
						m_format,
						m_type,
						desc.initialData[face * desc.mipCount + i].data
					));
				}
			}
		}
	}

	m_mipCount = desc.mipCount;
	return true;
}

void CubeTextureOpenGL::destroy()
{
	if (m_textureName)
	{
		m_resourceContext->deleteResource(new DeleteTextureCallback(m_textureName));
		m_textureName = 0;
	}
}

ITexture* CubeTextureOpenGL::resolve()
{
	return this;
}

int CubeTextureOpenGL::getWidth() const
{
	return m_side;
}

int CubeTextureOpenGL::getHeight() const
{
	return m_side;
}

int CubeTextureOpenGL::getDepth() const
{
	return 1;
}

bool CubeTextureOpenGL::lock(int side, int level, Lock& lock)
{
	lock.pitch = (m_side >> level) * m_pixelSize;
	lock.bits = m_data.ptr();
	return true;
}

void CubeTextureOpenGL::unlock(int side, int level)
{
	T_ANONYMOUS_VAR(ContextOpenGL::Scope)(m_resourceContext);
	T_OGL_SAFE(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
	T_OGL_SAFE(glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureName));
	T_OGL_SAFE(glTexImage2D(
		c_cubeFaces[side],
		level,
		m_components,
		m_side >> level,
		m_side >> level,
		0,
		m_format,
		m_type,
		m_data.c_ptr()
	));
}

void CubeTextureOpenGL::bindTexture(GLuint textureUnit) const
{
	T_OGL_SAFE(glActiveTexture(GL_TEXTURE0 + textureUnit));
	T_OGL_SAFE(glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureName));
}

void CubeTextureOpenGL::bindImage(GLuint imageUnit) const
{
}

void CubeTextureOpenGL::bindSize(GLint locationSize) const
{
	T_OGL_SAFE(glUniform4f(locationSize, GLfloat(m_side), GLfloat(m_side), GLfloat(m_side), GLfloat(1.0f)));
}

bool CubeTextureOpenGL::haveMips() const
{
	return m_mipCount > 1;
}

	}
}
