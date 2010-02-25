#ifndef traktor_sound_BankAssetEditor_H
#define traktor_sound_BankAssetEditor_H

#include <map>
#include "Core/RefArray.h"
#include "Editor/IObjectEditor.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_SOUND_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace editor
	{

class IEditor;

	}

	namespace resource
	{

class IResourceManager;

	}

	namespace ui
	{

class Command;
class Container;
class Event;
class PopupMenu;
class Slider;

		namespace custom
		{

class ToolBar;
class ToolBarButton;

		}
	}

	namespace sound
	{

class BankAsset;
class BankBuffer;
class IGrain;
class IGrainFacade;
class GrainProperties;
class GrainView;
class GrainViewItem;
class SoundChannel;
class SoundSystem;

class T_DLLCLASS BankAssetEditor : public editor::IObjectEditor
{
	T_RTTI_CLASS;

public:
	BankAssetEditor(editor::IEditor* editor);

	virtual bool create(ui::Widget* parent, db::Instance* instance, ISerializable* object);

	virtual void destroy();

	virtual void apply();

private:
	editor::IEditor* m_editor;
	Ref< db::Instance > m_instance;
	Ref< BankAsset > m_asset;
	Ref< ui::custom::ToolBar > m_toolBar;
	Ref< ui::custom::ToolBarButton > m_toolBarItemPlay;
	Ref< GrainView > m_grainView;
	Ref< ui::Container > m_containerGrainProperties;
	Ref< GrainProperties > m_grainProperties;
	Ref< ui::PopupMenu > m_menuGrains;
	std::map< const TypeInfo*, Ref< IGrainFacade > > m_grainFacades;
	Ref< resource::IResourceManager > m_resourceManager;
	Ref< SoundSystem > m_soundSystem;
	Ref< SoundChannel > m_soundChannel;
	Ref< BankBuffer > m_bankBuffer;

	void updateGrainView(GrainViewItem* parent, const RefArray< IGrain >& grains);

	void updateGrainView();

	void handleCommand(const ui::Command& command);

	void eventToolBarClick(ui::Event* event);

	void eventGrainSelect(ui::Event* event);

	void eventGrainButtonUp(ui::Event* event);

	void eventGrainPropertiesChange(ui::Event* event);

	void eventTimer(ui::Event* event);
};

	}
}

#endif	// traktor_sound_BankAssetEditor_H
