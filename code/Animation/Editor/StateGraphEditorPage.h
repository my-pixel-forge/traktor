/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_animation_StateGraphEditorPage_H
#define traktor_animation_StateGraphEditorPage_H

#include <map>
#include "Core/RefArray.h"
#include "Editor/IEditorPage.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_ANIMATION_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace editor
	{

class IDocument;
class IEditor;
class IEditorPageSite;

	}

	namespace ui
	{

class ButtonClickEvent;
class Container;
class MouseButtonDownEvent;
class Point;
class PopupMenu;
class SelectionChangeEvent;

		namespace custom
		{

class GraphControl;
class EdgeConnectEvent;
class EdgeDisconnectEvent;
class Node;
class NodeMovedEvent;
class ToolBar;
class ToolBarButtonClickEvent;

		}
	}

	namespace animation
	{

class AnimationPreviewControl;
class StateGraph;
class StateNode;
class StatePoseController;
class Transition;

/*! \brief
 * \ingroup Animation
 */
class T_DLLEXPORT StateGraphEditorPage : public editor::IEditorPage
{
	T_RTTI_CLASS;

public:
	StateGraphEditorPage(editor::IEditor* editor, editor::IEditorPageSite* site, editor::IDocument* document);

	virtual bool create(ui::Container* parent) T_OVERRIDE T_FINAL;

	virtual void destroy() T_OVERRIDE T_FINAL;

	virtual bool dropInstance(db::Instance* instance, const ui::Point& position) T_OVERRIDE T_FINAL;

	virtual bool handleCommand(const ui::Command& command) T_OVERRIDE T_FINAL;

	virtual void handleDatabaseEvent(db::Database* database, const Guid& eventId) T_OVERRIDE T_FINAL;

private:
	editor::IEditor* m_editor;
	editor::IEditorPageSite* m_site;
	editor::IDocument* m_document;
	Ref< StateGraph > m_stateGraph;
	Ref< StatePoseController > m_statePreviewController;
	Ref< ui::custom::ToolBar > m_toolBarGraph;
	Ref< ui::custom::GraphControl > m_editorGraph;
	Ref< ui::PopupMenu > m_menuPopup;
	Ref< ui::Container > m_containerPreview;
	Ref< ui::custom::ToolBar > m_toolBarPreview;
	Ref< AnimationPreviewControl > m_previewControl;
	Ref< ui::Container > m_previewConditions;

	void bindStateNodes();

	void createEditorNodes(const RefArray< StateNode >& states, const RefArray< Transition >& transitions);

	Ref< ui::custom::Node > createEditorNode(StateNode* state);

	void createState(const ui::Point& at);

	void updateGraph();

	void updatePreviewConditions();

	void eventToolBarGraphClick(ui::custom::ToolBarButtonClickEvent* event);

	void eventToolBarPreviewClick(ui::custom::ToolBarButtonClickEvent* event);

	void eventButtonDown(ui::MouseButtonDownEvent* event);

	void eventSelect(ui::SelectionChangeEvent* event);

	void eventNodeMoved(ui::custom::NodeMovedEvent* event);

	void eventEdgeConnect(ui::custom::EdgeConnectEvent* event);

	void eventEdgeDisconnect(ui::custom::EdgeDisconnectEvent* event);

	void eventPreviewConditionClick(ui::ButtonClickEvent* event);
};

	}
}

#endif	// traktor_animation_StateGraphEditorPage_H
