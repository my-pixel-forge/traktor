#pragma once

#include <string>
#include "Core/Containers/AlignedVector.h"
#include "Render/Vulkan/Editor/Glsl/GlslResource.h"
#include "Render/Vulkan/Editor/Glsl/GlslType.h"

namespace traktor
{
	namespace render
	{
	
class GlslUniformBuffer : public GlslResource
{
	T_RTTI_CLASS;

public:
	struct Uniform
	{
		std::wstring name;
		GlslType type;
		int32_t length;
	};

	GlslUniformBuffer(const std::wstring& name);

	const std::wstring& getName() const { return m_name; }

	void add(const std::wstring& uniformName, GlslType uniformType, int32_t length);

	const AlignedVector< Uniform >& get() const { return m_uniforms; }

private:
	std::wstring m_name;
	AlignedVector< Uniform > m_uniforms;
};

	}
}