/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#include "Sound/Editor/Processor/GraphAsset.h"
#include "Sound/Editor/Processor/GraphEditor.h"
#include "Sound/Editor/Processor/GraphEditorFactory.h"

namespace traktor
{
	namespace sound
	{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.sound.GraphEditorFactory", 0, GraphEditorFactory, editor::IEditorPageFactory)

const TypeInfoSet GraphEditorFactory::getEditableTypes() const
{
	TypeInfoSet typeSet;
	typeSet.insert(&type_of< GraphAsset >());
	return typeSet;
}

bool GraphEditorFactory::needOutputResources(const TypeInfo& typeInfo, std::set< Guid >& outDependencies) const
{
	return false;
}

Ref< editor::IEditorPage > GraphEditorFactory::createEditorPage(editor::IEditor* editor, editor::IEditorPageSite* site, editor::IDocument* document) const
{
	return new GraphEditor(editor, site, document);
}

void GraphEditorFactory::getCommands(std::list< ui::Command >& outCommands) const
{
}

	}
}
