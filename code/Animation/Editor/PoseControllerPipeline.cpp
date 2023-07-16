/*
 * TRAKTOR
 * Copyright (c) 2022 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Animation/Animation/AnimationGraphPoseControllerData.h"
#include "Animation/IK/IKPoseControllerData.h"
#include "Animation/RagDoll/RagDollPoseControllerData.h"
#include "Animation/Editor/PoseControllerPipeline.h"
#include "Editor/IPipelineDepends.h"

namespace traktor::animation
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.animation.PoseControllerPipeline", 0, PoseControllerPipeline, editor::DefaultPipeline)

TypeInfoSet PoseControllerPipeline::getAssetTypes() const
{
	return makeTypeInfoSet<
		AnimationGraphPoseControllerData,
		IKPoseControllerData,
		RagDollPoseControllerData
	>();
}

bool PoseControllerPipeline::buildDependencies(
	editor::IPipelineDepends* pipelineDepends,
	const db::Instance* sourceInstance,
	const ISerializable* sourceAsset,
	const std::wstring& outputPath,
	const Guid& outputGuid
) const
{
	if (const AnimationGraphPoseControllerData* statePoseControllerData = dynamic_type_cast< const AnimationGraphPoseControllerData* >(sourceAsset))
		pipelineDepends->addDependency(statePoseControllerData->getStateGraph(), editor::PdfBuild | editor::PdfResource);
	else if (const IKPoseControllerData* ikPoseContollerData = dynamic_type_cast< const IKPoseControllerData* >(sourceAsset))
		pipelineDepends->addDependency(ikPoseContollerData->getNeutralPoseController());
	else if (const RagDollPoseControllerData* ragDollPoseContollerData = dynamic_type_cast< const RagDollPoseControllerData* >(sourceAsset))
	{
		pipelineDepends->addDependency(ragDollPoseContollerData->getTrackPoseController());
		for (auto id : ragDollPoseContollerData->getCollisionGroup())
			pipelineDepends->addDependency(id, editor::PdfBuild | editor::PdfResource);
		for (auto id : ragDollPoseContollerData->getCollisionMask())
			pipelineDepends->addDependency(id, editor::PdfBuild | editor::PdfResource);
	}
	return true;
}

}
