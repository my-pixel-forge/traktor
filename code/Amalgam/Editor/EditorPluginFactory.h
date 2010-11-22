#ifndef traktor_amalgam_EditorPluginFactory_H
#define traktor_amalgam_EditorPluginFactory_H

#include "Editor/IEditorPluginFactory.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_AMALGAM_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace amalgam
	{

class T_DLLCLASS EditorPluginFactory : public editor::IEditorPluginFactory
{
	T_RTTI_CLASS;

public:
	virtual void getCommands(std::list< ui::Command >& outCommands) const;

	virtual Ref< editor::IEditorPlugin > createEditorPlugin(editor::IEditor* editor) const;
};

	}
}

#endif	// traktor_amalgam_EditorPluginFactory_H
