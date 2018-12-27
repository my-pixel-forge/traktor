/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_ui_NumericPropertyItem_H
#define traktor_ui_NumericPropertyItem_H

#include "Ui/Point.h"
#include "Ui/PropertyList/PropertyItem.h"

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

class Edit;

/*! \brief Numeric property item.
 * \ingroup UI
 */
class T_DLLCLASS NumericPropertyItem : public PropertyItem
{
	T_RTTI_CLASS;

public:
	enum Representation
	{
		RpNormal,
		RpDecibel,
		RpAngle
	};

	NumericPropertyItem(const std::wstring& text, double value, double limitMin, double limitMax, bool floatPoint, bool hex, Representation representation);

	void setValue(double value);

	double getValue() const;

	void setLimitMin(double limitMin);

	double getLimitMin() const;

	void setLimitMax(double limitMax);

	double getLimitMax() const;

	void setLimit(double limitMin, double limitMax);

protected:
	virtual void createInPlaceControls(Widget* parent) override;

	virtual void destroyInPlaceControls() override;

	virtual void resizeInPlaceControls(const Rect& rc, std::vector< WidgetRect >& outChildRects) override;

	virtual void mouseButtonDown(MouseButtonDownEvent* event) override;

	virtual void mouseButtonUp(MouseButtonUpEvent* event) override;

	virtual void mouseMove(MouseMoveEvent* event) override;

	virtual void paintValue(Canvas& canvas, const Rect& rc) override;

	virtual bool copy() override;

	virtual bool paste() override;

private:
	Ref< Edit > m_editor;
	double m_value;
	double m_limitMin;
	double m_limitMax;
	bool m_floatPoint;
	bool m_hex;
	Representation m_representation;
	bool m_mouseAdjust;
	Point m_mouseLastPosition;

	void eventEditFocus(FocusEvent* event);

	void eventEditKeyDownEvent(KeyDownEvent* event);
};

	}
}

#endif	// traktor_ui_NumericPropertyItem_H
