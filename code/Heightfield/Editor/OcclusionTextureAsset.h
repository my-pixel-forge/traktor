#ifndef traktor_hf_OcclusionTextureAsset_H
#define traktor_hf_OcclusionTextureAsset_H

#include "Core/Serialization/ISerializable.h"
#include "Resource/Id.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_HEIGHTFIELD_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace hf
	{

class Heightfield;

class T_DLLCLASS OcclusionTextureAsset : public ISerializable
{
	T_RTTI_CLASS;

public:
	OcclusionTextureAsset();

	virtual void serialize(ISerializer& s);

private:
	friend class OcclusionTexturePipeline;

	resource::Id< Heightfield > m_heightfield;
	resource::Id< ISerializable > m_occluderData;
	uint32_t m_size;
	float m_traceDistance;
	int32_t m_blurRadius;
};

	}
}

#endif	// traktor_hf_OcclusionTextureAsset_H
