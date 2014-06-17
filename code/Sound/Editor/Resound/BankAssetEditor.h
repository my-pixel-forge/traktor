#ifndef traktor_sound_BankAssetEditor_H
#define traktor_sound_BankAssetEditor_H

#include <map>
#include "Core/RefArray.h"
#include "Editor/IObjectEditor.h"

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

class Panel;
class ToolBar;
class ToolBarButton;

		}
	}

	namespace sound
	{

class BankAsset;
class BankBuffer;
class BankControl;
class BankControlGrain;
class IGrain;
class IGrainData;
class IGrainFacade;
class GrainProperties;
class SoundChannel;
class SoundSystem;

class BankAssetEditor : public editor::IObjectEditor
{
	T_RTTI_CLASS;

public:
	BankAssetEditor(editor::IEditor* editor);

	virtual bool create(ui::Widget* parent, db::Instance* instance, ISerializable* object);

	virtual void destroy();

	virtual void apply();

	virtual bool handleCommand(const ui::Command& command);

	virtual void handleDatabaseEvent(db::Database* database, const Guid& eventId);

	virtual ui::Size getPreferredSize() const;

private:
	editor::IEditor* m_editor;
	Ref< db::Instance > m_instance;
	Ref< BankAsset > m_asset;
	Ref< ui::custom::ToolBar > m_toolBar;
	Ref< ui::custom::ToolBarButton > m_toolBarItemPlay;
	Ref< ui::custom::ToolBarButton > m_toolBarItemRepeat;
	Ref< BankControl > m_bankControl;
	Ref< ui::custom::Panel > m_containerParameters;
	RefArray< ui::Slider > m_sliderParameters;
	Ref< ui::custom::Panel > m_containerGrainProperties;
	Ref< GrainProperties > m_grainProperties;
	Ref< ui::custom::Panel > m_containerGrainView;
	Ref< ui::PopupMenu > m_menuGrains;
	std::map< const TypeInfo*, Ref< IGrainFacade > > m_grainFacades;
	Ref< ui::Widget > m_currentGrainView;
	Ref< resource::IResourceManager > m_resourceManager;
	Ref< SoundSystem > m_soundSystem;
	Ref< SoundChannel > m_soundChannel;
	Ref< BankBuffer > m_bankBuffer;
	std::map< const IGrainData*, const IGrain* > m_grainInstances;

	void updateBankControl(BankControlGrain* parent, const RefArray< IGrainData >& grains);

	void updateBankControl();

	void updateProperties();

	void eventParameterChange(ui::Event* event);

	void eventToolBarClick(ui::Event* event);

	void eventGrainSelect(ui::Event* event);

	void eventGrainButtonUp(ui::Event* event);

	void eventGrainPropertiesChange(ui::Event* event);

	void eventGrainViewChange(ui::Event* event);

	void eventTimer(ui::Event* event);
};

	}
}

#endif	// traktor_sound_BankAssetEditor_H
