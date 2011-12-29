#include "Core/Log/Log.h"
#include "Editor/IEditor.h"
#include "Scene/Editor/ISceneEditorPlugin.h"
#include "Scene/Editor/ISceneEditorProfile.h"
#include "Scene/Editor/SceneAsset.h"
#include "Scene/Editor/SceneEditorPage.h"
#include "Scene/Editor/SceneEditorPageFactory.h"
#include "Ui/Command.h"
#include "World/Entity/EntityData.h"

namespace traktor
{
	namespace scene
	{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.scene.SceneEditorPageFactory", 0, SceneEditorPageFactory, editor::IEditorPageFactory)

const TypeInfoSet SceneEditorPageFactory::getEditableTypes() const
{
	TypeInfoSet typeSet;
	typeSet.insert(&type_of< SceneAsset >());
	typeSet.insert(&type_of< world::EntityData >());
	return typeSet;
}

Ref< editor::IEditorPage > SceneEditorPageFactory::createEditorPage(editor::IEditor* editor, editor::IEditorPageSite* site, editor::IDocument* document) const
{
	return new SceneEditorPage(editor, site, document);
}

void SceneEditorPageFactory::getCommands(std::list< ui::Command >& outCommands) const
{
	// Add editor commands.
	outCommands.push_back(ui::Command(L"Scene.Editor.AddEntity"));
	outCommands.push_back(ui::Command(L"Scene.Editor.Translate"));
	outCommands.push_back(ui::Command(L"Scene.Editor.Rotate"));
	outCommands.push_back(ui::Command(L"Scene.Editor.TogglePick"));
	outCommands.push_back(ui::Command(L"Scene.Editor.ToggleX"));
	outCommands.push_back(ui::Command(L"Scene.Editor.ToggleY"));
	outCommands.push_back(ui::Command(L"Scene.Editor.ToggleZ"));
	outCommands.push_back(ui::Command(L"Scene.Editor.ToggleGrid"));
	outCommands.push_back(ui::Command(L"Scene.Editor.ToggleGuide"));
	outCommands.push_back(ui::Command(L"Scene.Editor.ToggleSnap"));
	outCommands.push_back(ui::Command(L"Scene.Editor.Rewind"));
	outCommands.push_back(ui::Command(L"Scene.Editor.Play"));
	outCommands.push_back(ui::Command(L"Scene.Editor.Stop"));
	outCommands.push_back(ui::Command(L"Scene.Editor.SingleView"));
	outCommands.push_back(ui::Command(L"Scene.Editor.DoubleView"));
	outCommands.push_back(ui::Command(L"Scene.Editor.QuadrupleView"));

	// Add profile commands.
	std::vector< const TypeInfo* > profileTypes;
	type_of< ISceneEditorProfile >().findAllOf(profileTypes);
	for (std::vector< const TypeInfo* >::const_iterator i = profileTypes.begin(); i != profileTypes.end(); ++i)
	{
		Ref< ISceneEditorProfile > profile = dynamic_type_cast< ISceneEditorProfile* >((*i)->createInstance());
		if (profile)
			profile->getCommands(outCommands);
	}
}

	}
}
