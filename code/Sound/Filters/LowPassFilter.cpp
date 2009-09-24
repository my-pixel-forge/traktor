#include <cstring>
#include "Sound/Filters/LowPassFilter.h"

namespace traktor
{
	namespace sound
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.sound.LowPassFilter", LowPassFilter, IFilter)

LowPassFilter::LowPassFilter(float cutOff)
:	m_cutOff(cutOff)
{
	std::memset(m_history, 0, sizeof(m_history));
}

void LowPassFilter::apply(SoundBlock& outBlock)
{
	float dt = 1.0f / outBlock.sampleRate;
	float alpha = dt / (dt + 1.0f / m_cutOff);
	for (uint32_t i = 0; i < outBlock.samplesCount; ++i)
	{
		for (uint32_t j = 0; j < outBlock.maxChannel; ++j)
		{
			outBlock.samples[j][i] = outBlock.samples[j][i] * alpha + m_history[j] * (1.0f - alpha);
			m_history[j] = outBlock.samples[j][i];
		}
	}
}

	}
}
