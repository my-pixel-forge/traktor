/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_render_ScriptNodeFacade_H
#define traktor_render_ScriptNodeFacade_H

#include "Render/Editor/Shader/INodeFacade.h"

namespace traktor
{
	namespace ui
	{

class GraphControl;
class NodeShape;

	}

	namespace render
	{

class ShaderGraphEditorPage;

class ScriptNodeFacade : public INodeFacade
{
	T_RTTI_CLASS;

public:
	ScriptNodeFacade(ShaderGraphEditorPage* page, ui::GraphControl* graphControl);

	virtual Ref< Node > createShaderNode(
		const TypeInfo* nodeType,
		editor::IEditor* editor
	) override final;

	virtual Ref< ui::Node > createEditorNode(
		editor::IEditor* editor,
		ui::GraphControl* graphControl,
		ShaderGraph* shaderGraph,
		Node* shaderNode
	) override final;

	virtual void editShaderNode(
		editor::IEditor* editor,
		ui::GraphControl* graphControl,
		ui::Node* editorNode,
		ShaderGraph* shaderGraph,
		Node* shaderNode
	) override final;

	virtual void refreshEditorNode(
		editor::IEditor* editor,
		ui::GraphControl* graphControl,
		ui::Node* editorNode,
		ShaderGraph* shaderGraph,
		Node* shaderNode
	) override final;

	virtual void setValidationIndicator(
		ui::Node* editorNode,
		bool validationSucceeded
	) override final;

private:
	ShaderGraphEditorPage* m_page;
	Ref< ui::NodeShape > m_nodeShape;
};

	}
}

#endif	// traktor_render_ScriptNodeFacade_H
