#ifndef traktor_weather_CloudMaskResource_H
#define traktor_weather_CloudMaskResource_H

#include "Core/Serialization/Serializable.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_WEATHER_EXPORT)
#define T_DLLCLASS T_DLLEXPORT
#else
#define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace weather
	{

class T_DLLCLASS CloudMaskResource : public Serializable
{
	T_RTTI_CLASS(CloudMaskResource)

public:
	CloudMaskResource(int32_t size = 0);

	int32_t getSize() const;

	virtual bool serialize(Serializer& s);

private:
	int32_t m_size;
};

	}
}

#endif	// traktor_weather_CloudMaskResource_H
