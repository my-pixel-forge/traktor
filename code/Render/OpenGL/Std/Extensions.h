#ifndef traktor_render_Extensions_H
#define traktor_render_Extensions_H

#include "Render/OpenGL/Platform.h"

namespace traktor
{
	namespace render
	{

#if !defined(__APPLE__)

/*! \ingroup OGL */
//@{

// GL_ARB_shader_objects
// GL_ARB_shading_language_100
// GL_ARB_vertex_shader
// GL_ARB_fragment_shader
extern PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB;
extern PFNGLDELETEOBJECTARBPROC glDeleteObjectARB;
extern PFNGLSHADERSOURCEARBPROC glShaderSourceARB;
extern PFNGLCOMPILESHADERARBPROC glCompileShaderARB;
extern PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB;
extern PFNGLATTACHOBJECTARBPROC glAttachObjectARB;
extern PFNGLLINKPROGRAMARBPROC glLinkProgramARB;
extern PFNGLVALIDATEPROGRAMARBPROC glValidateProgramARB;
extern PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB;
extern PFNGLGETINFOLOGARBPROC glGetInfoLogARB;
extern PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB;
extern PFNGLGETACTIVEUNIFORMARBPROC glGetActiveUniformARB;
extern PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB;
extern PFNGLUNIFORM1IARBPROC glUniform1iARB;
extern PFNGLUNIFORM1FARBPROC glUniform1fARB;
extern PFNGLUNIFORM4FARBPROC glUniform4fARB;
extern PFNGLUNIFORM1FVARBPROC glUniform1fvARB;
extern PFNGLUNIFORM2FVARBPROC glUniform2fvARB;
extern PFNGLUNIFORM4FVARBPROC glUniform4fvARB;
extern PFNGLUNIFORMMATRIX4FVARBPROC glUniformMatrix4fvARB;
extern PFNGLGETACTIVEATTRIBARBPROC glGetActiveAttribARB;
extern PFNGLGETATTRIBLOCATIONARBPROC glGetAttribLocationARB;
extern PFNGLBINDATTRIBLOCATIONARBPROC glBindAttribLocationARB;
extern PFNGLENABLEVERTEXATTRIBARRAYARBPROC glEnableVertexAttribArrayARB;
extern PFNGLVERTEXATTRIBPOINTERARBPROC glVertexAttribPointerARB;

// GL_ARB_vertex_buffer_object
extern PFNGLBINDBUFFERARBPROC glBindBufferARB;
extern PFNGLGENBUFFERSARBPROC glGenBuffersARB;
extern PFNGLBUFFERDATAARBPROC glBufferDataARB;
extern PFNGLDELETEBUFFERSARBPROC glDeleteBuffersARB;
extern PFNGLMAPBUFFERARBPROC glMapBufferARB;
extern PFNGLUNMAPBUFFERARBPROC glUnmapBufferARB;

// GL_EXT_???
extern PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT;
extern PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT;
extern PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT;
extern PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT;
extern PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT;
extern PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT;
extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glRenderbufferStorageMultisampleEXT;
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT;
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT;
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameterivEXT;
extern PFNGLBLITFRAMEBUFFEREXTPROC glBlitFramebufferEXT;

// GL_???
extern PFNGLACTIVETEXTUREPROC glActiveTexture;

// GL_EXT_???
extern PFNGLBLENDEQUATIONEXTPROC glBlendEquationEXT;

// GL_ARB_texture_compression
extern PFNGLCOMPRESSEDTEXIMAGE2DPROC glCompressedTexImage2D;

// ???
extern PFNGLDRAWRANGEELEMENTSPROC glDrawRangeElements;

// GL_ARB_half_float_vertex
#	if !defined(GL_HALF_FLOAT_ARB)
#		define GL_HALF_FLOAT_ARB 0x140B
#	endif
#endif

enum Extentions
{
	E_GL_ARB_vertex_buffer_object = 0,
	E_GL_ARB_texture_non_power_of_two = 1,
	E_GL_ARB_texture_float = 2,
	E_GL_NV_float_buffer = 3,
	E_GL_ATI_texture_float = 4,
	E_GL_EXT_framebuffer_blit = 5,
	E_GL_EXT_framebuffer_object = 6,
	E_GL_EXT_framebuffer_multisample = 7,
	E_GL_ARB_half_float_vertex = 8
};

bool opengl_initialize_extensions();

bool opengl_have_extension(Extentions extension);

//@}

	}
}

#endif	// traktor_render_Extensions_H
