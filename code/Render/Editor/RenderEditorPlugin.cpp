#include "Core/Misc/SafeDestroy.h"
#include "Editor/IEditor.h"
#include "Editor/Settings.h"
#include "Render/IRenderSystem.h"
#include "Render/Editor/RenderEditorPlugin.h"
#include "Ui/MessageBox.h"

namespace traktor
{
	namespace render
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.render.RenderEditorPlugin", RenderEditorPlugin, editor::IEditorPlugin)

RenderEditorPlugin::RenderEditorPlugin(editor::IEditor* editor)
:	m_editor(editor)
{
}

bool RenderEditorPlugin::create(ui::Widget* parent, editor::IEditorPageSite* site)
{
	std::wstring renderSystemTypeName = m_editor->getSettings()->getProperty< editor::PropertyString >(L"Editor.RenderSystem");

	const TypeInfo* renderSystemType = TypeInfo::find(renderSystemTypeName);
	if (!renderSystemType)
	{
		ui::MessageBox::show(parent, std::wstring(L"Unable to instantiate render system \"") + renderSystemTypeName + std::wstring(L"\"\nNo such type"), L"Error", ui::MbIconError | ui::MbOk);
		return false;
	}

	Ref< IRenderSystem > renderSystem = dynamic_type_cast< IRenderSystem* >(renderSystemType->createInstance());
	T_ASSERT (renderSystem);

	RenderSystemCreateDesc desc;
	desc.mipBias = m_editor->getSettings()->getProperty< editor::PropertyFloat >(L"Editor.MipBias");

	if (!renderSystem->create(desc))
	{
		ui::MessageBox::show(parent, std::wstring(L"Unable to create render system \"") + renderSystemTypeName + std::wstring(L"\""), L"Error", ui::MbIconError | ui::MbOk);
		return false;
	}

	m_editor->setStoreObject(L"RenderSystem", renderSystem);
	return true;
}

void RenderEditorPlugin::destroy()
{
	Ref< IRenderSystem > renderSystem = m_editor->getStoreObject< IRenderSystem >(L"RenderSystem");
	if (renderSystem)
	{
		safeDestroy(renderSystem);
		m_editor->setStoreObject(L"RenderSystem", 0);
	}
}

bool RenderEditorPlugin::handleCommand(const ui::Command& command)
{
	return false;
}

void RenderEditorPlugin::handleDatabaseEvent(const Guid& eventId)
{
}

	}
}
