#include "Core/Settings/PropertyGroup.h"
#include "Core/Settings/PropertyString.h"
#include "Editor/App/WorkspaceDialog.h"
#include "I18N/Text.h"
#include "Ui/Edit.h"
#include "Ui/FloodLayout.h"
#include "Ui/MethodHandler.h"
#include "Ui/Static.h"
#include "Ui/TableLayout.h"
#include "Ui/Events/CommandEvent.h"

namespace traktor
{
	namespace editor
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.editor.WorkspaceDialog", WorkspaceDialog, ui::ConfigDialog)

bool WorkspaceDialog::create(ui::Widget* parent, PropertyGroup* settings)
{
	if (!ui::ConfigDialog::create(parent, i18n::Text(L"EDITOR_WORKSPACE_TITLE"), 500, 200, ui::ConfigDialog::WsDefaultResizable, new ui::FloodLayout()))
		return false;

	addClickEventHandler(ui::createMethodHandler(this, &WorkspaceDialog::eventDialogClick));

	Ref< ui::Container > containerInner = new ui::Container();
	if (!containerInner->create(this, ui::WsNone, new ui::TableLayout(L"*,100%", L"*", 8, 4)))
		return false;

	Ref< ui::Static > staticSourceDatabase = new ui::Static();
	staticSourceDatabase->create(containerInner, i18n::Text(L"EDITOR_SETTINGS_SOURCE_DATABASE"));

	m_editSourceDatabase = new ui::Edit();
	m_editSourceDatabase->create(containerInner, settings->getProperty< PropertyString >(L"Editor.SourceDatabase", L""));

	Ref< ui::Static > staticOutputDatabase = new ui::Static();
	staticOutputDatabase->create(containerInner, i18n::Text(L"EDITOR_SETTINGS_OUTPUT_DATABASE"));

	m_editOutputDatabase = new ui::Edit();
	m_editOutputDatabase->create(containerInner, settings->getProperty< PropertyString >(L"Editor.OutputDatabase", L""));

	Ref< ui::Static > staticAssetPath = new ui::Static();
	staticAssetPath->create(containerInner, i18n::Text(L"EDITOR_SETTINGS_ASSET_PATH"));

	m_editAssetPath = new ui::Edit();
	m_editAssetPath->create(containerInner, settings->getProperty< PropertyString >(L"Pipeline.AssetPath", L""));

	m_settings = settings;
	return true;
}

void WorkspaceDialog::eventDialogClick(ui::Event* event)
{
	if (checked_type_cast< ui::CommandEvent* >(event)->getCommand() == ui::DrOk)
	{
		m_settings->setProperty< PropertyString >(L"Editor.SourceDatabase", m_editSourceDatabase->getText());
		m_settings->setProperty< PropertyString >(L"Editor.OutputDatabase", m_editOutputDatabase->getText());
		m_settings->setProperty< PropertyString >(L"Pipeline.AssetPath", m_editAssetPath->getText());
	}
}

	}
}
