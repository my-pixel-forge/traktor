#ifndef traktor_terrain_SmoothBrush_H
#define traktor_terrain_SmoothBrush_H

#include "Resource/Proxy.h"
#include "Terrain/Editor/IBrush.h"

namespace traktor
{
	namespace hf
	{

class Heightfield;

	}

	namespace terrain
	{

class SmoothBrush : public IBrush
{
	T_RTTI_CLASS;

public:
	SmoothBrush(const resource::Proxy< hf::Heightfield >& heightfield, int32_t radius);

	virtual void begin(int32_t x, int32_t y);

	virtual void apply(int32_t x, int32_t y);

	virtual void end(int32_t x, int32_t y);

private:
	resource::Proxy< hf::Heightfield > m_heightfield;
	int32_t m_radius;
};

	}
}

#endif	// traktor_terrain_SmoothBrush_H
