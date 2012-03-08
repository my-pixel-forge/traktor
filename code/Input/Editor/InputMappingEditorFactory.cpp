#include "Input/Binding/InputMappingSourceData.h"
#include "Input/Editor/InputMappingEditor.h"
#include "Input/Editor/InputMappingEditorFactory.h"

namespace traktor
{
	namespace input
	{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.input.InputMappingEditorFactory", 0, InputMappingEditorFactory, editor::IObjectEditorFactory)

const TypeInfoSet InputMappingEditorFactory::getEditableTypes() const
{
	TypeInfoSet typeSet;
	typeSet.insert(&type_of< InputMappingSourceData >());
	return typeSet;
}

Ref< editor::IObjectEditor > InputMappingEditorFactory::createObjectEditor(editor::IEditor* editor) const
{
	return new InputMappingEditor(editor);
}

	}
}
