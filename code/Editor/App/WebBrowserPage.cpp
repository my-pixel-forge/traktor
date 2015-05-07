#include "Core/Settings/PropertyGroup.h"
#include "Core/Settings/PropertyString.h"
#include "Editor/IEditor.h"
#include "Editor/App/WebBrowserPage.h"
#include "Ui/Bitmap.h"
#include "Ui/TableLayout.h"
#include "Ui/WebBrowser.h"
#include "Ui/Custom/ToolBar/ToolBar.h"
#include "Ui/Custom/ToolBar/ToolBarButton.h"
#include "Ui/Custom/ToolBar/ToolBarButtonClickEvent.h"

// Resources
#include "Resources/Navigate.h"

namespace traktor
{
	namespace editor
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.editor.WebBrowserPage", WebBrowserPage, ui::Container)

WebBrowserPage::WebBrowserPage(IEditor* editor)
:	m_editor(editor)
{
}

bool WebBrowserPage::create(ui::Widget* parent)
{
	if (!ui::Container::create(parent, ui::WsNone, new ui::TableLayout(L"100%", L"*,100%", 0, 0)))
		return false;

	std::wstring url = m_editor->getSettings()->getProperty< PropertyString >(L"Editor.HomeUrl", L"about:blank");

	Ref< ui::custom::ToolBar > toolbar = new ui::custom::ToolBar();
	toolbar->create(this);
	toolbar->addImage(ui::Bitmap::load(c_ResourceNavigate, sizeof(c_ResourceNavigate), L"png"), 4);
	toolbar->addItem(new ui::custom::ToolBarButton(L"Home", 0, ui::Command(L"Editor.WebBrowser.Home")));
	toolbar->addItem(new ui::custom::ToolBarButton(L"Back", 1, ui::Command(L"Editor.WebBrowser.Back")));
	toolbar->addItem(new ui::custom::ToolBarButton(L"Forward", 2, ui::Command(L"Editor.WebBrowser.Forward")));
	toolbar->addItem(new ui::custom::ToolBarButton(L"Reload", 3, ui::Command(L"Editor.WebBrowser.Reload")));
	toolbar->addEventHandler< ui::custom::ToolBarButtonClickEvent >(this, &WebBrowserPage::eventToolClick);

	m_browser = new ui::WebBrowser();
	if (!m_browser->create(this, url))
		return false;

	return true;
}

void WebBrowserPage::eventToolClick(ui::custom::ToolBarButtonClickEvent* event)
{
	if (event->getCommand() == L"Editor.WebBrowser.Home")
	{
		std::wstring url = m_editor->getSettings()->getProperty< PropertyString >(L"Editor.HomeUrl", L"about:blank");
		m_browser->navigate(url);
	}
	else if (event->getCommand() == L"Editor.WebBrowser.Back")
		m_browser->back();
	else if (event->getCommand() == L"Editor.WebBrowser.Forward")
		m_browser->forward();
	else if (event->getCommand() == L"Editor.WebBrowser.Reload")
		m_browser->reload(false);
}

	}
}
