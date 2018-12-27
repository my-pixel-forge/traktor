/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_render_TextureAssetEditor_H
#define traktor_render_TextureAssetEditor_H

#include "Editor/IObjectEditor.h"
#include "Ui/PropertyList/AutoPropertyList.h"

namespace traktor
{
	namespace db
	{

class Instance;

	}

	namespace editor
	{

class IEditor;

	}

	namespace ui
	{

class Image;
class PropertyCommandEvent;

	}

	namespace render
	{

class TextureAsset;

class TextureAssetEditor
:	public editor::IObjectEditor
,	public ui::PropertyList::IPropertyGuidResolver
{
	T_RTTI_CLASS;

public:
	TextureAssetEditor(editor::IEditor* editor);

	virtual bool create(ui::Widget* parent, db::Instance* instance, ISerializable* object) override final;

	virtual void destroy() override final;

	virtual void apply() override final;

	virtual bool handleCommand(const ui::Command& command) override final;

	virtual void handleDatabaseEvent(db::Database* database, const Guid& eventId) override final;

	virtual ui::Size getPreferredSize() const override final;

private:
	editor::IEditor* m_editor;
	Ref< db::Instance > m_instance;
	Ref< TextureAsset > m_asset;
	Ref< ui::Image > m_imageTextureWithAlpha;
	Ref< ui::Image > m_imageTextureNoAlpha;
	Ref< ui::Image > m_imageTextureAlphaOnly;
	Ref< ui::AutoPropertyList > m_propertyList;

	void updatePreview();

	void eventPropertyCommand(ui::PropertyCommandEvent* event);

	virtual bool resolvePropertyGuid(const Guid& guid, std::wstring& resolved) const override final;
};

	}
}

#endif	// traktor_render_TextureAssetEditor_H
