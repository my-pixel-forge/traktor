#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/MemberComposite.h"
#include "Core/Serialization/MemberStaticArray.h"
#include "Core/Serialization/MemberStl.h"
#include "Core/Log/Log.h"
#include "Render/IRenderSystem.h"
#include "Render/ITexture.h"
#include "Render/Shader.h"
#include "Render/VertexBuffer.h"
#include "Render/VertexElement.h"
#include "Resource/IResourceManager.h"
#include "Resource/Member.h"
#include "Spark/Font.h"
#include "Spark/FontResource.h"

namespace traktor
{
	namespace spark
	{
		namespace
		{

#pragma pack(1)
struct GlyphVertex
{
	float pos[2];
};
#pragma pack()

		}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.spark.FontResource", 0, FontResource, ISerializable)

Ref< Font > FontResource::create(resource::IResourceManager* resourceManager, render::IRenderSystem* renderSystem) const
{
	Ref< Font > font = new Font();

	// Bind texture containing glyphs.
	if (!resourceManager->bind(m_texture, font->m_texture))
	{
		log::error << L"Font create failed; unable to bind texture" << Endl;
		return 0;
	}

	// Bind shader for rendering glyphs.
	if (!resourceManager->bind(m_shader, font->m_shader))
	{
		log::error << L"Font create failed; unable to bind shader" << Endl;
		return 0;
	}

	// Create quad geometry for rendering each glyph.
	std::vector< render::VertexElement > vertexElements;
	vertexElements.push_back(render::VertexElement(render::DuPosition, render::DtFloat2, offsetof(GlyphVertex, pos)));

	font->m_vertexBuffer = renderSystem->createVertexBuffer(vertexElements, 6 * sizeof(GlyphVertex), false);
	if (!font->m_vertexBuffer)
		return false;

	GlyphVertex* vertex = reinterpret_cast< GlyphVertex* >(font->m_vertexBuffer->lock());
	T_ASSERT (vertex);

	vertex[2].pos[0] = 0.0f; vertex[2].pos[1] = 1.0f;
	vertex[1].pos[0] = 1.0f; vertex[1].pos[1] = 1.0f;
	vertex[0].pos[0] = 1.0f; vertex[0].pos[1] = 0.0f;

	vertex[5].pos[0] = 0.0f; vertex[5].pos[1] = 1.0f;
	vertex[4].pos[0] = 1.0f; vertex[4].pos[1] = 0.0f;
	vertex[3].pos[0] = 0.0f; vertex[3].pos[1] = 0.0f;

	font->m_vertexBuffer->unlock();

	// Setup glyph to texture mapping.
	for (std::vector< Glyph >::const_iterator i = m_glyphs.begin(); i != m_glyphs.end(); ++i)
	{
		font->m_glyphRects[i->ch] = Vector4(
			i->rect[0] / 1024.0f,
			i->rect[1] / 1024.0f,
			i->rect[2] / 1024.0f,
			i->rect[3] / 1024.0f
		);
	}

	return font;
}

void FontResource::serialize(ISerializer& s)
{
	s >> resource::Member< render::ITexture >(L"texture", m_texture);
	s >> resource::Member< render::Shader >(L"shader", m_shader);
	s >> MemberStlVector< Glyph, MemberComposite< Glyph > >(L"glyphs", m_glyphs);
}

FontResource::Glyph::Glyph()
:	ch(0)
{
	rect[0] =
	rect[1] =
	rect[2] =
	rect[3] = 0;
}

void FontResource::Glyph::serialize(ISerializer& s)
{
	s >> Member< uint32_t >(L"ch", ch);
	s >> MemberStaticArray< uint32_t, 4 >(L"rect", rect);
}

	}
}
