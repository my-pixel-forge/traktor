#include "Animation/Editor/AnimatedMeshEntityEditor.h"
#include "Animation/AnimatedMeshEntityData.h"
#include "Animation/AnimatedMeshEntity.h"
#include "Animation/Skeleton.h"
#include "Animation/SkeletonUtils.h"
#include "Animation/Bone.h"
#include "Animation/IPoseController.h"
#include "Scene/Editor/EntityAdapter.h"
#include "Scene/Editor/SceneEditorContext.h"
#include "Render/PrimitiveRenderer.h"

namespace traktor
{
	namespace animation
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.animation.AnimatedMeshEntityEditor", AnimatedMeshEntityEditor, scene::DefaultEntityEditor)

void AnimatedMeshEntityEditor::drawGuide(
	scene::SceneEditorContext* context,
	render::PrimitiveRenderer* primitiveRenderer,
	scene::EntityAdapter* entityAdapter
) const
{
	const AnimatedMeshEntityData* animatedEntityData = checked_type_cast< const AnimatedMeshEntityData* >(entityAdapter->getEntityData());
	const AnimatedMeshEntity* animatedEntity = checked_type_cast< const AnimatedMeshEntity* >(entityAdapter->getEntity());

	primitiveRenderer->pushWorld(entityAdapter->getTransform().toMatrix44());

	resource::Proxy< Skeleton > skeleton = animatedEntityData->getSkeleton();
	const AlignedVector< Transform >& poseTransforms = animatedEntity->getPoseTransforms();

	if (skeleton.valid() && poseTransforms.size() == skeleton->getBoneCount())
	{
		const Color color(255, 255, 0, 180);
		for (uint32_t i = 0; i < skeleton->getBoneCount(); ++i)
		{
			const Bone* bone = skeleton->getBone(i);

			Vector4 start = poseTransforms[i].translation().xyz1();
			Vector4 end = (poseTransforms[i].translation() + poseTransforms[i] * Vector4(0.0f, 0.0f, bone->getLength(), 0.0f)).xyz1();

			Vector4 d = poseTransforms[i].axisZ();
			Vector4 a = poseTransforms[i].axisX();
			Vector4 b = poseTransforms[i].axisY();

			Scalar radius = bone->getRadius();
			d *= radius;
			a *= radius;
			b *= radius;

			primitiveRenderer->drawLine(start, start + d + a + b, color);
			primitiveRenderer->drawLine(start, start + d - a + b, color);
			primitiveRenderer->drawLine(start, start + d + a - b, color);
			primitiveRenderer->drawLine(start, start + d - a - b, color);

			primitiveRenderer->drawLine(start + d + a + b, end, color);
			primitiveRenderer->drawLine(start + d - a + b, end, color);
			primitiveRenderer->drawLine(start + d + a - b, end, color);
			primitiveRenderer->drawLine(start + d - a - b, end, color);

			primitiveRenderer->drawLine(start + d + a + b, start + d - a + b, color);
			primitiveRenderer->drawLine(start + d - a + b, start + d - a - b, color);
			primitiveRenderer->drawLine(start + d - a - b, start + d + a - b, color);
			primitiveRenderer->drawLine(start + d + a - b, start + d + a + b, color);

			primitiveRenderer->drawLine(start, end, Color(255, 255, 128, 180));
			primitiveRenderer->drawLine(start, start + a * Scalar(2.0f), Color(255, 0, 0, 180));
			primitiveRenderer->drawLine(start, start + b * Scalar(2.0f), Color(0, 255, 0, 180));
		}
	}

	primitiveRenderer->popWorld();

	scene::DefaultEntityEditor::drawGuide(context, primitiveRenderer, entityAdapter);
}

	}
}
