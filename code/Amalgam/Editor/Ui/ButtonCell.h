/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_amalgam_ButtonCell_H
#define traktor_amalgam_ButtonCell_H

#include "Ui/Command.h"
#include "Ui/IBitmap.h"
#include "Ui/Auto/AutoWidgetCell.h"

namespace traktor
{
	namespace amalgam
	{

/*! \brief
 * \ingroup Amalgam
 */
class ButtonCell : public ui::AutoWidgetCell
{
	T_RTTI_CLASS;

public:
	ButtonCell(
		ui::IBitmap* bitmap,
		const ui::Command& command
	);

	void setEnable(bool enable);

	virtual void mouseDown(ui::MouseButtonDownEvent* event, const ui::Point& position)  override final;

	virtual void mouseUp(ui::MouseButtonUpEvent* event, const ui::Point& position)  override final;

	virtual void paint(ui::Canvas& canvas, const ui::Rect& rect)  override final;

private:
	Ref< ui::IBitmap > m_bitmap;
	ui::Command m_command;
	bool m_enable;
	bool m_down;
};

	}
}

#endif	// traktor_amalgam_ButtonCell_H
