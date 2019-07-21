#include "Editor/IEditorPageSite.h"
#include "Shape/Editor/Bake/BakePipelineOperator.h"
#include "Shape/Editor/Bake/TracerEditorPlugin.h"
#include "Shape/Editor/Bake/TracerPanel.h"
#include "Shape/Editor/Bake/TracerProcessor.h"
#include "Ui/Application.h"

namespace traktor
{
    namespace shape
    {

T_IMPLEMENT_RTTI_CLASS(L"traktor.shape.TracerEditorPlugin", TracerEditorPlugin, editor::IEditorPlugin)

TracerEditorPlugin::TracerEditorPlugin(editor::IEditor* editor)
:   m_editor(editor)
,	m_site(nullptr)
{
}

bool TracerEditorPlugin::create(ui::Widget* parent, editor::IEditorPageSite* site)
{
    BakePipelineOperator::setTracerProcessor(new TracerProcessor(m_editor));

	m_site = site;

	m_tracerPanel = new TracerPanel();
	m_tracerPanel->create(parent);

	m_site->createAdditionalPanel(m_tracerPanel, ui::dpi96(100), false);

    return true;
}

void TracerEditorPlugin::destroy()
{
	if (m_tracerPanel)
	{
		m_site->destroyAdditionalPanel(m_tracerPanel);
		m_site = nullptr;
		m_tracerPanel = nullptr;
	}

    BakePipelineOperator::setTracerProcessor(nullptr);
}

bool TracerEditorPlugin::handleCommand(const ui::Command& command, bool result)
{
    return false;
}

void TracerEditorPlugin::handleDatabaseEvent(db::Database* database, const Guid& eventId)
{
}

void TracerEditorPlugin::handleWorkspaceOpened()
{
}

void TracerEditorPlugin::handleWorkspaceClosed()
{
}

    }
}