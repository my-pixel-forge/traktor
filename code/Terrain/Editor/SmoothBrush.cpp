#include "Core/Math/Const.h"
#include "Core/Math/Float.h"
#include "Heightfield/Heightfield.h"
#include "Terrain/Editor/IFallOff.h"
#include "Terrain/Editor/SmoothBrush.h"

namespace traktor
{
	namespace terrain
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.terrain.SmoothBrush", SmoothBrush, IBrush)

SmoothBrush::SmoothBrush(const resource::Proxy< hf::Heightfield >& heightfield)
:	m_heightfield(heightfield)
,	m_radius(0)
,	m_fallOff(0)
,	m_strength(0.0f)
{
}

uint32_t SmoothBrush::begin(int32_t x, int32_t y, const State& state)
{
	m_radius = state.radius;
	m_fallOff = state.falloff;
	m_strength = powf(abs(state.strength), 3.0f);
	return MdHeight;
}

void SmoothBrush::apply(int32_t x, int32_t y)
{
	for (int32_t iy = -m_radius; iy <= m_radius; ++iy)
	{
		for (int32_t ix = -m_radius; ix <= m_radius; ++ix)
		{
			float fx = float(ix) / m_radius;
			float fy = float(iy) / m_radius;

			float a = m_fallOff->evaluate(fx, fy) * m_strength;
			if (abs(a) <= FUZZY_EPSILON)
				continue;

			float height = 0.0f;
			for (int32_t iiy = -1; iiy <= 1; ++iiy)
			{
				for (int32_t iix = -1; iix <= 1; ++iix)
				{
					height += m_heightfield->getGridHeightNearest(x + ix + iix, y + iy + iiy);
				}
			}
			height /= 9.0f;

			float h = m_heightfield->getGridHeightNearest(x + ix, y + iy);
			m_heightfield->setGridHeight(x + ix, y + iy, lerp(h, height, a));
		}
	}
}

void SmoothBrush::end(int32_t x, int32_t y)
{
}

	}
}
