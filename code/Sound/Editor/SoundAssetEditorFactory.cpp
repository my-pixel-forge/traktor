#include "Sound/Editor/SoundAsset.h"
#include "Sound/Editor/SoundAssetEditor.h"
#include "Sound/Editor/SoundAssetEditorFactory.h"

namespace traktor
{
	namespace sound
	{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.sound.SoundAssetEditorFactory", 0, SoundAssetEditorFactory, editor::IObjectEditorFactory)

const TypeInfoSet SoundAssetEditorFactory::getEditableTypes() const
{
	TypeInfoSet typeSet;
	typeSet.insert(&type_of< SoundAsset >());
	return typeSet;
}

Ref< editor::IObjectEditor > SoundAssetEditorFactory::createObjectEditor(editor::IEditor* editor) const
{
	return new SoundAssetEditor(editor);
}

	}
}
