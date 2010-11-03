#include <algorithm>
#include "Animation/SkeletonUtils.h"
#include "Animation/Skeleton.h"
#include "Animation/Bone.h"
#include "Animation/Pose.h"

namespace traktor
{
	namespace animation
	{

void calculateBoneLocalTransforms(
	const Skeleton* skeleton,
	AlignedVector< Transform >& outBoneLocalTransforms
)
{
	T_ASSERT (skeleton);
	outBoneLocalTransforms.resize(skeleton->getBoneCount());
	for (uint32_t i = 0; i < skeleton->getBoneCount(); ++i)
		outBoneLocalTransforms[i] =
			Transform(skeleton->getBone(i)->getPosition()) *
			Transform(skeleton->getBone(i)->getOrientation());
}

void calculateBoneTransforms(
	const Skeleton* skeleton,
	AlignedVector< Transform >& outBoneTransforms
)
{
	T_ASSERT (skeleton);

	AlignedVector< Transform > localBoneTransforms;
	calculateBoneLocalTransforms(skeleton, localBoneTransforms);

	outBoneTransforms.resize(skeleton->getBoneCount());
	for (uint32_t i = 0; i < skeleton->getBoneCount(); ++i)
	{
		outBoneTransforms[i] = localBoneTransforms[i];
		for (int32_t boneIndex = skeleton->getBone(i)->getParent(); boneIndex >= 0; boneIndex = skeleton->getBone(boneIndex)->getParent())
		{
			Transform localParentTransform = localBoneTransforms[boneIndex] * Transform(Vector4(0.0f, 0.0f, skeleton->getBone(boneIndex)->getLength()));
			outBoneTransforms[i] = localParentTransform * outBoneTransforms[i];
		}
	}
}

void calculatePoseLocalTransforms(
	const Skeleton* skeleton,
	const Pose* pose,
	AlignedVector< Transform >& outBoneLocalTransforms
)
{
	T_ASSERT (skeleton);
	T_ASSERT (pose);

	outBoneLocalTransforms.resize(skeleton->getBoneCount());
	for (uint32_t i = 0; i < skeleton->getBoneCount(); ++i)
		outBoneLocalTransforms[i] = 
			Transform(skeleton->getBone(i)->getPosition() + pose->getBoneOffset(i)) *
			Transform(skeleton->getBone(i)->getOrientation() * pose->getBoneOrientation(i));
}

void calculatePoseTransforms(
	const Skeleton* skeleton,
	const Pose* pose,
	AlignedVector< Transform >& outBoneTransforms
)
{
	T_ASSERT (skeleton);
	T_ASSERT (pose);

	AlignedVector< Transform > localPoseTransforms;
	calculatePoseLocalTransforms(skeleton, pose, localPoseTransforms);

	outBoneTransforms.resize(skeleton->getBoneCount());
	for (uint32_t i = 0; i < skeleton->getBoneCount(); ++i)
	{
		Transform boneTransform = localPoseTransforms[i];
		for (int32_t boneIndex = skeleton->getBone(i)->getParent(); boneIndex >= 0; boneIndex = skeleton->getBone(boneIndex)->getParent())
		{
			Transform localParentTransform = localPoseTransforms[boneIndex] * Transform(Vector4(0.0f, 0.0f, skeleton->getBone(boneIndex)->getLength()));
			boneTransform = localParentTransform * boneTransform;
		}
		outBoneTransforms[i] = boneTransform;
	}
}

Aabb calculateBoundingBox(const Skeleton* skeleton)
{
	Aabb boundingBox;

	AlignedVector< Transform > boneTransforms;
	calculateBoneTransforms(skeleton, boneTransforms);

	for (uint32_t i = 0; i < uint32_t(boneTransforms.size()); ++i)
	{
		const Bone* bone = skeleton->getBone(i);

		float length = bone->getLength();
		float radius = bone->getRadius();

		Aabb boneLocalAabb(Vector4(-radius, -radius, 0.0f), Vector4(radius, radius, length));
		Aabb boneAabb = boneLocalAabb.transform(boneTransforms[i]);

		boundingBox.contain(boneAabb);
	}

	return boundingBox;
}

Aabb calculateBoundingBox(const Skeleton* skeleton, const Pose* pose)
{
	Aabb boundingBox;

	AlignedVector< Transform > poseTransforms;
	calculatePoseTransforms(skeleton, pose, poseTransforms);

	for (uint32_t i = 0; i < uint32_t(poseTransforms.size()); ++i)
	{
		const Bone* bone = skeleton->getBone(i);

		float length = bone->getLength();
		float radius = bone->getRadius();

		Aabb boneLocalAabb(Vector4(-radius, -radius, 0.0f), Vector4(radius, radius, length));
		Aabb boneAabb = boneLocalAabb.transform(poseTransforms[i]);

		boundingBox.contain(boneAabb);
	}

	return boundingBox;
}

void blendPoses(
	const Pose* pose1,
	const Pose* pose2,
	const Scalar& blend,
	Pose* outPose
)
{
	T_ASSERT (pose1);
	T_ASSERT (pose2);
	T_ASSERT (outPose);

	// Build mask of all used bone indices.
	BitSet indices;
	pose1->getIndexMask(indices);
	pose2->getIndexMask(indices);

	int minRange, maxRange;
	indices.range(minRange, maxRange);

	for (int i = minRange; i < maxRange; ++i)
	{
		if (!indices(i))
			continue;

		Vector4 o1 = pose1->getBoneOffset(i);
		Vector4 o2 = pose2->getBoneOffset(i);
		outPose->setBoneOffset(i, lerp(o1, o2, blend));

		Quaternion q1 = pose1->getBoneOrientation(i);
		Quaternion q2 = pose2->getBoneOrientation(i);
		outPose->setBoneOrientation(i, slerp(q1, q2, blend));
	}
}

	}
}
