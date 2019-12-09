#include "Core/Thread/Signal.h"
#include "Core/Timer/Timer.h"
#include "Sound/AudioChannel.h"
#include "Sound/ISoundBuffer.h"
#include "Sound/IAudioDriver.h"
#include "Sound/Sound.h"
#include "Sound/SoundSystem.h"
#include "UnitTest/CaseSound.h"

namespace traktor
{
	namespace
	{

class TestAudioDriver : public RefCountImpl< sound::IAudioDriver >
{
public:
	Timer& m_timer;
	Signal& m_signal;
	double m_signalAt;

	TestAudioDriver(Timer& timer, Signal& signal)
	:	m_timer(timer)
	,	m_signal(signal)
	,	m_signalAt(0.0)
	{
	}

	virtual bool create(const SystemApplication& sysapp, const sound::AudioDriverCreateDesc& desc, Ref< sound::IAudioMixer >& outMixer) override final
	{
		return true;
	}

	virtual void destroy() override final
	{
	}

	virtual void wait() override final
	{
	}

	virtual void submit(const sound::SoundBlock& soundBlock) override final
	{
		if (m_signal.wait(0))
			return;

		for (uint32_t i = 0; i < soundBlock.samplesCount; ++i)
		{
			if (soundBlock.samples[0][i] > 1.0f)
			{
				m_signalAt = m_timer.getElapsedTime();
				m_signal.set();
			}
		}
	}
};

class TestSoundBuffer : public RefCountImpl< sound::ISoundBuffer >
{
public:
	mutable float m_block[16];

	TestSoundBuffer()
	{
		for (int i = 0; i < sizeof_array(m_block); ++i)
			m_block[i] = 2.0f;
	}

	virtual Ref< sound::ISoundBufferCursor > createCursor() const override final
	{
		return 0;
	}

	virtual bool getBlock(sound::ISoundBufferCursor* cursor, const sound::IAudioMixer* mixer, sound::SoundBlock& outBlock) const override final
	{
		outBlock.samples[0] = m_block;
		outBlock.samplesCount = sizeof_array(m_block);
		outBlock.sampleRate = 44100;
		outBlock.maxChannel = 1;
		return true;
	}
};

	}

void CaseSound::run()
{
	Timer timer;
	Signal signal;

	TestAudioDriver soundDriver(timer, signal);
	sound::SoundSystem soundSystem(&soundDriver);

	sound::SoundSystemCreateDesc desc;
	desc.channels = 1;
	desc.driverDesc.sampleRate = 44100;
	desc.driverDesc.bitsPerSample = 16;
	desc.driverDesc.hwChannels = 1;
	desc.driverDesc.frameSamples = 1500;

	soundSystem.create(desc);

	TestSoundBuffer soundBuffer;

	timer.start();
	soundSystem.getChannel(0)->play(&soundBuffer, 0, 1.0f, 0.0f, 0.0f, false, 0);

	signal.wait();

	double latency = soundDriver.m_signalAt * 1000;
	CASE_ASSERT (latency < 10);

	soundSystem.destroy();
}

}
