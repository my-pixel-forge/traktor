#ifndef traktor_weather_CloudMaskAsset_H
#define traktor_weather_CloudMaskAsset_H

#include "Editor/Asset.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_WEATHER_EDITOR_EXPORT)
#define T_DLLCLASS T_DLLEXPORT
#else
#define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace weather
	{

class T_DLLCLASS CloudMaskAsset : public editor::Asset
{
	T_RTTI_CLASS(CloudMaskAsset)

public:
	virtual const Type* getOutputType() const;
};

	}
}

#endif	// traktor_weather_CloudMaskAsset_H
