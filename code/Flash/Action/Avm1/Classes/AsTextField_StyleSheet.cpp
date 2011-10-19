#include "Flash/Action/ActionFunctionNative.h"
#include "Flash/Action/Avm1/Classes/AsTextField_StyleSheet.h"

namespace traktor
{
	namespace flash
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.flash.AsTextField_StyleSheet", AsTextField_StyleSheet, ActionClass)

AsTextField_StyleSheet::AsTextField_StyleSheet(ActionContext* context)
:	ActionClass(context, "TextField.StyleSheet")
{
	Ref< ActionObject > prototype = new ActionObject();

	prototype->setMember("clear", ActionValue(createNativeFunction(context, this, &AsTextField_StyleSheet::TextField_StyleSheet_clear)));
	prototype->setMember("getStyle", ActionValue(createNativeFunction(context, this, &AsTextField_StyleSheet::TextField_StyleSheet_getStyle)));
	prototype->setMember("getStyleNames", ActionValue(createNativeFunction(context, this, &AsTextField_StyleSheet::TextField_StyleSheet_getStyleNames)));
	prototype->setMember("load", ActionValue(createNativeFunction(context, this, &AsTextField_StyleSheet::TextField_StyleSheet_load)));
	prototype->setMember("parseCSS", ActionValue(createNativeFunction(context, this, &AsTextField_StyleSheet::TextField_StyleSheet_parseCSS)));
	prototype->setMember("setStyle", ActionValue(createNativeFunction(context, this, &AsTextField_StyleSheet::TextField_StyleSheet_setStyle)));
	prototype->setMember("transform", ActionValue(createNativeFunction(context, this, &AsTextField_StyleSheet::TextField_StyleSheet_transform)));

	prototype->setMember("constructor", ActionValue(this));
	prototype->setReadOnly();

	setMember("prototype", ActionValue(prototype));
}

void AsTextField_StyleSheet::init(ActionObject* self, const ActionValueArray& args) const
{
}

void AsTextField_StyleSheet::coerce(ActionObject* self) const
{
	T_FATAL_ERROR;
}

void AsTextField_StyleSheet::TextField_StyleSheet_clear(CallArgs& ca)
{
}

void AsTextField_StyleSheet::TextField_StyleSheet_getStyle(CallArgs& ca)
{
}

void AsTextField_StyleSheet::TextField_StyleSheet_getStyleNames(CallArgs& ca)
{
}

void AsTextField_StyleSheet::TextField_StyleSheet_load(CallArgs& ca)
{
}

void AsTextField_StyleSheet::TextField_StyleSheet_parseCSS(CallArgs& ca)
{
}

void AsTextField_StyleSheet::TextField_StyleSheet_setStyle(CallArgs& ca)
{
}

void AsTextField_StyleSheet::TextField_StyleSheet_transform(CallArgs& ca)
{
}

	}
}
