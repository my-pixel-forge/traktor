#include "Core/Platform.h"
#include "Core/Log/Log.h"
#include "Flash/Action/ActionContext.h"
#include "Flash/Action/ActionFunctionNative.h"
#include "Flash/Action/Avm1/Classes/AsObject.h"

namespace traktor
{
	namespace flash
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.flash.AsObject", AsObject, ActionClass)

AsObject::AsObject(ActionContext* context)
:	ActionClass(context, "Object")
{
	Ref< ActionObject > prototype = new ActionObject();

	prototype->setMember("addProperty", ActionValue(createNativeFunction(context, this, &AsObject::Object_addProperty)));
	prototype->setMember("hasOwnProperty", ActionValue(createNativeFunction(context, this, &AsObject::Object_hasOwnProperty)));
	prototype->setMember("isPropertyEnumerable", ActionValue(createNativeFunction(context, this, &AsObject::Object_isPropertyEnumerable)));
	prototype->setMember("isPrototypeOf", ActionValue(createNativeFunction(context, this, &AsObject::Object_isPrototypeOf)));
	prototype->setMember("registerClass", ActionValue(createNativeFunction(context, this, &AsObject::Object_registerClass)));
	prototype->setMember("toString", ActionValue(createNativeFunction(context, this, &AsObject::Object_toString)));
	prototype->setMember("unwatch", ActionValue(createNativeFunction(context, this, &AsObject::Object_unwatch)));
	prototype->setMember("valueOf", ActionValue(createNativeFunction(context, this, &AsObject::Object_valueOf)));
	prototype->setMember("watch", ActionValue(createNativeFunction(context, this, &AsObject::Object_watch)));

	prototype->setMember("constructor", ActionValue(this));
	prototype->setReadOnly();

	setMember("prototype", ActionValue(prototype));
}

void AsObject::init(ActionObject* self, const ActionValueArray& args) const
{
}

void AsObject::coerce(ActionObject* self) const
{
	T_FATAL_ERROR;
}

void AsObject::Object_addProperty(ActionObject* self, const std::string& propertyName, ActionFunction* propertyGet, ActionFunction* propertySet) const
{
	self->addProperty(propertyName, propertyGet, propertySet);
}

bool AsObject::Object_hasOwnProperty(const ActionObject* self, const std::string& propertyName) const
{
	return self->hasOwnProperty(propertyName);
}

bool AsObject::Object_isPropertyEnumerable(const ActionObject* self) const
{
	return true;
}

bool AsObject::Object_isPrototypeOf(const ActionObject* self) const
{
	return false;
}

void AsObject::Object_registerClass(CallArgs& ca)
{
	Ref< ActionObject > global = ca.context->getGlobal();
	std::string movieClipName = ca.args[0].getString();
	
	if (ca.args.size() >= 2)
	{
		ActionValue theClass = ca.args[1];
		if (theClass.isObject())
		{
			global->setMember(movieClipName, theClass);
			ca.ret = ActionValue(true);
		}
		else
			ca.ret = ActionValue(false);
	}
	else
		ca.ret = ActionValue(global->deleteMember(movieClipName));
}

void AsObject::Object_toString(CallArgs& ca)
{
	ca.ret = ActionValue(ca.self->toString());
}

void AsObject::Object_unwatch(ActionObject* self) const
{
}

void AsObject::Object_valueOf(CallArgs& ca)
{
	ca.ret = ActionValue(ca.self->valueOf());
}

void AsObject::Object_watch(ActionObject* self) const
{
}

	}
}
