#pragma once

#include "Editor/ISettingsPage.h"
#include "Ui/CheckBox.h"
#include "Ui/Edit.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_RUNTIME_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace runtime
	{

/*! Deployment editor settings page.
 * \ingroup Runtime
 */
class T_DLLCLASS DeploySettingsPage : public editor::ISettingsPage
{
	T_RTTI_CLASS;

public:
	virtual bool create(ui::Container* parent, const PropertyGroup* originalSettings, PropertyGroup* settings, const std::list< ui::Command >& shortcutCommands) override final;

	virtual void destroy() override final;

	virtual bool apply(PropertyGroup* settings) override final;

private:
	Ref< ui::CheckBox > m_checkInheritCache;
	Ref< ui::CheckBox > m_checkHidePipeline;
	Ref< ui::CheckBox > m_checkUseDebugBinaries;
	Ref< ui::CheckBox > m_checkStaticallyLinked;
	Ref< ui::Edit > m_editAndroidHome;
	Ref< ui::Edit > m_editAndroidNdkRoot;
	Ref< ui::Edit > m_editAndroidToolchain;
	Ref< ui::Edit > m_editAndroidApiLevel;
	Ref< ui::Edit > m_editEmscripten;
};

	}
}

