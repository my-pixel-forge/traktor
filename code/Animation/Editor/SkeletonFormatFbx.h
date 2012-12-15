#ifndef traktor_animation_SkeletonFormatFbx_H
#define traktor_animation_SkeletonFormatFbx_H

#include "Animation/Editor/ISkeletonFormat.h"
#include "Core/Thread/Semaphore.h"

namespace traktor
{
	namespace animation
	{

class SkeletonFormatFbx : public ISkeletonFormat
{
	T_RTTI_CLASS;

public:
	virtual Ref< Skeleton > import(IStream* stream, const Vector4& offset, float radius, bool invertX, bool invertZ) const;

private:
	static Semaphore ms_lock;
};

	}
}

#endif	// traktor_animation_SkeletonFormatFbx_H
