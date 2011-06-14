#include <map>
#include "Core/Io/StringOutputStream.h"
#include "Core/Singleton/ISingleton.h"
#include "Core/Singleton/SingletonManager.h"
#include "Render/Types.h"

namespace traktor
{
	namespace render
	{
		namespace
		{

class HandleRegistry
{
public:
	HandleRegistry()
	:	m_nextUnusedHandle(1)
	{
	}

	handle_t getHandle(const std::wstring& name)
	{
		std::map< std::wstring, handle_t >::const_iterator i = m_handles.find(name);
		if (i != m_handles.end())
			return i->second;

		handle_t handle = m_nextUnusedHandle++;
		m_handles.insert(std::make_pair(name, handle));

		return handle;
	}

private:
	std::map< std::wstring, handle_t > m_handles;
	handle_t m_nextUnusedHandle;
};

HandleRegistry s_handleRegistry;

		}

handle_t getParameterHandle(const std::wstring& name)
{
	return s_handleRegistry.getHandle(name);
}

std::wstring getParameterNameFromGuid(const Guid& guid)
{
	const wchar_t hex[] = { L"0123456789ABCDEF" };

	StringOutputStream ss;
	ss << L"pmg_";

	for (int i = 0; i < 16; ++i)
	{
		ss.put(hex[guid[i] >> 4]);
		ss.put(hex[guid[i] & 0x0f]);
	}

	return ss.str();
}

std::wstring getDataUsageName(DataUsage usage)
{
	const wchar_t* c_names[] =
	{
		L"DuPosition",
		L"DuNormal",
		L"DuTangent",
		L"DuBinormal",
		L"DuColor",
		L"DuCustom"
	};
	T_ASSERT (int(usage) < sizeof_array(c_names));
	return c_names[int(usage)];
}

std::wstring getDataTypeName(DataType dataType)
{
	const wchar_t* c_names[] =
	{
		L"DtFloat1",
		L"DtFloat2",
		L"DtFloat3",
		L"DtFloat4",
		L"DtByte4",
		L"DtByte4N",
		L"DtShort2",
		L"DtShort4",
		L"DtShort2N",
		L"DtShort4N",
		L"DtHalf2",
		L"DtHalf4"
	};
	T_ASSERT (int(dataType) < sizeof_array(c_names));
	return c_names[int(dataType)];
}

uint32_t getDataElementCount(DataType dataType)
{
	const uint32_t c_elementCounts[] = { 1, 2, 3, 4, 4, 4, 2, 4, 2, 4, 2, 4 };
	T_ASSERT (int(dataType) < sizeof_array(c_elementCounts));
	return c_elementCounts[int(dataType)];
}

std::wstring getTextureFormatName(TextureFormat format)
{
	const wchar_t* c_names[] =
	{
		L"TfInvalid",
		L"TfR8",
		L"TfR8G8B8A8",
		L"TfR5G6B5",
		L"TfR5G5B5A1",
		L"TfR4G4B4A4",
		L"TfR16G16B16A16F",
		L"TfR32G32B32A32F",
		L"TfR16G16F",
		L"TfR32G32F",
		L"TfR16F",
		L"TfR32F",
		L"TfDXT1",
		L"TfDXT2",
		L"TfDXT3",
		L"TfDXT4",
		L"TfDXT5",
		L"TfPVRTC1",
		L"TfPVRTC2",
		L"TfPVRTC3",
		L"TfPVRTC4"
	};
	return int(format) < sizeof_array(c_names) ? c_names[int(format)] : c_names[0];
}

uint32_t getTextureBlockSize(TextureFormat format)
{
	const uint32_t c_blockSizes[] = { 0, 1, 4, 2, 2, 2, 8, 16, 4, 8, 2, 4, 8, 16, 16, 16, 16, 1, 1, 1, 1 };
	T_ASSERT (int(format) < sizeof_array(c_blockSizes));
	return c_blockSizes[int(format)];
}

uint32_t getTextureBlockDenom(TextureFormat format)
{
	const uint32_t c_blockDenoms[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 2, 4, 2, 4 };
	T_ASSERT (int(format) < sizeof_array(c_blockDenoms));
	return c_blockDenoms[int(format)];
}

uint32_t getTextureMipSize(uint32_t textureSize, uint32_t mipLevel)
{
	uint32_t mipSize = textureSize >> mipLevel;
	return mipSize > 0 ? mipSize : 1;
}

uint32_t getTextureRowPitch(TextureFormat format, uint32_t textureWidth)
{
	uint32_t blockDenom = getTextureBlockDenom(format);
	uint32_t blockWidth = (textureWidth + blockDenom - 1) / blockDenom;
	return getTextureBlockSize(format) * blockWidth;
}

uint32_t getTextureRowPitch(TextureFormat format, uint32_t textureWidth, uint32_t mipLevel)
{
	return getTextureRowPitch(format, getTextureMipSize(textureWidth, mipLevel));
}

uint32_t getTextureMipPitch(TextureFormat format, uint32_t textureWidth, uint32_t textureHeight)
{
	uint32_t blockDenom = getTextureBlockDenom(format);
	uint32_t blockWidth = (textureWidth + blockDenom - 1) / blockDenom;
	uint32_t blockHeight = (textureHeight + blockDenom - 1) / blockDenom;
	uint32_t blockCount = blockWidth * blockHeight;
	return getTextureBlockSize(format) * blockCount;
}

uint32_t getTextureMipPitch(TextureFormat format, uint32_t textureWidth, uint32_t textureHeight, uint32_t mipLevel)
{
	return getTextureMipPitch(format, getTextureMipSize(textureWidth, mipLevel), getTextureMipSize(textureHeight, mipLevel));
}

uint32_t getTextureSize(TextureFormat format, uint32_t textureWidth, uint32_t textureHeight, uint32_t mipLevels)
{
	uint32_t textureSize = 0;
	for (uint32_t mipLevel = 0; mipLevel < mipLevels; ++mipLevel)
		textureSize += getTextureMipPitch(format, textureWidth, textureHeight, mipLevel);
	return textureSize;
}

	}
}
