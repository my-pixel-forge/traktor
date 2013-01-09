#ifndef traktor_render_GlslProgram_H
#define traktor_render_GlslProgram_H

#include <string>
#include "Render/OpenGL/TypesOpenGL.h"

namespace traktor
{
	namespace render
	{

/*!
 * \ingroup OGL
 */
class GlslProgram
{
public:
	GlslProgram();

	GlslProgram(
		const std::wstring& vertexShader,
		const std::wstring& fragmentShader,
		const std::vector< std::wstring >& textures,
		const std::vector< SamplerBinding >& samplers,
		const RenderState& renderState
	);

	const std::wstring& getVertexShader() const;

	const std::wstring& getFragmentShader() const;

	const std::vector< std::wstring >& getTextures() const;

	const std::vector< SamplerBinding >& getSamplers() const;

	const RenderState& getRenderState() const;

private:
	std::wstring m_vertexShader;
	std::wstring m_fragmentShader;
	std::vector< std::wstring > m_textures;
	std::vector< SamplerBinding > m_samplers;
	RenderState m_renderState;
};

	}
}

#endif	// traktor_render_GlslProgram_H
