#include "Animation/Cloth/ClothEntity.h"
#include "Animation/Editor/Cloth/ClothEntityEditor.h"
#include "Core/Math/Const.h"
#include "Render/PrimitiveRenderer.h"
#include "Scene/Editor/EntityAdapter.h"
#include "Scene/Editor/SceneEditorContext.h"
#include "Ui/Command.h"

namespace traktor
{
	namespace animation
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.animation.ClothEntityEditor", ClothEntityEditor, scene::DefaultEntityEditor)

ClothEntityEditor::ClothEntityEditor(scene::SceneEditorContext* context, scene::EntityAdapter* entityAdapter)
:	scene::DefaultEntityEditor(context, entityAdapter)
{
}

bool ClothEntityEditor::handleCommand(const ui::Command& command)
{
	if (command == L"Animation.Editor.Reset")
	{
		Ref< ClothEntity > clothEntity = checked_type_cast< ClothEntity* >(getEntityAdapter()->getEntity());
		clothEntity->reset();
		return true;
	}
	return scene::DefaultEntityEditor::handleCommand(command);
}

void ClothEntityEditor::drawGuide(render::PrimitiveRenderer* primitiveRenderer) const
{
	if (getContext()->shouldDrawGuide(L"Animation.Cloth"))
	{
		ClothEntity* clothEntity = checked_type_cast< ClothEntity* >(getEntityAdapter()->getEntity());

		Transform transform = clothEntity->getTransform();
		Aabb3 boundingBox = clothEntity->getBoundingBox();
		
		primitiveRenderer->pushWorld(transform.toMatrix44());
		primitiveRenderer->drawWireAabb(boundingBox, Color4ub(255, 255, 0, 200));

		if (getEntityAdapter()->isSelected())
		{
			const auto& nodes = clothEntity->getNodes();

			for (const auto& edge : clothEntity->getEdges())
			{
				primitiveRenderer->drawLine(
					nodes[edge.index[0]].position[0],
					nodes[edge.index[1]].position[0],
					Color4ub(255, 255, 255)
				);
			}

			for (const auto& node : nodes)
			{
				if (node.invMass <= Scalar(FUZZY_EPSILON))
				{
					primitiveRenderer->drawSolidAabb(
						node.position[0],
						Vector4(0.1f, 0.05f, 0.05f, 0.05f),
						Color4ub(255, 0, 255, 128)
					);
				}
			}
		}

		primitiveRenderer->popWorld();
	}
}

	}
}
