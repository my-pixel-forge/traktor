#include "Core/Log/Log.h"
#include "Core/Misc/Endian.h"
#include "Flash/FlashMovie.h"
#include "Flash/FlashSprite.h"
#include "Flash/FlashSpriteInstance.h"
#include "Flash/Action/ActionContext.h"
#include "Flash/Action/ActionDictionary.h"
#include "Flash/Action/ActionFrame.h"
#include "Flash/Action/ActionFunction.h"
#include "Flash/Action/Avm1/ActionFunction1.h"
#include "Flash/Action/Avm1/ActionFunction2.h"
#include "Flash/Action/Avm1/ActionOperations.h"
#include "Flash/Action/Avm1/ActionVMImage1.h"
#include "Flash/Action/Avm1/Classes/AsObject.h"
#include "Flash/Action/Avm1/Classes/AsArray.h"
#include "Flash/Action/Avm1/Classes/AsFunction.h"
#include "Flash/Action/Avm1/Classes/AsMovieClip.h"

#define VM_TRACE_ENABLE 0

#if VM_TRACE_ENABLE
#	define T_WIDEN_X(x) L ## x
#	define T_WIDEN(x) T_WIDEN_X(x)
#	define VM_BEGIN(op) \
	case op : \
		{ \
			log::debug << T_WIDEN( #op ) << L" (stack " << stack.depth() << L")" << Endl << IncreaseIndent;
#	define VM_END() \
			log::debug << DecreaseIndent; \
		} \
		break;
#	define VM_NOT_IMPLEMENTED \
		log::error << L"Opcode not implemented" << Endl; \
		T_BREAKPOINT;
#	define VM_LOG(x) \
		log::debug << x << Endl;
#else
#	define VM_BEGIN(op) \
	case op : \
		{
#	define VM_END() \
		} \
		break;
#	define VM_NOT_IMPLEMENTED
#	define VM_LOG(x)
#endif

namespace traktor
{
	namespace flash
	{
		namespace
		{

void opx_nextFrame(ExecutionState& state)
{
	FlashSpriteInstance* movieClip = state.frame->getContext()->getMovieClip();
	movieClip->gotoNext();
}

void opx_prevFrame(ExecutionState& state)
{
	FlashSpriteInstance* movieClip = state.frame->getContext()->getMovieClip();
	movieClip->gotoPrevious();
}

void opx_play(ExecutionState& state)
{
	FlashSpriteInstance* movieClip = state.frame->getContext()->getMovieClip();
	movieClip->setPlaying(true);
}

void opx_stop(ExecutionState& state)
{
	FlashSpriteInstance* movieClip = state.frame->getContext()->getMovieClip();
	movieClip->setPlaying(false);
}

void opx_toggleQuality(ExecutionState& state)
{
	VM_NOT_IMPLEMENTED;
}

void opx_stopSounds(ExecutionState& state)
{
	VM_NOT_IMPLEMENTED;
}

void opx_add(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();

	ActionValue& number2 = stack.top(0);
	ActionValue& number1 = stack.top(-1);

	if (number1.isNumeric() && number2.isNumeric())
		number1 = ActionValue(number1.getNumber() + number2.getNumber());
	else
		number1 = ActionValue();

	stack.drop(1);
}

void opx_subtract(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();

	ActionValue& number2 = stack.top(0);
	ActionValue& number1 = stack.top(-1);

	if (number1.isNumeric() && number2.isNumeric())
		number1 = ActionValue(number1.getNumber() - number2.getNumber());
	else
		number1 = ActionValue();

	stack.drop(1);
}

void opx_multiply(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();

	ActionValue& number2 = stack.top(0);
	ActionValue& number1 = stack.top(-1);

	if (number1.isNumeric() && number2.isNumeric())
		number1 = ActionValue(number1.getNumber() * number2.getNumber());
	else
		number1 = ActionValue();

	stack.drop(1);
}

void opx_divide(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();

	ActionValue& number2 = stack.top(0);
	ActionValue& number1 = stack.top(-1);

	if (number1.isNumeric() && number2.isNumeric())
		number1 = ActionValue(number1.getNumber() / number2.getNumber());
	else
		number1 = ActionValue();

	stack.drop(1);
}

void opx_equal(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();

	ActionValue& number2 = stack.top(0);
	ActionValue& number1 = stack.top(-1);

	if (number1.isNumeric() && number2.isNumeric())
		number1 = ActionValue(bool(number1.getNumber() == number2.getNumber()));
	else
		number1 = ActionValue();

	stack.drop(1);
}

void opx_lessThan(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();

	ActionValue& number2 = stack.top(0);
	ActionValue& number1 = stack.top(-1);

	if (number1.isNumeric() && number2.isNumeric())
		number1 = ActionValue(bool(number1.getNumber() < number2.getNumber()));
	else
		number1 = ActionValue();

	stack.drop(1);
}

void opx_logicalAnd(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue number2 = stack.pop().toBoolean();
	ActionValue number1 = stack.pop().toBoolean();
	stack.push(ActionValue(bool(number1.getBoolean() && number2.getBoolean())));
}

void opx_logicalOr(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue number2 = stack.pop().toBoolean();
	ActionValue number1 = stack.pop().toBoolean();
	stack.push(ActionValue(bool(number1.getBoolean() || number2.getBoolean())));
}

void opx_logicalNot(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue value = stack.pop().toBoolean();
	stack.push(ActionValue(bool(!value.getBoolean())));
}

void opx_stringEqual(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue str2 = stack.pop();
	ActionValue str1 = stack.pop();
	stack.push(ActionValue(bool(str1.getStringSafe().compare(str2.getStringSafe()) == 0)));
}

void opx_stringLength(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue str = stack.pop();
	stack.push(ActionValue(avm_number_t(str.getStringSafe().length())));
}

void opx_stringExtract(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue count = stack.pop();
	ActionValue index = stack.pop();
	ActionValue strng = stack.pop();
	if (count.isNumeric() && index.isNumeric())
	{
		std::string str = strng.getStringSafe();
		int32_t offset = int32_t(index.getNumber());
		if (offset >= 0 && offset < int32_t(str.length()))
		{
			std::string res = str.substr(offset, int(count.getNumber()));
			stack.push(ActionValue(res));
		}
		else
			stack.push(ActionValue(""));
	}
	else
		stack.push(ActionValue(""));
}

void opx_pop(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	stack.pop();
}

void opx_int(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue& number = stack.top();
	if (number.isNumeric())
		number = ActionValue(avm_number_t(std::floor(number.getNumber())));
	else
		number = ActionValue();
}

void opx_getVariable(ExecutionState& state)
{
	ActionContext* context = state.frame->getContext();
	ActionValueStack& stack = state.frame->getStack();
	FlashSpriteInstance* movieClip = state.frame->getContext()->getMovieClip();
	ActionObject* self = state.frame->getSelf();

	std::string variableName = stack.pop().getStringSafe();
	ActionValue value;

	if (state.frame->getVariable(variableName, value))
		stack.push(value);
	else if (self->getMember(context, variableName, value))
		stack.push(value);
	else if (movieClip != 0 && self != movieClip && movieClip->getMember(context, variableName, value))
		stack.push(value);
	else if (context->getGlobal()->getLocalMember(variableName, value))
		stack.push(value);
	else
		stack.push(value);
}

void opx_setVariable(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	FlashSpriteInstance* movieClip = state.frame->getContext()->getMovieClip();

	ActionValue value = stack.pop();
	std::string variableName = stack.pop().getStringSafe();

	ActionValue* variableValue = state.frame->getVariableValue(variableName);
	if (variableValue)
		*variableValue = value;
	else
		movieClip->setMember(variableName, value);
}

void opx_setTargetExpr(ExecutionState& state)
{
	VM_NOT_IMPLEMENTED;
}

void opx_stringConcat(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue str2 = stack.pop();
	ActionValue str1 = stack.pop();
	stack.push(ActionValue(str1.getStringSafe() + str2.getStringSafe()));
}

void opx_getProperty(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	FlashSpriteInstance* movieClip = state.frame->getContext()->getMovieClip();

	ActionValue index = stack.pop();
	ActionValue target = stack.pop();
	T_ASSERT (target.getStringSafe() == "");

	switch (int(index.getNumberSafe()))
	{
	case 0:
		stack.push(ActionValue(movieClip->getTransform().e31 / 20.0f));
		break;
	case 1:
		stack.push(ActionValue(movieClip->getTransform().e32 / 20.0f));
		break;
	case 2:
		stack.push(ActionValue(movieClip->getTransform().e11 * 100.0f));
		break;
	case 3:
		stack.push(ActionValue(movieClip->getTransform().e22 * 100.0f));
		break;
	case 4:
		stack.push(ActionValue(avm_number_t(movieClip->getCurrentFrame())));
		break;
	case 5:
		stack.push(ActionValue(avm_number_t(movieClip->getSprite()->getFrameCount())));
		break;
	case 6:
		stack.push(ActionValue(1.0f));
		break;
	case 7:
		stack.push(ActionValue(true));
		break;
	case 8:
		{
			SwfRect bounds = movieClip->getBounds();
			stack.push(ActionValue((bounds.max.x - bounds.min.x) / 20.0f));
		}
		break;
	case 9:
		{
			SwfRect bounds = movieClip->getBounds();
			stack.push(ActionValue((bounds.max.y - bounds.min.y) / 20.0f));
		}
		break;

	default:
		VM_LOG(L"GetProperty, invalid index");
	}
}

void opx_setProperty(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	FlashSpriteInstance* movieClip = state.frame->getContext()->getMovieClip();

	ActionValue value = stack.pop();
	ActionValue index = stack.pop();
	ActionValue target = stack.pop();
	T_ASSERT (target.getStringSafe() == "");

	switch (int(index.getNumberSafe()))
	{
	case 0:
		{
			Matrix33 transform = movieClip->getTransform();
			transform.e13 = float(value.getNumberSafe() * 20.0f);
			movieClip->setTransform(transform);
		}
		break;
	case 1:
		{
			Matrix33 transform = movieClip->getTransform();
			transform.e23 = float(value.getNumberSafe() * 20.0f);
			movieClip->setTransform(transform);
		}
		break;
	case 2:
		{
			Matrix33 transform = movieClip->getTransform();
			transform.e11 = float(value.getNumberSafe() * 100.0f);
			movieClip->setTransform(transform);
		}
		break;
	case 3:
		{
			Matrix33 transform = movieClip->getTransform();
			transform.e22 = float(value.getNumberSafe() * 100.0f);
			movieClip->setTransform(transform);
		}
		break;

	default:
		VM_LOG(L"SetProperty, invalid index");
	}
}

void opx_cloneSprite(ExecutionState& state)
{
	FlashSpriteInstance* movieClip = state.frame->getContext()->getMovieClip();
	ActionValueStack& stack = state.frame->getStack();

	ActionValue depth = stack.pop();
	ActionValue target = stack.pop();
	ActionValue source = stack.pop();

	Ref< FlashSpriteInstance > sourceClip = source.getObject< FlashSpriteInstance >();
	Ref< FlashSpriteInstance > cloneClip = sourceClip->clone();

	cloneClip->setName(target.getStringSafe());

	movieClip->getDisplayList().showObject(
		int32_t(depth.getNumber()),
		cloneClip->getSprite()->getId(),
		cloneClip,
		true
	);
}

void opx_removeSprite(ExecutionState& state)
{
	VM_NOT_IMPLEMENTED;
}

void opx_trace(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	std::wstring trace = stack.pop().getWideStringSafe();
	if (trace != L"** BREAK **")
		log::debug << L"TRACE \"" << trace << L"\"" << Endl;
	else
		T_BREAKPOINT;
}

void opx_startDragMovie(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue target = stack.pop();
	uint32_t lockCenter = uint32_t(stack.pop().getNumberSafe());
	uint32_t constraint = uint32_t(stack.pop().getNumberSafe());
	if (constraint)
	{
		ActionValue y2 = stack.pop();
		ActionValue x2 = stack.pop();
		ActionValue y1 = stack.pop();
		ActionValue x1 = stack.pop();
	}
	// @fixme
}

void opx_stopDragMovie(ExecutionState& state)
{
	VM_NOT_IMPLEMENTED;
}

void opx_stringCompare(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue str2 = stack.pop();
	ActionValue str1 = stack.pop();
	stack.push(ActionValue(avm_number_t(str1.getStringSafe().compare(str2.getStringSafe()))));
}

void opx_throw(ExecutionState& state)
{
	VM_NOT_IMPLEMENTED;
}

void opx_castOp(ExecutionState& state)
{
	ActionContext* context = state.frame->getContext();
	ActionValueStack& stack = state.frame->getStack();

	ActionValue objectValue = stack.pop();
	ActionValue constructorValue = stack.pop();

	if (objectValue.isObject() && constructorValue.isObject())
	{
		ActionObject* object = objectValue.getObject();
		ActionObject* constructor = constructorValue.getObject();

		if (object->getPrototype(context) == constructor)
		{
			stack.push(objectValue);
			return;
		}
	}

	stack.push(ActionValue((avm_number_t)0));
}

void opx_implementsOp(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue implementationClass = stack.pop();
	uint32_t interfaceCount = uint32_t(stack.pop().getNumberSafe());
	for (uint32_t i = 0; i < interfaceCount; ++i)
	{
		ActionValue interfaceClass = stack.pop();
		implementationClass.getObject()->addInterface(interfaceClass.getObject());
	}
}

void opx_random(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue& max = stack.top();
	if (max.isNumeric())
	{
		int32_t rnd = std::rand() % std::max< int32_t >(int32_t(max.getNumber()), 1);
		max = ActionValue(avm_number_t(rnd));
	}
	else
		max = ActionValue();
}

void opx_mbLength(ExecutionState& state)
{
	VM_NOT_IMPLEMENTED;
}

void opx_ord(ExecutionState& state)
{
	VM_NOT_IMPLEMENTED;
}

void opx_chr(ExecutionState& state)
{
	VM_NOT_IMPLEMENTED;
}

void opx_getTime(ExecutionState& state)
{
	//ActionValueStack& stack = state.frame->getStack();
	//avm_number_t sinceStartup = avm_number_t(m_timer.getElapsedTime() * 1000.0);
	//stack.push(ActionValue(sinceStartup));
	VM_NOT_IMPLEMENTED;
}

void opx_mbSubString(ExecutionState& state)
{
	VM_NOT_IMPLEMENTED;
}

void opx_mbOrd(ExecutionState& state)
{
	VM_NOT_IMPLEMENTED;
}

void opx_mbChr(ExecutionState& state)
{
	VM_NOT_IMPLEMENTED;
}

void opx_delete(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	std::string memberName = stack.pop().getStringSafe();
	ActionValue objectValue = stack.pop();
	if (objectValue.isObject())
	{
		bool deleted = objectValue.getObject()->deleteMember(memberName);
		stack.push(ActionValue(deleted));
	}
	else
		stack.push(ActionValue(false));
}

void opx_delete2(ExecutionState& state)
{
	FlashSpriteInstance* movieClip = state.frame->getContext()->getMovieClip();
	ActionValueStack& stack = state.frame->getStack();

	std::string variableName = stack.pop().getStringSafe();
	bool deleted = movieClip->deleteMember(variableName);
	stack.push(ActionValue(deleted));
}

void opx_defineLocal(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	FlashSpriteInstance* movieClip = state.frame->getContext()->getMovieClip();

	ActionValue variableValue = stack.pop();
	std::string variableName = stack.pop().getStringSafe();
	if (state.frame->getCallee())
		state.frame->setVariable(variableName, variableValue);
	else
		movieClip->setMember(variableName, variableValue);
}

void opx_callFunction(ExecutionState& state)
{
	ActionContext* context = state.frame->getContext();
	ActionValueStack& stack = state.frame->getStack();
	FlashSpriteInstance* movieClip = state.frame->getContext()->getMovieClip();

	std::string functionName = stack.pop().getStringSafe();
	ActionValue functionObject;

	if (!state.frame->getVariable(functionName, functionObject))
	{
		if (!state.frame->getSelf()->getMember(context, functionName, functionObject))
		{
			if (!movieClip->getMember(context, functionName, functionObject))
			{
				context->getGlobal()->getLocalMember(functionName, functionObject);
			}
		}
	}

	if (functionObject.isObject())
	{
		ActionFunction* function = checked_type_cast< ActionFunction* >(functionObject.getObject());
		stack.push(function->call(state.frame, state.frame->getSelf()));
	}
	else
	{
		log::warning << L"Undefined function \"" << mbstows(functionName) << L"\"" << Endl;
		int argCount = int(stack.pop().getNumber());
		stack.drop(argCount);
		stack.push(ActionValue());
	}
}

void opx_modulo(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue number2 = stack.pop();
	ActionValue number1 = stack.pop();
	if (number1.isNumeric() && number2.isNumeric())
	{
		int n2 = int(number2.getNumber());
		int n1 = int(number1.getNumber());
		stack.push(ActionValue(avm_number_t(n1 % n2)));
	}
	else
		stack.push(ActionValue());
}

void opx_new(ExecutionState& state)
{
	ActionContext* context = state.frame->getContext();
	ActionValueStack& stack = state.frame->getStack();

	ActionValue classFunction = stack.pop();
	Ref< ActionFunction > classConstructor;
	
	if (classFunction.isObject())
		classConstructor = classFunction.getObject< ActionFunction >();
	else
	{
		std::string classConstructorName = classFunction.getStringSafe();
		context->getGlobal()->getLocalMember(classConstructorName, classFunction);
		classConstructor = classFunction.getObjectSafe< ActionFunction >();
	}

	if (classConstructor)
	{
		ActionValue prototype;
		classConstructor->getLocalMember("prototype", prototype);

		// Create instance of class.
		Ref< ActionObject > self = new ActionObject(prototype.getObjectSafe());

		// Call constructor.
		ActionValue object = classConstructor->call(state.frame, self);
		if (object.isObject())
			stack.push(object);
		else
			stack.push(ActionValue(self.ptr()));
	}
	else
	{
		VM_LOG(L"Not a class object");
		stack.push(ActionValue());
	}
}

void opx_defineLocal2(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	FlashSpriteInstance* movieClip = state.frame->getContext()->getMovieClip();

	std::string variableName = stack.pop().getStringSafe();
	if (state.frame->getCallee())
		state.frame->setVariable(variableName, ActionValue());
	else
		movieClip->setMember(variableName, ActionValue());
}

void opx_initArray(ExecutionState& state)
{
	ActionContext* context = state.frame->getContext();
	ActionValueStack& stack = state.frame->getStack();

	ActionValue arrayClassMember;
	if (context->getGlobal()->getLocalMember("Array", arrayClassMember) && arrayClassMember.isObject())
	{
		ActionValue arrayObject = arrayClassMember.getObject< ActionFunction >()->call(state.frame, 0);
		stack.push(arrayObject);
	}
	else
		stack.push(ActionValue());
}

void opx_initObject(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();

	int32_t initialPropertyCount = int32_t(stack.pop().getNumberSafe());

	Ref< ActionObject > scriptObject = new ActionObject();
	for (int32_t i = 0; i < initialPropertyCount; ++i)
	{
		ActionValue value = stack.pop();
		ActionValue name = stack.pop();
		scriptObject->setMember(name.getStringSafe(), value);
		VM_LOG(L"Initial property " << i << L" \"" << name.getWideStringSafe() << L"\" = " << value.getWideStringSafe());
	}

	stack.push(ActionValue(scriptObject));
}

void opx_typeOf(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();

	ActionValue value = stack.pop();
	if (value.isNumeric())
	{
		stack.push(ActionValue("number"));
	}
	else if (value.isBoolean())
	{
		stack.push(ActionValue("boolean"));
	}
	else if (value.isString())
	{
		stack.push(ActionValue("string"));
	}
	else if (value.isObject())
	{
		Ref< ActionObject > object = value.getObject();
		if (is_a< AsMovieClip >(object))
			stack.push(ActionValue("movieclip"));
		else if (is_a< AsFunction >(object))
			stack.push(ActionValue("function"));
		else
			stack.push(ActionValue("object"));
	}
	else
		stack.push(ActionValue("undefined"));
}

void opx_targetPath(ExecutionState& state)
{
	VM_NOT_IMPLEMENTED;
}

void opx_enumerate(ExecutionState& state)
{
	VM_NOT_IMPLEMENTED;
}

void opx_newAdd(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue value2 = stack.pop();
	ActionValue value1 = stack.pop();
	if (value2.isString() || value1.isString())
	{
		ActionValue string2 = value2.toString();
		ActionValue string1 = value1.toString();
		if (string2.isString() && string1.isString())
		{
			std::string str2 = string2.getString();
			std::string str1 = string1.getString();
			stack.push(ActionValue(str1 + str2));
		}
		else
			stack.push(ActionValue());
	}
	else
	{
		ActionValue number2 = value2.toNumber();
		ActionValue number1 = value1.toNumber();
		if (number2.isNumeric() && number1.isNumeric())
		{
			avm_number_t n2 = number2.getNumber();
			avm_number_t n1 = number1.getNumber();
			stack.push(ActionValue(n1 + n2));
		}
		else
			stack.push(ActionValue());
	}
}

void opx_newLessThan(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue value2 = stack.pop();
	ActionValue value1 = stack.pop();
	if (value2.isString() || value1.isString())
	{
		ActionValue string2 = value2.toString();
		ActionValue string1 = value1.toString();
		if (string2.isString() && string1.isString())
		{
			std::string str2 = string2.getString();
			std::string str1 = string1.getString();
			stack.push(ActionValue(bool(str1.compare(str2) < 0)));
		}
		else
			stack.push(ActionValue());
	}
	else
	{
		ActionValue number2 = value2.toNumber();
		ActionValue number1 = value1.toNumber();
		if (number2.isNumeric() && number1.isNumeric())
		{
			avm_number_t n2 = number2.getNumber();
			avm_number_t n1 = number1.getNumber();
			stack.push(ActionValue(bool(n1 < n2)));
		}
		else
			stack.push(ActionValue());
	}
}

void opx_newEquals(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue value2 = stack.pop();
	ActionValue value1 = stack.pop();

	if (!value2.isUndefined() && !value1.isUndefined())
	{
		ActionValue::Type predicateType = max(value2.getType(), value1.getType());
		if (predicateType == ActionValue::AvtBoolean)
		{
			bool v2 = value2.getBooleanSafe();
			bool v1 = value1.getBooleanSafe();
			stack.push(ActionValue(v1 == v2));
		}
		else if (predicateType == ActionValue::AvtNumber)
		{
			avm_number_t v2 = value2.getNumberSafe();
			avm_number_t v1 = value1.getNumberSafe();
			stack.push(ActionValue(v1 == v2));
		}
		else if (predicateType == ActionValue::AvtString)
		{
			std::string v2 = value2.getStringSafe();
			std::string v1 = value1.getStringSafe();
			stack.push(ActionValue(v1 == v2));
		}
		else	// AvtObject
		{
			Ref< ActionObject > object2 = value2.getObjectSafe();
			Ref< ActionObject > object1 = value1.getObjectSafe();
			stack.push(ActionValue(object1 == object2));
		}
	}
	else if (value1.isObject() && value2.isUndefined())
	{
		ActionObject* object1 = value1.getObject();
		stack.push(ActionValue(object1 == 0));
	}
	else if (value1.isUndefined() && value2.isObject())
	{
		ActionObject* object2 = value2.getObject();
		stack.push(ActionValue(object2 == 0));
	}
	else if (value1.isUndefined() && value2.isUndefined())
		stack.push(ActionValue(true));
	else
		stack.push(ActionValue(false));
}

void opx_toNumber(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	stack.top() = stack.top().toNumber();
}

void opx_toString(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	stack.top() = stack.top().toString();
}

void opx_dup(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	stack.push(stack.top());
}

void opx_swap(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue v1 = stack.pop();
	ActionValue v2 = stack.pop();
	stack.push(v1);
	stack.push(v2);
}

void opx_getMember(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();

	ActionValue memberName = stack.pop();
	ActionValue& targetValue = stack.top();

	if (targetValue.isObject())
	{
		ActionObject* target = targetValue.getObject();
		if (target)
		{
			ActionContext* context = state.frame->getContext();

			if (target->getMember(context, memberName, targetValue))
				return;

			Ref< ActionFunction > propertyGet;
			if (memberName.isString() && target->getPropertyGet(context, memberName.getString(), propertyGet))
			{
				stack.push(ActionValue(avm_number_t(0)));
				targetValue = propertyGet->call(state.frame, target);
				return;
			}
		}
	}

	// Unable to get member value; ensure top of stack is undefined.
	targetValue = ActionValue();
}

void opx_setMember(ExecutionState& state)
{
	ActionContext* context = state.frame->getContext();
	ActionValueStack& stack = state.frame->getStack();

	ActionValue memberValue = stack.pop();
	ActionValue memberName = stack.pop();

	ActionValue targetValue = stack.pop();
	if (targetValue.isObject())
	{
		Ref< ActionObject > target = targetValue.getObject();
		if (target)
		{
			Ref< ActionFunction > propertySet;
			if (memberName.isString() && target->getPropertySet(context, memberName.getString(), propertySet))
			{
				stack.push(memberValue);
				stack.push(ActionValue(avm_number_t(1)));
				propertySet->call(state.frame, target);
			}
			else
				target->setMember(memberName, memberValue);
		}
	}

#if defined(_DEBUG)
	if (memberValue.isObject())
	{
		Ref< ActionFunction > memberFunction = memberValue.getObject< ActionFunction >();
		if (memberFunction)
			memberFunction->setName(memberName.getStringSafe());
	}
#endif
}

void opx_increment(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue& number = stack.top();
	if (number.isNumeric())
		number = ActionValue(number.getNumber() + 1);
	else
		number = ActionValue();
}

void opx_decrement(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue& number = stack.top();
	if (number.isNumeric())
		number = ActionValue(number.getNumber() - 1);
	else
		number = ActionValue();
}

void opx_callMethod(ExecutionState& state)
{
	ActionContext* context = state.frame->getContext();
	ActionValueStack& stack = state.frame->getStack();
	FlashSpriteInstance* movieClip = context->getMovieClip();

	ActionValue classConstructorName = stack.pop();
	ActionValue targetValue = stack.pop();
	ActionValue returnValue;

	Ref< ActionObject > target = targetValue.getObjectSafe();
	Ref< ActionFunction > method;

	if (!target)
		target = movieClip;

	if (target)
	{
		if (classConstructorName.isString())
		{
			ActionValue memberValue;
			if (target->getMember(context, classConstructorName.getString(), memberValue))
				method = dynamic_type_cast< ActionFunction* >(memberValue.getObjectSafe());
		}
		else
			method = dynamic_type_cast< ActionFunction* >(target);
	}

	if (method)
		returnValue = method->call(state.frame, target);
	else
	{
		log::warning << L"Undefined method \"" << classConstructorName.getWideStringSafe() << L"\"" << Endl;
		int argCount = int(stack.pop().getNumber());
		stack.drop(argCount);
	}

	stack.push(returnValue);
}

void opx_newMethod(ExecutionState& state)
{
	ActionContext* context = state.frame->getContext();
	ActionValueStack& stack = state.frame->getStack();

	std::string classConstructorName = stack.pop().getStringSafe();
	Ref< ActionObject > classConstructorObject = stack.pop().getObjectSafe();
	if (classConstructorObject)
	{
		Ref< ActionFunction > classConstructor;

		if (classConstructorName.empty())
			classConstructor = dynamic_type_cast< ActionFunction* >(classConstructorObject);
		else
		{
			ActionValue methodValue;
			classConstructorObject->getMember(context, classConstructorName, methodValue);
			classConstructor = methodValue.getObjectSafe< ActionFunction >();
		}

		if (classConstructor)
		{
			ActionValue prototype;
			classConstructor->getLocalMember("prototype", prototype);

			// Create instance of class.
			Ref< ActionObject > self = new ActionObject(prototype.getObjectSafe());

			// Call constructor.
			ActionValue object = classConstructor->call(state.frame, self);
			if (object.isObject())
				stack.push(object);
			else
				stack.push(ActionValue(self.ptr()));
		}
		else
		{
			VM_LOG(L"Not a class object");
			stack.push(ActionValue());
		}
	}
	else
	{
		VM_LOG(L"No such class");
		stack.push(ActionValue());
	}
}

void opx_instanceOf(ExecutionState& state)
{
	ActionContext* context = state.frame->getContext();
	ActionValueStack& stack = state.frame->getStack();

	ActionValue objectValue = stack.pop();
	ActionValue constructorValue = stack.pop();

	if (objectValue.isObject() && constructorValue.isObject())
	{
		ActionObject* object = objectValue.getObject();
		ActionObject* constructor = constructorValue.getObject();

		if (object->getPrototype(context) == constructor)
		{
			stack.push(ActionValue(true));
			return;
		}
	}

	stack.push(ActionValue(false));
}

void opx_enum2(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();

	ActionValue enumObject = stack.pop();
	if (enumObject.isObject())
	{
		ActionObject* object = enumObject.getObject();
		const ActionObject::property_map_t& properties = object->getProperties();
		for (ActionObject::property_map_t::const_iterator i = properties.begin(); i != properties.end(); ++i)
			stack.push(ActionValue(i->first));
	}
	stack.push(ActionValue(avm_number_t(0)));
}

void opx_bitwiseAnd(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue number2 = stack.pop();
	ActionValue number1 = stack.pop();
	if (number1.isNumeric() && number2.isNumeric())
	{
		int32_t n2 = int32_t(number2.getNumber());
		int32_t n1 = int32_t(number1.getNumber());
		stack.push(ActionValue(avm_number_t(n1 & n2)));
	}
	else
		stack.push(ActionValue());
}

void opx_bitwiseOr(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue& number2 = stack.top(-1);
	ActionValue& number1 = stack.top(0);
	if (number1.isNumeric() && number2.isNumeric())
	{
		int32_t n2 = int32_t(number2.getNumber());
		int32_t n1 = int32_t(number1.getNumber());
		number2 = ActionValue(avm_number_t(n1 | n2));
	}
	else
		number2 = ActionValue();
	stack.drop(1);
}

void opx_bitwiseXor(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue& number2 = stack.top(-1);
	ActionValue& number1 = stack.top(0);
	if (number1.isNumeric() && number2.isNumeric())
	{
		int32_t n2 = int32_t(number2.getNumber());
		int32_t n1 = int32_t(number1.getNumber());
		number2 = ActionValue(avm_number_t(n1 ^ n2));
	}
	else
		number2 = ActionValue();
	stack.drop(1);
}

void opx_shiftLeft(ExecutionState& state)
{
	VM_NOT_IMPLEMENTED;
}

void opx_shiftRight(ExecutionState& state)
{
	VM_NOT_IMPLEMENTED;
}

void opx_shiftRight2(ExecutionState& state)
{
	VM_NOT_IMPLEMENTED;
}

void opx_strictEq(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue value2 = stack.pop();
	ActionValue value1 = stack.pop();
	if (value1.getType() == value2.getType())
	{
		if (value1.isBoolean())
		{
			bool b2 = value2.getBoolean();
			bool b1 = value1.getBoolean();
			stack.push(ActionValue(b1 == b2));
		}
		else if (value1.isNumeric())
		{
			avm_number_t n2 = value2.getNumber();
			avm_number_t n1 = value1.getNumber();
			stack.push(ActionValue(n1 == n2));
		}
		else if (value1.isString())
		{
			std::string s2 = value2.getString();
			std::string s1 = value1.getString();
			stack.push(ActionValue(s1 == s2));
		}
		else if (value1.isObject())
		{
			ActionObject* o2 = value2.getObject();
			ActionObject* o1 = value1.getObject();
			stack.push(ActionValue(o1 == o2));
		}
		else
			stack.push(ActionValue(true));
	}
	else
		stack.push(ActionValue(false));
}

void opx_greater(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue value2 = stack.pop();
	ActionValue value1 = stack.pop();
	if (value2.isString() || value1.isString())
	{
		ActionValue string2 = value2.toString();
		ActionValue string1 = value1.toString();
		if (string2.isString() && string1.isString())
		{
			std::string str2 = string2.getString();
			std::string str1 = string1.getString();
			stack.push(ActionValue(bool(str1.compare(str2) > 0)));
		}
		else
			stack.push(ActionValue());
	}
	else
	{
		ActionValue number2 = value2.toNumber();
		ActionValue number1 = value1.toNumber();
		if (number2.isNumeric() && number1.isNumeric())
		{
			avm_number_t n2 = number2.getNumber();
			avm_number_t n1 = number1.getNumber();
			stack.push(ActionValue(bool(n1 > n2)));
		}
		else
			stack.push(ActionValue());
	}
}

void opx_stringGreater(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	ActionValue str2 = stack.pop();
	ActionValue str1 = stack.pop();
	stack.push(ActionValue(bool(str1.getStringSafe().compare(str2.getStringSafe()) > 0)));
}

void opx_extends(ExecutionState& state)
{
	ActionContext* context = state.frame->getContext();
	ActionValueStack& stack = state.frame->getStack();

	ActionValue superClass = stack.pop();
	ActionValue subClass = stack.pop();

	VM_LOG(subClass.getObject< ActionFunction >()->getName() << L" extends " << superClass.getObject< ActionFunction >()->getName());

	ActionValue superPrototype;
	superClass.getObject()->getMember(context, "prototype", superPrototype);

	Ref< ActionObject > prototype = new ActionObject();
	prototype->setMember("__proto__", superPrototype);
	prototype->setMember("__constructor__", ActionValue(superClass.getObject()));

	subClass.getObject()->setMember("prototype", ActionValue(prototype));
}

void opp_gotoFrame(PreparationState& state)
{
#if defined(T_BIG_ENDIAN)
	swap8in32(*reinterpret_cast< uint16_t* >(state.data));
#endif
}

void opx_gotoFrame(ExecutionState& state)
{
	FlashSpriteInstance* movieClip = state.frame->getContext()->getMovieClip();

	uint16_t frame = *reinterpret_cast< const uint16_t* >(state.data);
	movieClip->gotoFrame(frame);
}

void opx_getUrl(ExecutionState& state)
{
	ActionContext* context = state.frame->getContext();
	ActionValueStack& stack = state.frame->getStack();

	const char* url = reinterpret_cast< const char* >(state.data);
	const char* query = url + strlen(url) + 1;
	ActionValue getUrl;
	context->getGlobal()->getLocalMember("getUrl", getUrl);
	if (getUrl.isObject())
	{
		Ref< ActionFunction > function = dynamic_type_cast< ActionFunction* >(getUrl.getObject());
		if (function)
		{
			stack.push(ActionValue(query));
			stack.push(ActionValue(url));
			stack.push(ActionValue(avm_number_t(2)));
			function->call(state.frame, 0);
		}
		else
			log::warning << L"_global.getUrl must be a function object" << Endl;
	}
}

void opx_setRegister(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();

	uint8_t registerIndex = *state.data;
	state.frame->setRegister(registerIndex, !stack.empty() ? stack.top() : ActionValue());

	VM_LOG(L"Set register " << int32_t(registerIndex) << L" = " << (!stack.empty() ? stack.top().getWideStringSafe() : L"< Stack empty >"));
}

void opp_constantPool(PreparationState& state)
{
#if defined(T_BIG_ENDIAN)
	swap8in32(*reinterpret_cast< uint16_t* >(state.data));
#endif
}

void opx_constantPool(ExecutionState& state)
{
	uint16_t dictionaryCount = *reinterpret_cast< const uint16_t* >(state.data);
	const char* dictionaryEntry = reinterpret_cast< const char* >(state.data + 2);

	state.frame->setDictionary(new ActionDictionary(
		dictionaryCount,
		dictionaryEntry
	));
}

void opx_waitForFrame(ExecutionState& state)
{
}

void opx_setTarget(ExecutionState& state)
{
	VM_NOT_IMPLEMENTED;
}

void opx_gotoLabel(ExecutionState& state)
{
	FlashSpriteInstance* movieClip = state.frame->getContext()->getMovieClip();

	const char* label = reinterpret_cast< const char* >(state.data);
	int frame = movieClip->getSprite()->findFrame(label);
	if (frame >= 0)
	{
		movieClip->setPlaying(false);
		movieClip->gotoFrame(frame);
	}
}

void opx_waitForFrameExpression(ExecutionState& state)
{
	VM_NOT_IMPLEMENTED;
}

void opp_defineFunction2(PreparationState& state)
{
	uint8_t* data = state.data;

	const char* functionName = reinterpret_cast< const char* >(data);
	data += strlen(functionName) + 1;

#if defined(T_BIG_ENDIAN)
	swap8in32(*(uint16_t*)data);
#endif
	uint16_t argumentCount = *reinterpret_cast< const uint16_t* >(data);
	data += sizeof(uint16_t);

	data += sizeof(uint8_t);

#if defined(T_BIG_ENDIAN)
	swap8in32(*(uint16_t*)data);
#endif
	data += sizeof(uint16_t);

	for (int i = 0; i  < argumentCount; ++i)
	{
		data += sizeof(uint8_t);

		const char* variableName = reinterpret_cast< const char* >(data);
		data += strlen(variableName) + 1;
	}

#if defined(T_BIG_ENDIAN)
	swap8in32(*(uint16_t*)data);
#endif
	uint16_t codeSize = *reinterpret_cast< const uint16_t* >(data);
	data += sizeof(uint16_t);

	state.npc += codeSize;
}

void opx_defineFunction2(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	FlashSpriteInstance* movieClip = state.frame->getContext()->getMovieClip();

	const uint8_t* data = state.data;

	const char T_UNALIGNED * functionName = reinterpret_cast< const char* >(data);
	data += strlen(functionName) + 1;

	uint16_t argumentCount = *reinterpret_cast< const uint16_t* >(data);
	data += sizeof(uint16_t);

	uint8_t registerCount = *data;
	data += sizeof(uint8_t);

	uint16_t flags = *reinterpret_cast< const uint16_t* >(data);
	data += sizeof(uint16_t);

	std::vector< std::pair< std::string, uint8_t > > argumentsIntoRegisters(argumentCount);
	for (int i = 0; i  < argumentCount; ++i)
	{
		uint8_t registerIndex = *data;
		data += sizeof(uint8_t);

		const char* variableName = reinterpret_cast< const char* >(data);
		data += strlen(variableName) + 1;

		argumentsIntoRegisters[i] = std::make_pair(variableName, registerIndex);
	}

	uint16_t codeSize = *reinterpret_cast< const uint16_t* >(data);
	data += sizeof(uint16_t);

	Ref< ActionVMImage1 > image = new ActionVMImage1(state.npc, codeSize);
	image->prepare();

	Ref< ActionFunction > function = new ActionFunction2(
		functionName,
		image,
		registerCount,
		flags,
		argumentsIntoRegisters,
		state.frame->getVariables(),
		state.frame->getDictionary()
	);

	// Create our own prototype member as it will probably be filled with additional members.
	// Default extends the Object class.
	function->setMember("prototype", ActionValue(new ActionObject()));

	if (*functionName != 0)
	{
		// Assign function object into variable with the same name as the function.
		movieClip->setMember(functionName, ActionValue(function));
	}
	else
	{
		// Push function object onto stack.
		stack.push(ActionValue(function));
	}

	state.npc += codeSize;
}

void opx_try(ExecutionState& state)
{
	VM_NOT_IMPLEMENTED;
}

void opx_with(ExecutionState& state)
{
	VM_NOT_IMPLEMENTED;
}

void opp_pushData(PreparationState& state)
{
#if defined(T_BIG_ENDIAN)
	{
		uint8_t* data = state.data;
		uint8_t* end = data + state.length;
		while (data < end)
		{
			uint8_t type = *data++;
			if (type == 0)		// String
			{
				uint32_t length = uint32_t(strlen(reinterpret_cast< const char* >(data)));
				data += length + 1;
			}
			else if (type == 1)	// Number
			{
				swap8in32(*(float*)data);
				data += sizeof(float);
			}
			else if (type == 4)	// Register
				data += sizeof(uint8_t);
			else if (type == 5)	// Boolean
				data += sizeof(uint8_t);
			else if (type == 6)	// Double
				data += sizeof(double);
			else if (type == 7)	// Integer (32bit)
			{
				swap8in32(*(int32_t*)data);
				data += sizeof(int32_t);
			}
			else if (type == 8)	// Dictionary (8bit index)
				data += sizeof(uint8_t);
			else if (type == 9)	// Dictionary (16bit index)
			{
				swap8in32(*(uint16_t*)data);
				data += sizeof(uint16_t);
			}
		}
	}
#endif

	// Try to convert values and replace with custom type.
	{
		uint8_t nd[65536];
		uint8_t* ndp = nd;

		uint8_t* data = state.data;
		uint8_t* end = data + state.length;
		while (data < end)
		{
			uint8_t type = *data++;

			if (type == 0)		// String
			{
				uint32_t length = uint32_t(strlen((const char*)data));
				uint16_t index = state.image->addConstData(ActionValue((const char*)data));
				
				*ndp++ = 100;
				*(uint16_t*)ndp = index;
				ndp += sizeof(uint16_t);

				data += length + 1;
			}
			else if (type == 1)	// Number
			{
				uint16_t index = state.image->addConstData(ActionValue(*(const float*)data));

				*ndp++ = 100;
				*(uint16_t*)ndp = index;
				ndp += sizeof(uint16_t);

				data += sizeof(float);
			}
			else if (type == 2)	// Null
			{
				*ndp++ = 2;
			}
			else if (type == 3)	// Undefined
			{
				*ndp++ = 3;
			}
			else if (type == 4)	// Register
			{
				*ndp++ = 4;
				*ndp++ = *data++;
			}
			else if (type == 5)	// Boolean
			{
				*ndp++ = 5;
				*ndp++ = *data++;
			}
			else if (type == 6)	// Double
			{
				union { double d; uint8_t b[8]; uint32_t dw[2]; } w;

#if defined(T_LITTLE_ENDIAN)
				w.dw[0] = *(const uint32_t*)&data[4];
				w.dw[1] = *(const uint32_t*)&data[0];
#elif defined(T_BIG_ENDIAN)
				w.b[0] = data[3];
				w.b[1] = data[2];
				w.b[2] = data[1];
				w.b[3] = data[0];
				w.b[4] = data[7];
				w.b[5] = data[6];
				w.b[6] = data[5];
				w.b[7] = data[4];
#endif
				
				uint16_t index = state.image->addConstData(ActionValue(avm_number_t(w.d)));

				*ndp++ = 100;
				*(uint16_t*)ndp = index;
				ndp += sizeof(uint16_t);

				data += sizeof(double);
			}
			else if (type == 7)	// Integer (32bit)
			{
				uint16_t index = state.image->addConstData(ActionValue(avm_number_t(*(const int32_t*)data)));

				*ndp++ = 100;
				*(uint16_t*)ndp = index;
				ndp += sizeof(uint16_t);

				data += sizeof(int32_t);
			}
			else if (type == 8)	// Dictionary (8bit index)
			{
				*ndp++ = 8;
				*ndp++ = *data++;
			}
			else if (type == 9)	// Dictionary (16bit index)
			{
				*ndp++ = 9;
				*(uint16_t*)ndp = *(const uint16_t*)data;
				ndp += sizeof(uint16_t);

				data += sizeof(uint16_t);
			}
			else
				break;
		}

		if (int(ndp - nd) <= state.length)
		{
			while (int(ndp - nd) < state.length)
				*ndp++ = 200;

			std::memcpy(state.data, nd, state.length);
		}
	}
}

void opx_pushData(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();

	const uint8_t* data = state.data;
	const uint8_t* end = data + state.length;
	while (data < end)
	{
		ActionValue value;
		uint8_t type = *data++;

		if (type == 0)		// String
		{
			uint32_t length = uint32_t(strlen((const char*)data));
			value = ActionValue((const char*)data);
			data += length + 1;
		}
		else if (type == 1)	// Number
		{
			value = ActionValue(avm_number_t(*(const float*)data));
			data += sizeof(float);
		}
		else if (type == 2)	// Null
		{
			value = ActionValue((ActionObject*)0);
		}
		else if (type == 3)	// Undefined
		{
			// Do nothing, value is already undefined.
		}
		else if (type == 4)	// Register
		{
			uint8_t index = *data++;
			value = state.frame->getRegister(index);
		}
		else if (type == 5)	// Boolean
		{
			value = ActionValue(bool(*data++ ? true : false));
		}
		else if (type == 6)	// Double
		{
			union { double d; uint8_t b[8]; uint32_t dw[2]; } w;

#if defined(T_LITTLE_ENDIAN)
			w.dw[0] = *(const uint32_t*)&data[4];
			w.dw[1] = *(const uint32_t*)&data[0];
#elif defined(T_BIG_ENDIAN)
			w.b[0] = data[3];
			w.b[1] = data[2];
			w.b[2] = data[1];
			w.b[3] = data[0];
			w.b[4] = data[7];
			w.b[5] = data[6];
			w.b[6] = data[5];
			w.b[7] = data[4];
#endif
			
			value = ActionValue(avm_number_t(w.d));
			data += sizeof(double);
		}
		else if (type == 7)	// Integer (32bit)
		{
			value = ActionValue(avm_number_t(*reinterpret_cast< const int32_t* >(data)));
			data += sizeof(int32_t);
		}
		else if (type == 8)	// Dictionary (8bit index)
		{
			uint8_t index = *data++;
			value = ActionValue(state.frame->getDictionary()->get(index));
		}
		else if (type == 9)	// Dictionary (16bit index)
		{
			uint16_t index = *reinterpret_cast< const uint16_t* >(data);
			value = ActionValue(state.frame->getDictionary()->get(index));
			data += sizeof(uint16_t);
		}
		else if (type == 100)	// Preconverted constant value.
		{
			uint16_t index = *reinterpret_cast< const uint16_t* >(data);
			value = state.image->getConstData(index);
			data += sizeof(uint16_t);
		}
		else if (type == 200)	// End
			break;
		else
			break;

		stack.push(value);
	}
}

void opp_branchAlways(PreparationState& state)
{
#if defined(T_BIG_ENDIAN)
	swap8in32(*reinterpret_cast< int16_t* >(state.data));
#endif
}

void opx_branchAlways(ExecutionState& state)
{
	int16_t offset = *reinterpret_cast< const int16_t* >(state.data);
	state.npc += offset;
}

void opx_getUrl2(ExecutionState& state)
{
	ActionContext* context = state.frame->getContext();
	ActionValueStack& stack = state.frame->getStack();

	ActionValue getUrl;
	context->getGlobal()->getLocalMember("getUrl", getUrl);
	if (getUrl.isObject())
	{
		Ref< ActionFunction > function = dynamic_type_cast< ActionFunction* >(getUrl.getObject());
		if (function)
		{
			ActionValue target = stack.pop();
			ActionValue url = stack.pop();

			stack.push(target);
			stack.push(url);
			stack.push(ActionValue(avm_number_t(2)));

			function->call(state.frame, 0);
		}
		else
			log::error << L"_global.getUrl must be a function object" << Endl;
	}
}

void opp_defineFunction(PreparationState& state)
{
	uint8_t* data = state.data;

	const char* functionName = reinterpret_cast< const char* >(data);
	data += strlen(functionName) + 1;

#if defined(T_BIG_ENDIAN)
	swap8in32(*(uint16_t*)data);
#endif
	uint16_t argumentCount = *reinterpret_cast< const uint16_t* >(data);
	data += sizeof(uint16_t);

	for (int i = 0; i  < argumentCount; ++i)
	{
		const char* argumentName = reinterpret_cast< const char* >(data);
		data += strlen(argumentName) + 1;
	}

#if defined(T_BIG_ENDIAN)
	swap8in32(*(uint16_t*)data);
#endif
	uint16_t codeSize = *reinterpret_cast< const uint16_t* >(data);
	data += sizeof(uint16_t);

	state.npc += codeSize;
}

void opx_defineFunction(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	FlashSpriteInstance* movieClip = state.frame->getContext()->getMovieClip();

	const uint8_t* data = state.data;

	const char* functionName = reinterpret_cast< const char* >(data);
	data += strlen(functionName) + 1;

	uint16_t argumentCount = *reinterpret_cast< const uint16_t* >(data);
	data += sizeof(uint16_t);

	for (int i = 0; i  < argumentCount; ++i)
	{
		const char* argumentName = reinterpret_cast< const char* >(data);
		data += strlen(argumentName) + 1;
	}

	uint16_t codeSize = *reinterpret_cast< const uint16_t* >(data);
	data += sizeof(uint16_t);

	Ref< ActionVMImage1 > image = new ActionVMImage1(state.npc, codeSize);
	image->prepare();

	Ref< ActionFunction > function = new ActionFunction1(
		functionName,
		image,
		argumentCount,
		state.frame->getDictionary()
	);

	// Create our own prototype member as it will probably be filled with additional members.
	function->setMember("prototype", ActionValue(new ActionObject()));

	if (*functionName != 0)
	{
		// Assign function object into variable with the same name as the function.
		movieClip->setMember(functionName, ActionValue(function));
	}
	else
	{
		// Push function object onto stack.
		stack.push(ActionValue(function));
	}

	state.npc += codeSize;
}

void opp_branchIfTrue(PreparationState& state)
{
#if defined(T_BIG_ENDIAN)
	swap8in32(*reinterpret_cast< int16_t* >(state.data));
#endif
}

void opx_branchIfTrue(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();

	bool condition = stack.pop().getBooleanSafe();
	if (condition)
	{
		int16_t offset = *reinterpret_cast< const int16_t* >(state.data);
		state.npc += offset;
	}
}

void opx_callFrame(ExecutionState& state)
{
	VM_NOT_IMPLEMENTED;
}

void opx_gotoFrame2(ExecutionState& state)
{
	ActionValueStack& stack = state.frame->getStack();
	FlashSpriteInstance* movieClip = state.frame->getContext()->getMovieClip();

	ActionValue frame = stack.pop();
	int frameIndex = -1;

	if (frame.isNumeric())
	{
		frameIndex = int(frame.getNumber());
	}
	else if (frame.isString())
	{
		frameIndex = movieClip->getSprite()->findFrame(frame.getString());
	}
	else
	{
		log::error << L"Invalid frame in GotoFrame2" << Endl;
		return;
	}

	if (frameIndex < 0)
	{
		log::warning << L"No such frame" << Endl;
		return;
	}

	// Frame bias
	if ((state.data[0] & 1) == 1)
	{
		uint16_t frameBias = *reinterpret_cast< const uint16_t* >(&state.data[1]);
		frameIndex += frameBias;
	}

	// Play or stop
	if ((state.data[0] & 2) == 2)
		movieClip->setPlaying(true);
	else
		movieClip->setPlaying(false);

	movieClip->gotoFrame(frameIndex);
}

		}

const OperationInfo c_operationInfos[] =
{
	// 0x00
	{ AopEnd, "AopEnd", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopNextFrame, "AopNextFrame", 0, &opx_nextFrame },
	{ AopPrevFrame, "AopPrevFrame", 0, &opx_prevFrame },
	{ AopPlay, "AopPlay", 0, &opx_play },
	{ AopStop, "AopStop", 0, &opx_stop },
	{ AopToggleQuality, "AopToggleQuality", 0, &opx_toggleQuality },
	{ AopStopSounds, "AopStopSounds", 0, &opx_stopSounds },
	{ AopAdd, "AopAdd", 0, &opx_add },
	{ AopSubtract, "AopSubtract", 0, &opx_subtract },
	{ AopMultiply, "AopMultiply", 0, &opx_multiply },
	{ AopDivide, "AopDivide", 0, &opx_divide },
	{ AopEqual, "AopEqual", 0, &opx_equal },
	{ AopLessThan, "AopLessThan", 0, &opx_lessThan },

	// 0x10
	{ AopLogicalAnd, "AopLogicalAnd", 0, &opx_logicalAnd },
	{ AopLogicalOr, "AopLogicalOr", 0, &opx_logicalOr },
	{ AopLogicalNot, "AopLogicalNot", 0, &opx_logicalNot },
	{ AopStringEqual, "AopStringEqual", 0, &opx_stringEqual },
	{ AopStringLength, "AopStringLength", 0, &opx_stringLength },
	{ AopStringExtract, "AopStringExtract", 0, &opx_stringExtract },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopPop, "AopPop", 0, &opx_pop },
	{ AopInt, "AopInt", 0, &opx_int },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopGetVariable, "AopGetVariable", 0, &opx_getVariable },
	{ AopSetVariable, "AopSetVariable", 0, &opx_setVariable },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },

	// 0x20
	{ AopSetTargetExpression, "AopSetTargetExpression", 0, &opx_setTargetExpr },
	{ AopStringConcat, "AopStringConcat", 0, &opx_stringConcat },
	{ AopGetProperty, "AopGetProperty", 0, &opx_getProperty },
	{ AopSetProperty, "AopSetProperty", 0, &opx_setProperty },
	{ AopCloneSprite, "AopCloneSprite", 0, &opx_cloneSprite },
	{ AopRemoveSprite, "AopRemoveSprite", 0, &opx_removeSprite },
	{ AopTrace, "AopTrace", 0, &opx_trace },
	{ AopStartDragMovie, "AopStartDragMovie", 0, &opx_startDragMovie },
	{ AopStopDragMovie, "AopStopDragMovie", 0, &opx_stopDragMovie },
	{ AopStringCompare, "AopStringCompare", 0, &opx_stringCompare },
	{ AopThrow, "AopThrow", 0, &opx_throw },
	{ AopCastOp, "AopCastOp", 0, &opx_castOp },
	{ AopImplementsOp, "AopImplementsOp", 0, &opx_implementsOp },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },

	// 0x30
	{ AopRandom, "AopRandom", 0, &opx_random },
	{ AopMbLength, "AopMbLength", 0, &opx_mbLength },
	{ AopOrd, "AopOrd", 0, &opx_ord },
	{ AopChr, "AopChr", 0, &opx_chr },
	{ AopGetTime, "AopGetTime", 0, &opx_getTime },
	{ AopMbSubString, "AopMbSubString", 0, &opx_mbSubString },
	{ AopMbOrd, "AopMbOrd", 0, &opx_mbOrd },
	{ AopMbChr, "AopMbChr", 0, &opx_mbChr },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopDelete, "AopDelete", 0, &opx_delete },
	{ AopDelete2, "AopDelete2", 0, &opx_delete2 },
	{ AopDefineLocal, "AopDefineLocal", 0, &opx_defineLocal },
	{ AopCallFunction, "AopCallFunction", 0, &opx_callFunction },
	{ AopReturn, "AopReturn", 0, 0 },
	{ AopModulo, "AopModulo", 0, &opx_modulo },

	// 0x40
	{ AopNew, "AopNew", 0, &opx_new },
	{ AopDefineLocal2, "AopDefineLocal2", 0, &opx_defineLocal2 },
	{ AopInitArray, "AopInitArray", 0, &opx_initArray },
	{ AopInitObject, "AopInitObject", 0, &opx_initObject },
	{ AopTypeOf, "AopTypeOf", 0, &opx_typeOf },
	{ AopTargetPath, "AopTargetPath", 0, &opx_targetPath },
	{ AopEnumerate, "AopEnumerate", 0, &opx_enumerate },
	{ AopNewAdd, "AopNewAdd", 0, &opx_newAdd },
	{ AopNewLessThan, "AopNewLessThan", 0, &opx_newLessThan },
	{ AopNewEquals, "AopNewEquals", 0, &opx_newEquals },
	{ AopToNumber, "AopToNumber", 0, &opx_toNumber },
	{ AopToString, "AopToString", 0, &opx_toString },
	{ AopDup, "AopDup", 0, &opx_dup },
	{ AopSwap, "AopSwap", 0, &opx_swap },
	{ AopGetMember, "AopGetMember", 0, &opx_getMember },
	{ AopSetMember, "AopSetMember", 0, &opx_setMember },

	// 0x50
	{ AopIncrement, "AopIncrement", 0, &opx_increment },
	{ AopDecrement, "AopDecrement", 0, &opx_decrement },
	{ AopCallMethod, "AopCallMethod", 0, &opx_callMethod },
	{ AopNewMethod, "AopNewMethod", 0, &opx_newMethod },
	{ AopInstanceOf, "AopInstanceOf", 0, &opx_instanceOf },
	{ AopEnum2, "AopEnum2", 0, &opx_enum2 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },

	// 0x60
	{ AopBitwiseAnd, "AopBitwiseAnd", 0, &opx_bitwiseAnd },
	{ AopBitwiseOr, "AopBitwiseOr", 0, &opx_bitwiseOr },
	{ AopBitwiseXor, "AopBitwiseXor", 0, &opx_bitwiseXor },
	{ AopShiftLeft, "AopShiftLeft", 0, &opx_shiftLeft },
	{ AopShiftRight, "AopShiftRight", 0, &opx_shiftRight },
	{ AopShiftRight2, "AopShiftRight2", 0, &opx_shiftRight2 },
	{ AopStrictEq, "AopStrictEq", 0, &opx_strictEq },
	{ AopGreater, "AopGreater", 0, &opx_greater },
	{ AopStringGreater, "AopStringGreater", 0, &opx_stringGreater },
	{ AopExtends, "AopExtends", 0, &opx_extends },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },

	// 0x70
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },

	// 0x80
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopGotoFrame, "AopGotoFrame", &opp_gotoFrame, &opx_gotoFrame },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopGetUrl, "AopGetUrl", 0, &opx_getUrl },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopSetRegister, "AopSetRegister", 0, &opx_setRegister },
	{ AopConstantPool, "AopConstantPool", &opp_constantPool, &opx_constantPool },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopWaitForFrame, "AopWaitForFrame", 0, &opx_waitForFrame },
	{ AopSetTarget, "AopSetTarget", 0, &opx_setTarget },
	{ AopGotoLabel, "AopGotoLabel", 0, &opx_gotoLabel },
	{ AopWaitForFrameExpression, "AopWaitForFrameExpression", 0, &opx_waitForFrameExpression },
	{ AopDefineFunction2, "AopDefineFunction2", &opp_defineFunction2, &opx_defineFunction2 },
	{ AopTry, "AopTry", 0, &opx_try },

	// 0x90
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopWith, "AopWith", 0, &opx_with },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopPushData, "AopPushData", &opp_pushData, &opx_pushData },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopBranchAlways, "AopBranchAlways", &opp_branchAlways, &opx_branchAlways },
	{ AopGetUrl2, "AopGetUrl2", 0, &opx_getUrl2 },
	{ AopDefineFunction, "AopDefineFunction", &opp_defineFunction, &opx_defineFunction },
	{ AopReserved, "AopReserved", 0, 0 },
	{ AopBranchIfTrue, "AopBranchIfTrue", &opp_branchIfTrue, &opx_branchIfTrue },
	{ AopCallFrame, "AopCallFrame", 0, &opx_callFrame },
	{ AopGotoFrame2, "AopGotoFrame2", 0, &opx_gotoFrame2 }
};

	}
}
