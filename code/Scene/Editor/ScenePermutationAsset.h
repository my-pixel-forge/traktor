#ifndef traktor_scene_ScenePermutationAsset_H
#define traktor_scene_ScenePermutationAsset_H

#include <list>
#include "Core/Containers/SmallMap.h"
#include "Core/Serialization/ISerializable.h"
#include "Resource/Id.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_SCENE_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace render
	{

class ImageProcessSettings;

	}

	namespace world
	{

class WorldRenderSettings;

	}

	namespace scene
	{

class T_DLLCLASS ScenePermutationAsset : public ISerializable
{
	T_RTTI_CLASS;

public:
	virtual void serialize(ISerializer& s);

private:
	friend class ScenePermutationPipeline;

	Guid m_scene;
	std::list< std::wstring > m_includeLayers;
	Ref< world::WorldRenderSettings > m_overrideWorldRenderSettings;
	resource::Id< render::ImageProcessSettings > m_overrideImageProcessSettings[world::QuLast];
	SmallMap< std::wstring, resource::Id< render::ITexture > > m_overrideImageProcessParams;
};

	}
}

#endif	// traktor_scene_ScenePermutationAsset_H
