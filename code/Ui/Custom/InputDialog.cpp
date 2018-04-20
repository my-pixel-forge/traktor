/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#include "Ui/Application.h"
#include "Ui/DropDown.h"
#include "Ui/Edit.h"
#include "Ui/Static.h"
#include "Ui/TableLayout.h"
#include "Ui/Custom/InputDialog.h"
#include "Ui/Custom/MiniButton.h"

namespace traktor
{
	namespace ui
	{
		namespace custom
		{

T_IMPLEMENT_RTTI_CLASS(L"traktor.ui.custom.InputDialog", InputDialog, ConfigDialog)

InputDialog::InputDialog()
:	m_outFields(0)
{
}

bool InputDialog::create(
	Widget* parent,
	const std::wstring& title,
	const std::wstring& message,
	Field* outFields,
	uint32_t outFieldsCount
)
{
	if (!ConfigDialog::create(parent, title, dpi96(300), dpi96(180), ConfigDialog::WsDefaultFixed, new TableLayout(L"100%", L"*,*", 4, 4)))
		return false;

	Ref< Static > labelMessage = new Static();
	labelMessage->create(this, message);

	Ref< Container > container = new Container();
	container->create(this, WsNone, new TableLayout(L"*,100%", L"*", 0, 4));

	m_outFields = outFields;
	for (uint32_t i = 0; i < outFieldsCount; ++i)
	{
		Ref< Static > labelEdit = new Static();
		labelEdit->create(container, m_outFields[i].title);

		if (!m_outFields[i].values)
		{
			if (!m_outFields[i].browseFile)
			{
				Ref< Edit > edit = new Edit();
				edit->create(container, m_outFields[i].value, WsClientBorder | WsTabStop, m_outFields[i].validator);
				m_editFields.push_back(edit);
			}
			else
			{
				Ref< Container > fieldContainer = new Container();
				fieldContainer->create(container, WsNone, new TableLayout(L"100%,*", L"*", 0, 4));

				Ref< Edit > edit = new Edit();
				edit->create(fieldContainer, m_outFields[i].value, WsClientBorder | WsTabStop, m_outFields[i].validator);
				m_editFields.push_back(edit);

				Ref< MiniButton > browse = new MiniButton();
				browse->create(fieldContainer, L"...");
			}
		}
		else
		{
			Ref< DropDown > dropDown = new DropDown();
			dropDown->create(container, m_outFields[i].value, WsClientBorder | WsTabStop);
			for (const wchar_t** it = m_outFields[i].values; *it; it += 2)
			{
				T_ASSERT (*it);
				dropDown->add(*it);
			}
			dropDown->select(0);
			m_editFields.push_back(dropDown);
		}
	}

	fit();

	return true;
}

int InputDialog::showModal()
{
	if (m_editFields.empty())
		return DrCancel;

	Ref< Edit > edit = dynamic_type_cast< Edit* >(m_editFields.front());
	if (edit)
	{
		edit->setFocus();
		edit->selectAll();
	}

	int result = ConfigDialog::showModal();
	if (result == DrOk && m_outFields)
	{
		for (uint32_t i = 0; i < uint32_t(m_editFields.size()); ++i)
		{
			Ref< Edit > editField = dynamic_type_cast< Edit* >(m_editFields[i]);
			if (edit)
				m_outFields[i].value = editField->getText();

			Ref< DropDown > dropDown = dynamic_type_cast< DropDown* >(m_editFields[i]);
			if (dropDown)
			{
				int32_t index = dropDown->getSelected();
				if (index >= 0)
				{
					for (const wchar_t** it = m_outFields[i].values; *it; it += 2)
					{
						T_ASSERT (*it);
						if (index-- <= 0)
						{
							m_outFields[i].value = *(it + 1);
							break;
						}
					}
				}
			}
		}
	}
	
	return result;
}

		}
	}
}
