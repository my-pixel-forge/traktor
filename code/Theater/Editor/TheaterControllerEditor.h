#ifndef traktor_theater_TheaterControllerEditor_H
#define traktor_theater_TheaterControllerEditor_H

#include "Scene/Editor/ISceneControllerEditor.h"

namespace traktor
{
	namespace scene
	{

class PostFrameEvent;

	}

	namespace ui
	{

class SelectionChangeEvent;

		namespace custom
		{

class CursorMoveEvent;
class EditList;
class EditListEditEvent;
class KeyMoveEvent;
class ToolBar;
class ToolBarButtonClickEvent;
class SequencerControl;

		}
	}

	namespace theater
	{

/*! \brief
 */
class TheaterControllerEditor : public scene::ISceneControllerEditor
{
	T_RTTI_CLASS;

public:
	TheaterControllerEditor();

	virtual bool create(scene::SceneEditorContext* context, ui::Container* parent);

	virtual void destroy();

	virtual void entityRemoved(scene::EntityAdapter* entityAdapter);

	virtual void propertiesChanged();

	virtual bool handleCommand(const ui::Command& command);

	virtual void update();

	virtual void draw(render::PrimitiveRenderer* primitiveRenderer);

private:
	Ref< ui::custom::ToolBar > m_toolBarActs;
	Ref< ui::custom::EditList > m_listActs;
	Ref< ui::custom::ToolBar > m_toolBar;
	Ref< ui::custom::SequencerControl > m_trackSequencer;
	Ref< scene::SceneEditorContext > m_context;
	float m_timeOffset;

	void updateView();

	void captureEntities();

	void deleteSelectedKey();

	void setLookAtEntity();

	void easeVelocity();

	void gotoPreviousKey();

	void gotoNextKey();

	void eventActSelected(ui::SelectionChangeEvent* event);

	void eventActEdit(ui::custom::EditListEditEvent* event);

	void eventToolBarClick(ui::custom::ToolBarButtonClickEvent* event);

	void eventSequencerCursorMove(ui::custom::CursorMoveEvent* event);

	void eventSequencerKeyMove(ui::custom::KeyMoveEvent* event);

	void eventContextPostFrame(scene::PostFrameEvent* event);
};

	}
}

#endif	// traktor_theater_TheaterControllerEditor_H
