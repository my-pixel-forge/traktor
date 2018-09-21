/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_ui_custom_ColorControl_H
#define traktor_ui_custom_ColorControl_H

#include "Ui/Widget.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_UI_CUSTOM_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace drawing
	{

class Image;

	}

	namespace ui
	{

class Bitmap;

		namespace custom
		{

/*! \brief Color static control.
 * \ingroup UIC
 */
class T_DLLCLASS ColorControl : public Widget
{
	T_RTTI_CLASS;

public:
	bool create(Widget* parent, int style);

	void setColor(const Color4ub& color);

	Color4ub getColor() const;

	virtual Size getPreferedSize() const T_OVERRIDE;

private:
	Color4ub m_color;
	Ref< drawing::Image > m_previewImage;
	Ref< Bitmap > m_previewBitmap;

	void eventPaint(PaintEvent* event);
};

		}
	}
}

#endif	// traktor_ui_custom_ColorControl_H
