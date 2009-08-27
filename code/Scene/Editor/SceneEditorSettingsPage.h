#ifndef traktor_scene_SceneEditorSettingsPage_H
#define traktor_scene_SceneEditorSettingsPage_H

#include "Core/Heap/Ref.h"
#include "Editor/ISettingsPage.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_SCENE_EDITOR_EXPORT)
#define T_DLLCLASS T_DLLEXPORT
#else
#define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace ui
	{

class CheckBox;

	}

	namespace scene
	{

/*! \brief Scene editor settings page.
 * \ingroup Scene
 */
class SceneEditorSettingsPage : public editor::ISettingsPage
{
	T_RTTI_CLASS(SceneEditorSettingsPage)

public:
	virtual bool create(ui::Container* parent, editor::Settings* settings, const std::list< ui::Command >& shortcutCommands);

	virtual void destroy();

	virtual bool apply(editor::Settings* settings);

private:
	Ref< ui::CheckBox > m_checkInvertMouseWheel;
};

	}
}

#endif	// traktor_scene_SceneEditorSettingsPage_H
