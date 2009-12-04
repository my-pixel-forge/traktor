#include "Animation/Bone.h"
#include "Animation/Skeleton.h"
#include "Animation/SkeletonUtils.h"
#include "Animation/Animation/Animation.h"
#include "Animation/Editor/AnimationFormatBvh.h"
#include "Animation/Editor/SkeletonFormatBvh.h"
#include "Animation/Editor/BvhParser/BvhDocument.h"
#include "Animation/Editor/BvhParser/BvhJoint.h"
#include "Core/Log/Log.h"
#include "Core/Math/Const.h"
#include "Core/Misc/String.h"

namespace traktor
{
	namespace animation
	{
		namespace
		{

Quaternion calculateReferenceOrientation(const Skeleton* skeleton, int32_t boneIndex)
{
	if (boneIndex >= 0)
	{
		const Bone* bone = skeleton->getBone(boneIndex);
		T_ASSERT (bone);
		Quaternion QworldParent = calculateReferenceOrientation(skeleton, bone->getParent());
		Quaternion Qlocal = bone->getOrientation();
		return QworldParent * Qlocal;
	}
	else
		return Quaternion::identity();
}

void convertKeyPose(
	const Skeleton* skeleton,
	const BvhJoint* joint,
	const BvhDocument::cv_t& cv,
	Animation::KeyPose& kp,
	int32_t& boneIndex,
	const Quaternion& QparentWorldOrientation
)
{
	const RefArray< BvhJoint >& children = joint->getChildren();
	for (RefArray< BvhJoint >::const_iterator i = children.begin(); i != children.end(); ++i)
	{
		const BvhJoint* childJoint = *i;

		const std::vector< std::wstring >& ch = joint->getChannels();
		int32_t offset = joint->getChannelOffset();

		Vector4 P = Vector4::zero();
		Quaternion QlocalOrientation = Quaternion::identity();

		for (std::vector< std::wstring >::const_iterator k = ch.begin(); k != ch.end(); ++k)
		{
			float c = cv[offset++];

			if (*k == L"Xposition")
				P += Vector4(c, 0.0f, 0.0f, 0.0f);
			else if (*k == L"Yposition")
				P += Vector4(0.0f, c, 0.0f, 0.0f);
			else if (*k == L"Zposition")
				P += Vector4(0.0f, 0.0f, c, 0.0f);

			else if (*k == L"Xrotation")
				QlocalOrientation *= Quaternion(Vector4(deg2rad(c), 0.0f, 0.0f, 0.0f));
			else if (*k == L"Yrotation")
				QlocalOrientation *= Quaternion(Vector4(0.0f, deg2rad(c), 0.0f, 0.0f));
			else if (*k == L"Zrotation")
				QlocalOrientation *= Quaternion(Vector4(0.0f, 0.0f, deg2rad(c), 0.0f));
		}

		Quaternion Qref = calculateReferenceOrientation(skeleton, boneIndex);

		kp.pose.setBoneOrientation(
			boneIndex,
			Qref.inverse() * QlocalOrientation * Qref
		);

		boneIndex++;

		convertKeyPose(
			skeleton,
			childJoint,
			cv,
			kp,
			boneIndex,
			QparentWorldOrientation * QlocalOrientation
		);
	}
}

		}

T_IMPLEMENT_RTTI_CLASS(L"traktor.animation.AnimationFormatBvh", AnimationFormatBvh, IAnimationFormat)

Ref< Animation > AnimationFormatBvh::import(IStream* stream) const
{
	Ref< BvhDocument > document = BvhDocument::parse(stream);
	if (!document)
		return 0;

	// Create reference skeleton from BVH hierarchy; use
	// SkeletonFormatBvh class to ensure consistency with skeleton pipeline.
	SkeletonFormatBvh skeletonFormat;
	Ref< Skeleton > skeleton = skeletonFormat.create(document);
	if (!skeleton)
		return 0;

	// Create key poses from BVH motions.
	Ref< Animation > anim = new Animation();

	float at = 0.0f;

	const BvhDocument::cv_list_t& cvl = document->getChannelValues();
	for (BvhDocument::cv_list_t::const_iterator i = cvl.begin(); i != cvl.end(); ++i)
	{
		const BvhDocument::cv_t& cv = *i;

		Animation::KeyPose kp;
		kp.at = at;

		int32_t boneIndex = 0;
		convertKeyPose(
			skeleton,
			document->getRootJoint(),
			*i,
			kp,
			boneIndex,
			Quaternion::identity()
		);

		anim->addKeyPose(kp);

		at += document->getFrameTime();
	}
	
	return anim;
}

	}
}
