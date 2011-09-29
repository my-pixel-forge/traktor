#ifndef traktor_terrain_HeightfieldAsset_H
#define traktor_terrain_HeightfieldAsset_H

#include "Editor/Asset.h"
#include "Core/Math/Vector4.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_TERRAIN_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace terrain
	{

class T_DLLCLASS HeightfieldAsset : public editor::Asset
{
	T_RTTI_CLASS;

public:
	HeightfieldAsset();

	virtual const TypeInfo* getOutputType() const;

	virtual bool serialize(ISerializer& s);

	const Vector4& getWorldExtent() const { return m_worldExtent; }

	uint32_t getPatchDim() const { return m_patchDim; }

	uint32_t getDetailSkip() const { return m_detailSkip; }

private:
	Vector4 m_worldExtent;
	uint32_t m_patchDim;
	uint32_t m_detailSkip;
};

	}
}

#endif	// traktor_terrain_HeightfieldAsset_H
