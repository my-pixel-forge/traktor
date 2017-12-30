/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#include "Ui/Application.h"
#include "Ui/Clipboard.h"
#include "Ui/Command.h"
#include "Ui/Edit.h"
#include "Ui/StyleBitmap.h"
#include "Ui/Custom/MiniButton.h"
#include "Ui/Custom/PropertyList/TextPropertyItem.h"
#include "Ui/Custom/PropertyList/PropertyList.h"
#include "Ui/Events/ButtonClickEvent.h"
#include "Ui/Events/FocusEvent.h"

// Resources
#include "Resources/SmallPen.h"

namespace traktor
{
	namespace ui
	{
		namespace custom
		{

T_IMPLEMENT_RTTI_CLASS(L"traktor.ui.custom.TextPropertyItem", TextPropertyItem, PropertyItem)

TextPropertyItem::TextPropertyItem(const std::wstring& text, const std::wstring& value, bool multiLine)
:	PropertyItem(text)
,	m_value(value)
,	m_multiLine(multiLine)
{
}

void TextPropertyItem::setValue(const std::wstring& value)
{
	m_value = value;
}

const std::wstring& TextPropertyItem::getValue() const
{
	return m_value;
}

void TextPropertyItem::createInPlaceControls(Widget* parent)
{
	if (!m_multiLine)
	{
		T_ASSERT (!m_editor);
		m_editor = new Edit();
		m_editor->create(
			parent,
			L"",
			WsWantAllInput
		);
		m_editor->setVisible(false);
		m_editor->addEventHandler< FocusEvent >(this, &TextPropertyItem::eventEditFocus);
		m_editor->addEventHandler< KeyDownEvent >(this, &TextPropertyItem::eventEditKeyDownEvent);
	}
	else
	{
		T_ASSERT (!m_buttonEdit);
		m_buttonEdit = new MiniButton();
		m_buttonEdit->create(parent, new ui::StyleBitmap(L"UI.SmallPen", c_ResourceSmallPen, sizeof(c_ResourceSmallPen)));
		m_buttonEdit->addEventHandler< ButtonClickEvent >(this, &TextPropertyItem::eventClick);
	}
}

void TextPropertyItem::destroyInPlaceControls()
{
	if (m_editor)
	{
		m_editor->destroy();
		m_editor = 0;
	}
	if (m_buttonEdit)
	{
		m_buttonEdit->destroy();
		m_buttonEdit = 0;
	}
}

void TextPropertyItem::resizeInPlaceControls(const Rect& rc, std::vector< WidgetRect >& outChildRects)
{
	if (m_editor)
	{
		outChildRects.push_back(WidgetRect(
			m_editor,
			Rect(
				rc.left,
				rc.top,
				rc.right,
				rc.bottom
			)
		));
	}
	if (m_buttonEdit)
	{
		outChildRects.push_back(WidgetRect(
			m_buttonEdit,
			Rect(
				rc.right - rc.getHeight(),
				rc.top,
				rc.right,
				rc.bottom
			)
		));
	}
}

void TextPropertyItem::mouseButtonDown(MouseButtonDownEvent* event)
{
	if (m_editor)
	{
		m_editor->setText(m_value);
		m_editor->setVisible(true);
		m_editor->setFocus();
		m_editor->selectAll();
	}
}

void TextPropertyItem::paintValue(Canvas& canvas, const Rect& rc)
{
	if (!m_multiLine)
		canvas.drawText(rc.inflate(-2, 0), m_value, AnLeft, AnCenter);
	else
		canvas.drawText(rc.inflate(-2, 0), m_value.empty() ? L"" : L"...", AnLeft, AnCenter);
}

bool TextPropertyItem::copy()
{
	if (!m_editor->isVisible(false))
	{
		Clipboard* clipboard = Application::getInstance()->getClipboard();
		if (clipboard)
			return clipboard->setText(m_value);
		else
			return false;
	}
	else
		return false;
}

bool TextPropertyItem::paste()
{
	if (!m_editor->isVisible(false))
	{
		Clipboard* clipboard = Application::getInstance()->getClipboard();
		if (!clipboard)
			return false;

		m_value = clipboard->getText();
		return true;
	}
	else
		return false;
}

void TextPropertyItem::eventEditFocus(FocusEvent* event)
{
	if (event->lostFocus() && m_editor->isVisible(false))
	{
		m_value = m_editor->getText();
		m_editor->setVisible(false);
		notifyChange();
	}
}

void TextPropertyItem::eventEditKeyDownEvent(KeyDownEvent* event)
{
	if (event->getVirtualKey() == ui::VkReturn)
	{
		m_value = m_editor->getText();
		m_editor->setVisible(false);
		notifyChange();
	}
	else if (event->getVirtualKey() == ui::VkEscape)
		m_editor->setVisible(false);
}

void TextPropertyItem::eventClick(ButtonClickEvent* event)
{
	notifyCommand(Command(L"Property.Edit"));
}

		}
	}
}
