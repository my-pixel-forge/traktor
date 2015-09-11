#include "Core/Misc/SafeDestroy.h"
#include "Database/Instance.h"
#include "Editor/IDocument.h"
#include "Editor/IEditor.h"
#include "Editor/IEditorPageSite.h"
#include "Render/IRenderSystem.h"
#include "Render/Resource/ShaderFactory.h"
#include "Resource/ResourceManager.h"
#include "Script/ScriptContextFactory.h"
#include "Script/Lua/ScriptManagerLua.h"
#include "Spark/Stage.h"
#include "Spark/ShapeResourceFactory.h"
#include "Spark/Editor/SparkEditControl.h"
#include "Spark/Editor/SparkEditorPage.h"
#include "Ui/Container.h"

namespace traktor
{
	namespace spark
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.spark.SparkEditorPage", SparkEditorPage, editor::IEditorPage)

SparkEditorPage::SparkEditorPage(editor::IEditor* editor, editor::IEditorPageSite* site, editor::IDocument* document)
:	m_editor(editor)
,	m_site(site)
,	m_document(document)
{
}

bool SparkEditorPage::create(ui::Container* parent)
{
	Ref< render::IRenderSystem > renderSystem = m_editor->getStoreObject< render::IRenderSystem >(L"RenderSystem");
	if (!renderSystem)
		return false;

	Ref< db::Database > database = m_editor->getOutputDatabase();

	m_scriptManager = new script::ScriptManagerLua();

	m_resourceManager = new resource::ResourceManager(true);
	m_resourceManager->addFactory(new script::ScriptContextFactory(database, m_scriptManager));
	m_resourceManager->addFactory(new render::ShaderFactory(database, renderSystem));
	m_resourceManager->addFactory(new ShapeResourceFactory(database, renderSystem));

	m_editControl = new SparkEditControl(m_editor);
	m_editControl->create(parent, ui::WsNone, database, m_resourceManager, renderSystem);
	m_editControl->update();

	Ref< Stage > stage = m_document->getObject< Stage >(0);
	m_editControl->setStage(stage);

	m_site->setPropertyObject(stage);
	return true;
}

void SparkEditorPage::destroy()
{
	safeDestroy(m_editControl);
	safeDestroy(m_resourceManager);
	safeDestroy(m_scriptManager);
}

void SparkEditorPage::activate()
{
}

void SparkEditorPage::deactivate()
{
}

bool SparkEditorPage::dropInstance(db::Instance* instance, const ui::Point& position)
{
	return false;
}

bool SparkEditorPage::handleCommand(const ui::Command& command)
{
	return false;
}

void SparkEditorPage::handleDatabaseEvent(db::Database* database, const Guid& eventId)
{
	if (m_resourceManager)
		m_resourceManager->reload(eventId, false);

	m_editControl->update();
}

	}
}