/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_sound_SoundDriverXAudio2_H
#define traktor_sound_SoundDriverXAudio2_H

#if !defined(_XBOX)
#	define _WIN32_DCOM
#	include <windows.h>
#endif
#include <xaudio2.h>
#include "Core/Misc/ComRef.h"
#include "Sound/ISoundDriver.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_SOUND_XAUDIO2_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace sound
	{

/*!
 * \ingroup XA2
 */
class T_DLLCLASS SoundDriverXAudio2 : public ISoundDriver
{
	T_RTTI_CLASS;

public:
	SoundDriverXAudio2();

	virtual ~SoundDriverXAudio2();

	virtual bool create(const SystemApplication& sysapp, const SoundDriverCreateDesc& desc, Ref< ISoundMixer >& outMixer) override final;

	virtual void destroy() override final;

	virtual void wait() override final;

	virtual void submit(const SoundBlock& soundBlock) override final;

private:
	SoundDriverCreateDesc m_desc;
	ComRef< IXAudio2 > m_audio;
	IXAudio2EngineCallback* m_engineCallback;
	IXAudio2VoiceCallback* m_voiceCallback;
	IXAudio2MasteringVoice* m_masteringVoice;
	IXAudio2SourceVoice* m_sourceVoice;
	WAVEFORMATEXTENSIBLE m_wfx;
	HANDLE m_eventNotify;
	HRESULT m_hResult;
	uint32_t m_bufferSize;
	uint8_t* m_buffers[3];
	uint32_t m_nextSubmitBuffer;

	bool reset();
};

	}
}

#endif	// traktor_sound_SoundDriverXAudio2_H
