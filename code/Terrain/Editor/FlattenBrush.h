#pragma once

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

class FlattenBrush : public IBrush
{
	T_RTTI_CLASS;

public:
	FlattenBrush(const resource::Proxy< hf::Heightfield >& heightfield);

	virtual uint32_t begin(float x, float y, const State& state) override final;

	virtual void apply(float x, float y) override final;

	virtual void end(float x, float y) override final;

private:
	resource::Proxy< hf::Heightfield > m_heightfield;
	int32_t m_radius;
	const IFallOff* m_fallOff;
	float m_strength;
	float m_height;
};

	}
}

