#ifndef traktor_sound_SoundPlayer_H
#define traktor_sound_SoundPlayer_H

#include "Core/Containers/AlignedVector.h"
#include "Core/Thread/Semaphore.h"
#include "Sound/Player/ISoundPlayer.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_SOUND_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace sound
	{

class EchoFilter;
class LowPassFilter;
class SoundChannel;
class SoundHandle;
class SurroundEnvironment;
class SurroundFilter;

/*! \brief High-level sound player implementation.
 * \ingroup Sound
 */
class T_DLLCLASS SoundPlayer : public ISoundPlayer
{
	T_RTTI_CLASS;

public:
	SoundPlayer();

	bool create(SoundSystem* soundSystem, SurroundEnvironment* surroundEnvironment);

	virtual void destroy();

	virtual Ref< ISoundHandle > play(const Sound* sound, uint32_t priority);

	virtual Ref< ISoundHandle > play3d(const Sound* sound, const Vector4& position, uint32_t priority);

	virtual void setListenerTransform(const Transform& listenerTransform);

	virtual void update(float dT);

private:
	struct Channel
	{
		Vector4 position;
		Ref< SurroundFilter > surroundFilter;
		Ref< LowPassFilter > lowPassFilter;
		Ref< EchoFilter > echoFilter;
		const Sound* sound;
		SoundChannel* soundChannel;
		uint32_t priority;
		float time;
		float fadeOff;
		Ref< SoundHandle > handle;
	};

	Semaphore m_lock;
	Ref< SoundSystem > m_soundSystem;
	Ref< SurroundEnvironment > m_surroundEnvironment;
	AlignedVector< Channel > m_channels;
	float m_time;
};

	}
}

#endif	// traktor_sound_SoundPlayer_H
