#include <cmath>
#include <cstring>
#include "Core/Log/Log.h"
#include "Core/Math/Const.h"
#include "Core/Math/MathUtils.h"
#include "Core/Math/Vector4.h"
#include "Core/Memory/Alloc.h"
#include "Core/Misc/Align.h"
#include "Core/Thread/Acquire.h"
#include "Sound/IFilter.h"
#include "Sound/ISoundBuffer.h"
#include "Sound/ISoundMixer.h"
#include "Sound/SoundChannel.h"

namespace traktor
{
	namespace sound
	{
		namespace
		{

const uint32_t c_outputSamplesBlockCount = 8;

inline void moveSamples(float* destSamples, const float* sourceSamples, int32_t samplesCount)
{
	T_ASSERT ((samplesCount & 3) == 0);
	Vector4 tmp[4];
	int32_t i;

	for (i = 0; i < samplesCount - 16; i += 16)
	{
		tmp[0] = Vector4::loadAligned(&sourceSamples[i + 0]);
		tmp[1] = Vector4::loadAligned(&sourceSamples[i + 4]);
		tmp[2] = Vector4::loadAligned(&sourceSamples[i + 8]);
		tmp[3] = Vector4::loadAligned(&sourceSamples[i + 12]);

		tmp[0].storeAligned(&destSamples[i + 0]);
		tmp[1].storeAligned(&destSamples[i + 4]);
		tmp[2].storeAligned(&destSamples[i + 8]);
		tmp[3].storeAligned(&destSamples[i + 12]);
	}

	for (; i < samplesCount; i += 4)
		Vector4::loadAligned(&sourceSamples[i]).storeAligned(&destSamples[i]);
}

		}

T_IMPLEMENT_RTTI_CLASS(L"traktor.sound.SoundChannel", SoundChannel, Object)

SoundChannel::~SoundChannel()
{
	Alloc::freeAlign(m_outputSamples[0]);
}

void SoundChannel::setVolume(float volume)
{
	m_volume = clamp(volume, 0.0f, 1.0f);
}

void SoundChannel::setFilter(const IFilter* filter)
{
	if (m_currentState.filter != filter)
	{
		T_ANONYMOUS_VAR(Acquire< Semaphore >)(m_lock);
		if ((m_currentState.filter = filter) != 0)
			m_currentState.filterInstance = filter->createInstance();
		else
			m_currentState.filterInstance = 0;
	}
}

const IFilter* SoundChannel::getFilter() const
{
	return m_currentState.filter;
}

void SoundChannel::setPitch(float pitch)
{
	m_pitch = pitch;
}

float SoundChannel::getPitch() const
{
	return m_pitch;
}

bool SoundChannel::isPlaying() const
{
	// \note Reading from active state; ie other thread's state.
	return bool(m_currentState.buffer != 0 || m_activeState.buffer != 0);
}

void SoundChannel::stop()
{
	T_ANONYMOUS_VAR(Acquire< Semaphore >)(m_lock);
	m_currentState.buffer = 0;
	m_currentState.cursor = 0;
	m_currentState.filter = 0;
	m_currentState.filterInstance = 0;
	m_eventFinish.broadcast();
}

ISoundBufferCursor* SoundChannel::getCursor() const
{
	return m_currentState.cursor;
}

void SoundChannel::setParameter(float parameter)
{
	T_ANONYMOUS_VAR(Acquire< Semaphore >)(m_lock);
	if (m_currentState.cursor)
		m_currentState.cursor->setParameter(parameter);
}

bool SoundChannel::play(const ISoundBuffer* buffer, float volume, float presence, float presenceRate, uint32_t repeat)
{
	if (!buffer)
		return false;

	Ref< ISoundBufferCursor > cursor = buffer->createCursor();
	if (!cursor)
		return false;

	// Queue state; activated next time channel is polled for another
	// sound block.
	{
		T_ANONYMOUS_VAR(Acquire< Semaphore >)(m_lock);
		m_currentState.buffer = buffer;
		m_currentState.cursor = cursor;
		m_currentState.volume = volume;
		m_currentState.presence = presence;
		m_currentState.presenceRate = presenceRate;
		m_currentState.repeat = max< uint32_t >(repeat, 1);
	}

	return true;
}

SoundChannel::SoundChannel(uint32_t id, Event& eventFinish, uint32_t hwSampleRate, uint32_t hwFrameSamples)
:	m_id(id)
,	m_eventFinish(eventFinish)
,	m_hwSampleRate(hwSampleRate)
,	m_hwFrameSamples(hwFrameSamples)
,	m_outputSamplesIn(0)
,	m_pitch(1.0f)
,	m_volume(1.0f)
{
	const uint32_t outputSamplesCount = hwFrameSamples * c_outputSamplesBlockCount;
	const uint32_t outputSamplesSize = SbcMaxChannelCount * outputSamplesCount * sizeof(float);

	m_outputSamples[0] = static_cast< float* >(Alloc::acquireAlign(outputSamplesSize, 16, T_FILE_LINE));
	std::memset(m_outputSamples[0], 0, outputSamplesSize);

	for (uint32_t i = 1; i < SbcMaxChannelCount; ++i)
		m_outputSamples[i] = m_outputSamples[0] + outputSamplesCount * i;
}

bool SoundChannel::getBlock(const ISoundMixer* mixer, double time, SoundBlock& outBlock)
{
	T_ANONYMOUS_VAR(Acquire< Semaphore >)(m_lock);

	// Update active state.
	if (m_currentState.buffer != m_activeState.buffer)
		m_outputSamplesIn = 0;
	if (m_currentState.buffer != m_activeState.buffer || m_currentState.filter != m_activeState.filter)
		m_activeState = m_currentState;

	if (!m_activeState.buffer || !m_activeState.cursor)
		return false;

	const ISoundBuffer* soundBuffer = m_activeState.buffer;
	T_ASSERT (soundBuffer);

	// Remove old output samples.
	if (m_outputSamplesIn >= m_hwFrameSamples)
	{
		m_outputSamplesIn -= m_hwFrameSamples;
		if (m_outputSamplesIn > 0)
		{
			for (uint32_t i = 0; i < SbcMaxChannelCount; ++i)
				moveSamples(
					m_outputSamples[i],
					m_outputSamples[i] + m_hwFrameSamples,
					alignUp(m_outputSamplesIn, 4)
				);
		}
	}

	while (m_outputSamplesIn < m_hwFrameSamples)
	{
		// Request sound block from buffer.
		SoundBlock soundBlock = { { 0, 0, 0, 0, 0, 0, 0, 0 }, m_hwFrameSamples, 0, 0 };
		if (!soundBuffer->getBlock(m_activeState.cursor, mixer, soundBlock))
		{
			// No more blocks from sound buffer.
			if (--m_activeState.repeat > 0)
			{
				m_activeState.cursor->reset();
				if (!soundBuffer->getBlock(m_activeState.cursor, mixer, soundBlock))
				{
					m_activeState.buffer = 0;
					m_activeState.cursor = 0;
					m_currentState.buffer = 0;
					m_currentState.cursor = 0;
					m_eventFinish.broadcast();
					return false;
				}
			}
			else
			{
				m_activeState.buffer = 0;
				m_activeState.cursor = 0;
				m_currentState.buffer = 0;
				m_currentState.cursor = 0;
				m_eventFinish.broadcast();
				return false;
			}
		}

		// We might get a null block; does not indicate end of stream.
		if (!soundBlock.samplesCount || !soundBlock.sampleRate || !soundBlock.maxChannel)
			return false;

		T_ASSERT (soundBlock.samplesCount <= m_hwFrameSamples);

		// Apply filter on sound block.
		if (m_activeState.filter)
		{
			m_activeState.filter->apply(m_activeState.filterInstance, soundBlock);
			T_ASSERT (soundBlock.samplesCount <= m_hwFrameSamples);
		}

		uint32_t sampleRate = uint32_t(m_pitch * soundBlock.sampleRate);

		// Convert sound block into hardware required sample rate.
		if (sampleRate != m_hwSampleRate)
		{
			uint32_t outputSamplesCount = (soundBlock.samplesCount * m_hwSampleRate) / sampleRate;

			// Ensure we produce "multiple-of-4" number of samples; slight adjust block's sample rate.
			outputSamplesCount = alignUp(outputSamplesCount, 4);
			soundBlock.sampleRate = (soundBlock.samplesCount * m_hwSampleRate) / outputSamplesCount;

			for (uint32_t i = 0; i < SbcMaxChannelCount; ++i)
			{
				const float* inputSamples = soundBlock.samples[i];
				
				float* outputSamples = m_outputSamples[i] + m_outputSamplesIn;
				T_ASSERT (alignUp(outputSamples, 16) == outputSamples);
				T_ASSERT (m_outputSamplesIn + outputSamplesCount < m_hwFrameSamples * c_outputSamplesBlockCount);

				if (inputSamples)
					mixer->stretch(
						outputSamples,
						outputSamplesCount,
						inputSamples,
						soundBlock.samplesCount,
						m_volume * m_activeState.volume
					);
				else
					mixer->mute(outputSamples, outputSamplesCount);
			}

			m_outputSamplesIn += outputSamplesCount;
		}
		else
		{
			for (uint32_t i = 0; i < SbcMaxChannelCount; ++i)
			{
				const float* inputSamples = soundBlock.samples[i];
				float* outputSamples = m_outputSamples[i] + m_outputSamplesIn;
				T_ASSERT (m_outputSamplesIn + soundBlock.samplesCount < m_hwFrameSamples * c_outputSamplesBlockCount);

				if (inputSamples)
					mixer->mulConst(
						outputSamples,
						inputSamples,
						soundBlock.samplesCount,
						m_volume * m_activeState.volume
					);
				else
					mixer->mute(outputSamples, soundBlock.samplesCount);
			}
			m_outputSamplesIn += soundBlock.samplesCount;
		}
	}

	// Build output block.
	outBlock.samplesCount = m_hwFrameSamples;
	outBlock.sampleRate = m_hwSampleRate;
	outBlock.maxChannel = SbcMaxChannelCount;
	for (uint32_t i = 0; i < SbcMaxChannelCount; ++i)
		outBlock.samples[i] = m_outputSamples[i];

	return true;
}

	}
}
