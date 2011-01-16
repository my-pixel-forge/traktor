#ifndef traktor_animation_SkeletonUtils_H
#define traktor_animation_SkeletonUtils_H

#include "Core/Config.h"
#include "Core/Containers/AlignedVector.h"
#include "Core/Math/Transform.h"
#include "Core/Math/Aabb3.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_ANIMATION_EXPORT)
#define T_DLLCLASS T_DLLEXPORT
#else
#define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace animation
	{

class Skeleton;
class Pose;

/*! \ingroup Animation */
//@{

void T_DLLCLASS calculateBoneLocalTransforms(
	const Skeleton* skeleton,
	AlignedVector< Transform >& outBoneTransforms
);

void T_DLLCLASS calculateBoneTransforms(
	const Skeleton* skeleton,
	AlignedVector< Transform >& outBoneTransforms
);

void T_DLLCLASS calculatePoseLocalTransforms(
	const Skeleton* skeleton,
	const Pose* pose,
	AlignedVector< Transform >& outBoneLocalTransforms
);

void T_DLLCLASS calculatePoseTransforms(
	const Skeleton* skeleton,
	const Pose* pose,
	AlignedVector< Transform >& outBoneTransforms
);

Aabb3 T_DLLCLASS calculateBoundingBox(const Skeleton* skeleton);

Aabb3 T_DLLCLASS calculateBoundingBox(const Skeleton* skeleton, const Pose* pose);

void T_DLLCLASS blendPoses(
	const Pose* pose1,
	const Pose* pose2,
	const Scalar& blend,
	Pose* outPose
);

//@}

	}
}

#endif	// traktor_animation_SkeletonUtils_H
