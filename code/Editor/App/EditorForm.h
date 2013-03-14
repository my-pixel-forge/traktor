#ifndef traktor_editor_EditorForm_H
#define traktor_editor_EditorForm_H

#include <list>
#include "Core/Guid.h"
#include "Core/Io/Path.h"
#include "Core/Library/Library.h"
#include "Core/Thread/Semaphore.h"
#include "Editor/IEditor.h"
#include "Ui/Command.h"
#include "Ui/Form.h"

namespace traktor
{

class CommandLine;
class Thread;

	namespace net
	{

class DiscoveryManager;
class StreamServer;

	}

	namespace ui
	{

class ShortcutTable;
class Dock;
class DockPane;
class MenuBar;
class PopupMenu;
class MenuItem;
class Tab;
class Event;

		namespace custom
		{

class ToolBar;
class StatusBar;
class ProgressBar;

		}
	}

	namespace db
	{

class ConnectionManager;
class Database;

	}

	namespace editor
	{

class BuildView;
class DatabaseView;
class Document;
class EditorPageSite;
class EditorPluginSite;
class IEditorPage;
class IEditorPageFactory;
class IEditorPluginFactory;
class IEditorTool;
class IObjectEditor;
class IObjectEditorFactory;
class LogView;
class MRU;
class PipelineAgentsManager;
class PropertiesView;

/*! \brief Main editor form.
 *
 * This is the surrounding form containing editor pages and the
 * database view.
 */
class EditorForm
:	public ui::Form
,	public IEditor
{
	T_RTTI_CLASS;

public:
	EditorForm();

	bool create(const CommandLine& cmdLine);

	void destroy();

	/*! \name IEditor implementation */
	//@{

	virtual Ref< const PropertyGroup > getSettings() const;

	virtual Ref< const PropertyGroup > getGlobalSettings() const;

	virtual Ref< const PropertyGroup > getWorkspaceSettings() const;

	virtual Ref< PropertyGroup > checkoutGlobalSettings();

	virtual void commitGlobalSettings();

	virtual void revertGlobalSettings();

	virtual Ref< PropertyGroup > checkoutWorkspaceSettings();

	virtual void commitWorkspaceSettings();

	virtual void revertWorkspaceSettings();

	virtual Ref< db::Database > getSourceDatabase() const;

	virtual Ref< db::Database > getOutputDatabase() const;

	virtual void updateDatabaseView();

	virtual bool highlightInstance(const db::Instance* instance);

	virtual const TypeInfo* browseType(const TypeInfo* base);

	virtual Ref< db::Instance > browseInstance(const TypeInfo& filterType);

	virtual Ref< db::Instance > browseInstance(const IBrowseFilter* filter);

	virtual bool openEditor(db::Instance* instance);

	virtual bool openDefaultEditor(db::Instance* instance);

	virtual Ref< IEditorPage > getActiveEditorPage();

	virtual void setActiveEditorPage(IEditorPage* editorPage);

	virtual void buildAssets(const std::vector< Guid >& assetGuids, bool rebuild);

	virtual void buildAsset(const Guid& assetGuid, bool rebuild);

	virtual void buildAssets(bool rebuild);

	virtual bool buildAssetDependencies(const ISerializable* asset, uint32_t recursionDepth, RefArray< PipelineDependency >& outDependencies);

	virtual void setStoreObject(const std::wstring& name, Object* object);

	virtual Object* getStoreObject(const std::wstring& name) const;

	//@}

private:
	friend class EditorPageSite;
	friend class EditorPluginSite;

	RefArray< IEditorPageFactory > m_editorPageFactories;
	RefArray< IObjectEditorFactory > m_objectEditorFactories;
	RefArray< IEditorPluginFactory > m_editorPluginFactories;
	RefArray< IEditorTool > m_editorTools;
	RefArray< EditorPluginSite > m_editorPluginSites;
	Ref< net::DiscoveryManager > m_discoveryManager;
	Ref< net::StreamServer > m_streamServer;
	Ref< db::ConnectionManager > m_dbConnectionManager;
	Ref< PipelineAgentsManager > m_agentsManager;
	std::map< std::wstring, Ref< Object > > m_objectStore;
	Ref< MRU > m_mru;
	std::list< ui::Command > m_shortcutCommands;
	Ref< ui::ShortcutTable > m_shortcutTable;
	Ref< ui::Dock > m_dock;
	Ref< ui::DockPane > m_paneWest;
	Ref< ui::DockPane > m_paneEast;
	Ref< ui::DockPane > m_paneSouth;
	Ref< ui::MenuBar > m_menuBar;
	Ref< ui::MenuItem > m_menuItemRecent;
	Ref< ui::MenuItem > m_menuItemOtherPanels;
	Ref< ui::custom::ToolBar > m_toolBar;
	Ref< ui::custom::StatusBar > m_statusBar;
	Ref< ui::custom::ProgressBar > m_buildProgress;
	Ref< ui::Tab > m_tab;
	Ref< ui::PopupMenu > m_menuTab;
	Ref< ui::MenuItem > m_menuTools;
	Ref< DatabaseView > m_dataBaseView;
	Ref< PropertiesView > m_propertiesView;
	Ref< LogView > m_logView;
	Ref< BuildView > m_buildView;
	Ref< db::Database > m_sourceDatabase;
	Ref< db::Database > m_outputDatabase;
	Ref< IEditorPage > m_activeEditorPage;
	Ref< Document > m_activeDocument;
	Ref< EditorPageSite > m_activeEditorPageSite;
	std::vector< Guid > m_eventIds;
	Thread* m_threadAssetMonitor;
	Thread* m_threadBuild;
	Semaphore m_lockBuild;
	Path m_workspacePath;
	Ref< PropertyGroup > m_globalSettings;
	Ref< PropertyGroup > m_workspaceSettings;
	Ref< PropertyGroup > m_mergedSettings;

	bool createWorkspace();

	bool openWorkspace();

	bool openWorkspace(const Path& workspacePath);

	void closeWorkspace();

	void setPropertyObject(Object* properties);

	void createAdditionalPanel(ui::Widget* widget, int size, int32_t direction);

	void destroyAdditionalPanel(ui::Widget* widget);

	void showAdditionalPanel(ui::Widget* widget);

	void hideAdditionalPanel(ui::Widget* widget);

	void updateAdditionalPanelMenu();

	void buildAssetsThread(std::vector< Guid > assetGuids, bool rebuild);

	void buildCancel();

	void updateMRU();

	void updateTitle();

	void updateShortcutTable();

	void saveCurrentDocument();

	void saveAllDocuments();

	void closeCurrentEditor();

	void closeAllEditors();

	void closeAllOtherEditors();

	void findInDatabase();

	void activatePreviousEditor();

	void activateNextEditor();

	void loadLanguageDictionary();

	void loadHelpDictionary();

	void checkModified();

	bool currentModified();

	bool anyModified();

	bool handleCommand(const ui::Command& command);

	/*! \name Event handlers. */
	//@{

	void eventShortcut(ui::Event* event);

	void eventMenuClick(ui::Event* event);

	void eventToolClicked(ui::Event* event);

	void eventTabButtonDown(ui::Event* event);

	void eventTabSelChange(ui::Event* event);

	void eventTabClose(ui::Event* event);

	void eventClose(ui::Event* event);

	void eventTimer(ui::Event* event);

	//@}

	/*! \name Monitor thread methods. */
	//@{

	void threadAssetMonitor();

	//@}
};

	}
}

#endif	// traktor_editor_EditorForm_H
