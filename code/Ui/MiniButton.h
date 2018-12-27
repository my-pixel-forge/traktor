/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_ui_MiniButton_H
#define traktor_ui_MiniButton_H

#include "Ui/Widget.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_UI_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace ui
	{

/*! \brief Mini button control.
 * \ingroup UI
 */
class T_DLLCLASS MiniButton : public Widget
{
	T_RTTI_CLASS;
	
public:
	bool create(Widget* parent, const std::wstring& text);

	bool create(Widget* parent, IBitmap* image);

	void setImage(IBitmap* image);

	virtual Size getPreferedSize() const override;

private:
	bool m_pushed;
	Ref< IBitmap > m_image;
	
	void eventButtonDown(MouseButtonDownEvent* event);
	
	void eventButtonUp(MouseButtonUpEvent* event);
	
	void eventPaint(PaintEvent* event);
};

	}
}

#endif	// traktor_ui_MiniButton_H
