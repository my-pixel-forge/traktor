#pragma once

#include "Core/Object.h"
#include "Core/Ref.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor::ui
{

class Widget;

}

namespace traktor::editor
{

class PropertiesView;

/*!
 * \ingroup Editor
 */
class T_DLLCLASS IEditorPageSite : public Object
{
	T_RTTI_CLASS;

public:
	/*! Create properties view widget. */
	virtual Ref< PropertiesView > createPropertiesView(ui::Widget* parent) = 0;

	/*! Create additional docking panel. */
	virtual void createAdditionalPanel(ui::Widget* widget, int size, bool south) = 0;

	/*! Destroy additional docking panel. */
	virtual void destroyAdditionalPanel(ui::Widget* widget) = 0;

	/*! Show additional docking panel. */
	virtual void showAdditionalPanel(ui::Widget* widget) = 0;

	/*! Hide additional docking panel. */
	virtual void hideAdditionalPanel(ui::Widget* widget) = 0;
};

}
