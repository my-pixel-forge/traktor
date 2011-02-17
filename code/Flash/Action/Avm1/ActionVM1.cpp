#include <cstring>
#include <cmath>
#include "Core/Log/Log.h"
#include "Core/Misc/TString.h"
#include "Core/Misc/Endian.h"
#include "Flash/FlashSprite.h"
#include "Flash/FlashSpriteInstance.h"
#include "Flash/Action/ActionFrame.h"
#include "Flash/Action/ActionContext.h"
#include "Flash/Action/ActionDictionary.h"
#include "Flash/Action/Avm1/ActionVM1.h"
#include "Flash/Action/Avm1/ActionFunction1.h"
#include "Flash/Action/Avm1/ActionFunction2.h"
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

inline int16_t readInt16(const uint8_t T_UNALIGNED * data)
{
	int16_t value = *reinterpret_cast< const int16_t T_UNALIGNED * >(data);
#if defined(T_BIG_ENDIAN)
	swap8in32(value);
#endif
	return value;
}

inline uint16_t readUInt16(const uint8_t T_UNALIGNED * data)
{
	uint16_t value = *reinterpret_cast< const uint16_t T_UNALIGNED * >(data);
#if defined(T_BIG_ENDIAN)
	swap8in32(value);
#endif
	return value;
}

inline int32_t readInt32(const uint8_t T_UNALIGNED * data)
{
	int32_t value = *reinterpret_cast< const int32_t T_UNALIGNED * >(data);
#if defined(T_BIG_ENDIAN)
	swap8in32(value);
#endif
	return value;
}

inline float readFloat(const uint8_t* data)
{
	float value = *reinterpret_cast< const float T_UNALIGNED * >(data);
#if defined(T_BIG_ENDIAN)
	swap8in32(value);
#endif
	return value;
}

		}

T_IMPLEMENT_RTTI_CLASS(L"traktor.flash.ActionVM1", ActionVM1, IActionVM)

ActionVM1::ActionVM1()
{
	m_timer.start();
}

void ActionVM1::execute(ActionFrame* frame) const
{
	VM_LOG(L"-- Execute begin --");

	ActionContext* context = frame->getContext();
	ActionValueStack& stack = frame->getStack();

	FlashSpriteInstance* movieClip = dynamic_type_cast< FlashSpriteInstance* >(frame->getSelf());
	if (!movieClip)
	{
		// Not a movie clip; get root movie clip from global object.
		ActionValue root;
		context->getGlobal()->getLocalMember("_root", root);
		movieClip = root.getObject< FlashSpriteInstance >();
	}

	Ref< const FlashSprite > movie = movieClip ? movieClip->getSprite() : 0;

	const uint8_t T_UNALIGNED * pc = frame->getCode();
	T_ASSERT (pc);

	const uint8_t T_UNALIGNED * end = pc + frame->getCodeSize();
	while (pc < end)
	{
		const uint8_t op = *pc;
		const uint8_t T_UNALIGNED * npc = pc + 1;
		const uint8_t T_UNALIGNED * data = 0;
		uint16_t length = 1;

		if (op & 0x80)
		{
			length = *reinterpret_cast< const uint16_t T_UNALIGNED * >(pc + 1);
			data = pc + 3;
			npc = pc + 3 + length;
		}

		if (op == AopEnd || op == AopReturn)
			break;
		
		switch (op)
		{
		VM_BEGIN(AopNextFrame)
			T_ASSERT (movieClip);
			movieClip->gotoNext();
		VM_END()

		VM_BEGIN(AopPrevFrame)
			T_ASSERT (movieClip);
			movieClip->gotoPrevious();
		VM_END()

		VM_BEGIN(AopPlay)
			T_ASSERT (movieClip);
			movieClip->setPlaying(true);
		VM_END()

		VM_BEGIN(AopStop)
			T_ASSERT (movieClip);
			movieClip->setPlaying(false);
		VM_END()

		VM_BEGIN(AopToggleQuality)
		VM_END()

		VM_BEGIN(AopStopSounds)
		VM_END()

		VM_BEGIN(AopGotoFrame)
			T_ASSERT (movieClip);
			uint16_t frame = readUInt16(data);
			movieClip->gotoFrame(frame);
		VM_END()

		VM_BEGIN(AopGetUrl)
			const char T_UNALIGNED * url = reinterpret_cast< const char T_UNALIGNED * >(data);
			const char T_UNALIGNED * query = url + strlen(url) + 1;
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
					function->call(frame, 0);
				}
				else
					log::warning << L"_global.getUrl must be a function object" << Endl;
			}
		VM_END()

		VM_BEGIN(AopWaitForFrame)
		VM_END()

		VM_BEGIN(AopSetTarget)
			VM_NOT_IMPLEMENTED
		VM_END()

		VM_BEGIN(AopGotoLabel)
			T_ASSERT (movieClip);
			const char T_UNALIGNED * label = reinterpret_cast< const char T_UNALIGNED * >(data);
			int frame = movie->findFrame(label);
			if (frame >= 0)
			{
				movieClip->setPlaying(false);
				movieClip->gotoFrame(frame);
			}
		VM_END()

		VM_BEGIN(AopAdd)
			ActionValue& number2 = stack.top(0);
			ActionValue& number1 = stack.top(-1);
			if (number1.isNumeric() && number2.isNumeric())
				number1 = ActionValue(number1.getNumber() + number2.getNumber());
			else
				number1 = ActionValue();
			stack.drop(1);
		VM_END()

		VM_BEGIN(AopSubtract)
			ActionValue& number2 = stack.top(0);
			ActionValue& number1 = stack.top(-1);
			if (number1.isNumeric() && number2.isNumeric())
				number1 = ActionValue(number1.getNumber() - number2.getNumber());
			else
				number1 = ActionValue();
			stack.drop(1);
		VM_END()

		VM_BEGIN(AopMultiply)
			ActionValue& number2 = stack.top(0);
			ActionValue& number1 = stack.top(-1);
			if (number1.isNumeric() && number2.isNumeric())
				number1 = ActionValue(number1.getNumber() * number2.getNumber());
			else
				number1 = ActionValue();
			stack.drop(1);
		VM_END()

		VM_BEGIN(AopDivide)
			ActionValue& number2 = stack.top(0);
			ActionValue& number1 = stack.top(-1);
			if (number1.isNumeric() && number2.isNumeric())
				number1 = ActionValue(number1.getNumber() / number2.getNumber());
			else
				number1 = ActionValue();
			stack.drop(1);
		VM_END()

		VM_BEGIN(AopEqual)
			ActionValue& number2 = stack.top(0);
			ActionValue& number1 = stack.top(-1);
			if (number1.isNumeric() && number2.isNumeric())
				number1 = ActionValue(bool(number1.getNumber() == number2.getNumber()));
			else
				number1 = ActionValue();
			stack.drop(1);
		VM_END()

		VM_BEGIN(AopLessThan)
			ActionValue& number2 = stack.top(0);
			ActionValue& number1 = stack.top(-1);
			if (number1.isNumeric() && number2.isNumeric())
				number1 = ActionValue(bool(number1.getNumber() < number2.getNumber()));
			else
				number1 = ActionValue();
			stack.drop(1);
		VM_END()

		VM_BEGIN(AopLogicalAnd)
			ActionValue number2 = stack.pop().toBoolean();
			ActionValue number1 = stack.pop().toBoolean();
			stack.push(ActionValue(bool(number1.getBoolean() && number2.getBoolean())));
		VM_END()

		VM_BEGIN(AopLogicalOr)
			ActionValue number2 = stack.pop().toBoolean();
			ActionValue number1 = stack.pop().toBoolean();
			stack.push(ActionValue(bool(number1.getBoolean() || number2.getBoolean())));
		VM_END()

		VM_BEGIN(AopLogicalNot)
			ActionValue value = stack.pop().toBoolean();
			stack.push(ActionValue(bool(!value.getBoolean())));
		VM_END()

		VM_BEGIN(AopStringEqual)
			ActionValue str2 = stack.pop();
			ActionValue str1 = stack.pop();
			stack.push(ActionValue(bool(str1.getStringSafe().compare(str2.getStringSafe()) == 0)));
		VM_END()

		VM_BEGIN(AopStringLength)
			ActionValue str = stack.pop();
			stack.push(ActionValue(avm_number_t(str.getStringSafe().length())));
		VM_END()

		VM_BEGIN(AopStringExtract)
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
		VM_END()

		VM_BEGIN(AopPop)
			stack.pop();
		VM_END()

		VM_BEGIN(AopInt)
			ActionValue& number = stack.top();
			if (number.isNumeric())
				number = ActionValue(avm_number_t(std::floor(number.getNumber())));
			else
				number = ActionValue();
		VM_END()

		VM_BEGIN(AopGetVariable)
			std::string variableName = stack.pop().getStringSafe();
			ActionValue value;

			if (frame->getVariable(variableName, value))
				stack.push(value);
			else if (frame->getSelf()->getMember(context, variableName, value))
				stack.push(value);
			else if (movieClip->getMember(context, variableName, value))
				stack.push(value);
			else if (context->getGlobal()->getLocalMember(variableName, value))
				stack.push(value);
			else if (variableName == "_global")
			{
				value = ActionValue(context->getGlobal());
				stack.push(value);
			}
			else
			{
				FlashDisplayList::layer_map_t::const_iterator i = movieClip->getDisplayList().findLayer(variableName);
				if (i != movieClip->getDisplayList().getLayers().end())
					value = ActionValue(i->second.instance);
				stack.push(value);
			}

			VM_LOG(L"Get variable \"" << mbstows(variableName) << L"\" = " << value.getWideStringSafe());
		VM_END()

		VM_BEGIN(AopSetVariable)
			ActionValue value = stack.pop();
			std::string variableName = stack.pop().getStringSafe();

			if (frame->hasVariable(variableName))
				frame->setVariable(variableName, value);
			else
				movieClip->setMember(variableName, value);

			VM_LOG(L"Set variable \"" << mbstows(variableName) << L"\" = " << value.getWideStringSafe());
		VM_END()

		VM_BEGIN(AopSetTargetExpression)
		VM_END()

		VM_BEGIN(AopStringConcat)
			ActionValue str2 = stack.pop();
			ActionValue str1 = stack.pop();
			stack.push(ActionValue(str1.getStringSafe() + str2.getStringSafe()));
		VM_END()

		VM_BEGIN(AopGetProperty)
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
				stack.push(ActionValue(avm_number_t(movie->getFrameCount())));
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
		VM_END()

		VM_BEGIN(AopSetProperty)
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
		VM_END()

		VM_BEGIN(AopCloneSprite)
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
		VM_END()

		VM_BEGIN(AopRemoveSprite)
			VM_NOT_IMPLEMENTED
		VM_END()

		VM_BEGIN(AopTrace)
			std::wstring trace = stack.pop().getWideStringSafe();
			if (trace != L"** BREAK **")
				log::debug << L"TRACE \"" << trace << L"\"" << Endl;
			else
				T_BREAKPOINT;
		VM_END()

		VM_BEGIN(AopStartDragMovie)
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
		VM_END()

		VM_BEGIN(AopStopDragMovie)
			// @fixme
		VM_END()

		VM_BEGIN(AopStringCompare)
			ActionValue str2 = stack.pop();
			ActionValue str1 = stack.pop();
			stack.push(ActionValue(avm_number_t(str1.getStringSafe().compare(str2.getStringSafe()))));
		VM_END()

		VM_BEGIN(AopThrow)
			VM_NOT_IMPLEMENTED
		VM_END()

		VM_BEGIN(AopCastOp)
			VM_NOT_IMPLEMENTED
		VM_END()

		VM_BEGIN(AopImplementsOp)
			ActionValue implementationClass = stack.pop();
			uint32_t interfaceCount = uint32_t(stack.pop().getNumberSafe());
			for (uint32_t i = 0; i < interfaceCount; ++i)
			{
				ActionValue interfaceClass = stack.pop();
				implementationClass.getObject()->addInterface(interfaceClass.getObject());
			}
		VM_END()

		VM_BEGIN(AopRandom)
			ActionValue& max = stack.top();
			if (max.isNumeric())
			{
				int32_t rnd = std::rand() % std::max< int32_t >(int32_t(max.getNumber()), 1);
				max = ActionValue(avm_number_t(rnd));
			}
			else
				max = ActionValue();
		VM_END()

		VM_BEGIN(AopMbLength)
			VM_NOT_IMPLEMENTED
		VM_END()

		VM_BEGIN(AopOrd)
			VM_NOT_IMPLEMENTED
		VM_END()

		VM_BEGIN(AopChr)
			VM_NOT_IMPLEMENTED
		VM_END()

		VM_BEGIN(AopGetTime)
			avm_number_t sinceStartup = avm_number_t(m_timer.getElapsedTime() * 1000.0);
			stack.push(ActionValue(sinceStartup));
		VM_END()

		VM_BEGIN(AopMbSubString)
			VM_NOT_IMPLEMENTED
		VM_END()

		VM_BEGIN(AopMbOrd)
			VM_NOT_IMPLEMENTED
		VM_END()

		VM_BEGIN(AopMbChr)
			VM_NOT_IMPLEMENTED
		VM_END()

		VM_BEGIN(AopWaitForFrameExpression)
			VM_NOT_IMPLEMENTED
		VM_END()

		VM_BEGIN(AopPushData)
			const uint8_t T_UNALIGNED * end = data + length;
			while (data < end)
			{
				ActionValue value;
				uint8_t type = *data++;

				if (type == 0)		// String
				{
					uint32_t length = uint32_t(strlen(reinterpret_cast< const char T_UNALIGNED * >(data)));
					value = ActionValue(reinterpret_cast< const char T_UNALIGNED * >(data));
					data += length + 1;
					VM_LOG(L"Push data, string " << value.getWideStringSafe());
				}
				else if (type == 1)	// Number
				{
					value = ActionValue(avm_number_t(readFloat(data)));
					data += sizeof(float);
					VM_LOG(L"Push data, number " << value.getWideStringSafe());
				}
				else if (type == 2)	// Null
				{
					value = ActionValue((ActionObject*)0);
					VM_LOG(L"Push data, null");
				}
				else if (type == 3)	// Undefined
				{
					// Do nothing, value is already undefined.
					VM_LOG(L"Push data, undefined");
				}
				else if (type == 4)	// Register
				{
					uint8_t index = *data;
					value = frame->getRegister(index);
					data += sizeof(uint8_t);
					VM_LOG(L"Push data, register " << int32_t(index) << L" (" << value.getWideStringSafe() << L")");
				}
				else if (type == 5)	// Boolean
				{
					value = ActionValue(bool(*data ? true : false));
					data += sizeof(uint8_t);
					VM_LOG(L"Push data, boolean " << value.getWideStringSafe());
				}
				else if (type == 6)	// Double
				{
					union { double d; uint8_t b[8]; uint32_t dw[2]; } w;

#if defined(T_LITTLE_ENDIAN)
					w.dw[0] = *reinterpret_cast< const uint32_t T_UNALIGNED * >(&data[4]);
					w.dw[1] = *reinterpret_cast< const uint32_t T_UNALIGNED * >(&data[0]);
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
					VM_LOG(L"Push data, double " << value.getWideStringSafe());
				}
				else if (type == 7)	// Integer (32bit)
				{
					value = ActionValue(avm_number_t(readInt32(data)));
					data += sizeof(int32_t);
					VM_LOG(L"Push data, integer " << value.getWideStringSafe());
				}
				else if (type == 8)	// Dictionary (8bit index)
				{
					uint8_t index = *data;
					value = ActionValue(frame->getDictionary()->get(index));
					data += sizeof(uint8_t);
					VM_LOG(L"Push data, dictionary " << int32_t(index) << L" (" << value.getWideStringSafe() << L")");
				}
				else if (type == 9)	// Dictionary (16bit index)
				{
					uint16_t index = readUInt16(data);
					value = ActionValue(frame->getDictionary()->get(index));
					data += sizeof(uint16_t);
					VM_LOG(L"Push data, dictionary " << int32_t(index) << L" (" << value.getWideStringSafe() << L")");
				}
				else
				{
					log::warning << L"Unknown data type " << type << L", out of sync" << Endl;
				}

				stack.push(value);
			}
		VM_END()

		VM_BEGIN(AopBranchAlways)
			int16_t offset = readInt16(data);
			npc += offset;
		VM_END()

		VM_BEGIN(AopGetUrl2)
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

					function->call(frame, 0);
				}
				else
					log::error << L"_global.getUrl must be a function object" << Endl;
			}
		VM_END()

		VM_BEGIN(AopBranchIfTrue)
			bool condition = stack.pop().getBooleanSafe();
			if (condition)
			{
				int16_t offset = readInt16(data);
				npc += offset;
			}
		VM_END()

		VM_BEGIN(AopCallFrame)
			VM_NOT_IMPLEMENTED
		VM_END()

		VM_BEGIN(AopGotoFrame2)
			ActionValue frame = stack.pop();
			int frameIndex = -1;

			if (frame.isNumeric())
			{
				frameIndex = int(frame.getNumber());
			}
			else if (frame.isString())
			{
				frameIndex = movie->findFrame(frame.getString());
			}
			else
			{
				log::error << L"Invalid frame in GotoFrame2" << Endl;
				break;
			}

			if (frameIndex < 0)
			{
				log::warning << L"No such frame" << Endl;
				break;
			}

			// Frame bias
			if ((data[0] & 1) == 1)
			{
				uint16_t frameBias = *reinterpret_cast< const uint16_t T_UNALIGNED* >(&data[1]);
				frameIndex += frameBias;
			}

			// Play or stop
			if ((data[0] & 2) == 2)
				movieClip->setPlaying(true);
			else
				movieClip->setPlaying(false);

			movieClip->gotoFrame(frameIndex);
		VM_END()

		VM_BEGIN(AopDelete)
			std::string memberName = stack.pop().getStringSafe();
			ActionValue objectValue = stack.pop();
			if (objectValue.isObject())
			{
				bool deleted = objectValue.getObject()->deleteMember(memberName);
				stack.push(ActionValue(deleted));
			}
			else
				stack.push(ActionValue(false));
		VM_END()

		VM_BEGIN(AopDelete2)
			std::string variableName = stack.pop().getStringSafe();
			bool deleted = movieClip->deleteMember(variableName);
			stack.push(ActionValue(deleted));
		VM_END()

		VM_BEGIN(AopDefineLocal)
			ActionValue variableValue = stack.pop();
			std::string variableName = stack.pop().getStringSafe();
			if (frame->getCallee())
				frame->setVariable(variableName, variableValue);
			else
				movieClip->setMember(variableName, variableValue);
			VM_LOG(L"Set local variable \"" << mbstows(variableName) << L"\" = " << variableValue.getWideStringSafe());
		VM_END()

		VM_BEGIN(AopCallFunction)
			std::string functionName = stack.pop().getStringSafe();
			ActionValue functionObject;

			if (!frame->getVariable(functionName, functionObject))
			{
				if (!frame->getSelf()->getMember(context, functionName, functionObject))
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
				stack.push(function->call(frame, frame->getSelf()));
			}
			else
			{
				log::warning << L"Undefined function \"" << mbstows(functionName) << L"\"" << Endl;
				int argCount = int(stack.pop().getNumber());
				stack.drop(argCount);
				stack.push(ActionValue());
			}
		VM_END()

		VM_BEGIN(AopModulo)
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
		VM_END()

		VM_BEGIN(AopNew)
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
				ActionValue object = classConstructor->call(frame, self);
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
		VM_END()

		VM_BEGIN(AopDefineLocal2)
			std::string variableName = stack.pop().getStringSafe();
			if (frame->getCallee())
				frame->setVariable(variableName, ActionValue());
			else
				movieClip->setMember(variableName, ActionValue());
		VM_END()

		VM_BEGIN(AopInitArray)
			ActionValue arrayClassMember;
			if (context->getGlobal()->getLocalMember("Array", arrayClassMember) && arrayClassMember.isObject())
			{
				ActionValue arrayObject = arrayClassMember.getObject< ActionFunction >()->call(frame, 0);
				stack.push(arrayObject);
			}
			else
				stack.push(ActionValue());
		VM_END()

		VM_BEGIN(AopInitObject)
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
		VM_END()

		VM_BEGIN(AopTypeOf)
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
		VM_END()

		VM_BEGIN(AopTargetPath)
			VM_NOT_IMPLEMENTED
		VM_END()

		VM_BEGIN(AopEnumerate)
			VM_NOT_IMPLEMENTED
		VM_END()

		VM_BEGIN(AopNewAdd)
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
		VM_END()

		VM_BEGIN(AopNewLessThan)
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
		VM_END()

		VM_BEGIN(AopNewEquals)
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
			
		VM_END()

		VM_BEGIN(AopToNumber)
			stack.top() = stack.top().toNumber();
		VM_END()

		VM_BEGIN(AopToString)
			stack.top() = stack.top().toString();
		VM_END()

		VM_BEGIN(AopDup)
			stack.push(stack.top());
		VM_END()

		VM_BEGIN(AopSwap)
			ActionValue v1 = stack.pop();
			ActionValue v2 = stack.pop();
			stack.push(v1);
			stack.push(v2);
		VM_END()

		VM_BEGIN(AopGetMember)
			std::string memberName = stack.pop().getStringSafe();
			ActionValue targetValue = stack.pop();
			ActionValue memberValue;

			if (targetValue.isObject())
			{
				ActionObject* target = targetValue.getObject();
				if (target)
				{
					Ref< ActionFunction > propertyGet;
					if (target->getPropertyGet(context, memberName, propertyGet))
					{
						stack.push(ActionValue(avm_number_t(0)));
						memberValue = propertyGet->call(frame, target);
					}
					else
						target->getMember(context, memberName, memberValue);
				}
				else
					VM_LOG(L"Target null");
			}
			else
				VM_LOG(L"Target undefined");
			
			stack.push(memberValue);
			VM_LOG(L"Get member \"" << mbstows(memberName) << L"\" = " << memberValue.getWideStringSafe());
		VM_END()

		VM_BEGIN(AopSetMember)
			ActionValue memberValue = stack.pop();
			std::string memberName = stack.pop().getStringSafe();
			VM_LOG(L"Set member \"" << mbstows(memberName) << L"\" = \"" << memberValue.getWideStringSafe() << L"\"");

			ActionValue targetValue = stack.pop();
			if (targetValue.isObject())
			{
				Ref< ActionObject > target = targetValue.getObject();
				Ref< ActionFunction > propertySet;
				if (target->getPropertySet(context, memberName, propertySet))
				{
					stack.push(memberValue);
					stack.push(ActionValue(avm_number_t(1)));
					propertySet->call(frame, target);
				}
				else
				{
					VM_LOG(L"on target " << target->toString().getWideStringSafe());
					target->setMember(memberName, memberValue);
				}
			}
			else
				VM_LOG(L"Target undefined");

#if defined(_DEBUG)
			if (memberValue.isObject())
			{
				Ref< ActionFunction > memberFunction = memberValue.getObject< ActionFunction >();
				if (memberFunction)
					memberFunction->setName(memberName);
			}
#endif
		VM_END()

		VM_BEGIN(AopIncrement)
			ActionValue& number = stack.top();
			if (number.isNumeric())
				number = ActionValue(number.getNumber() + 1);
			else
				number = ActionValue();
		VM_END()

		VM_BEGIN(AopDecrement)
			ActionValue& number = stack.top();
			if (number.isNumeric())
				number = ActionValue(number.getNumber() - 1);
			else
				number = ActionValue();
		VM_END()

		VM_BEGIN(AopCallMethod)
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
				returnValue = method->call(frame, target);
			else
			{
				log::warning << L"Undefined method \"" << classConstructorName.getWideStringSafe() << L"\"" << Endl;
				int argCount = int(stack.pop().getNumber());
				stack.drop(argCount);
			}

			stack.push(returnValue);
		VM_END()

		VM_BEGIN(AopNewMethod)
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
					ActionValue object = classConstructor->call(frame, self);
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
		VM_END()

		VM_BEGIN(AopInstanceOf)
			VM_NOT_IMPLEMENTED
		VM_END()

		VM_BEGIN(AopEnum2)
			ActionValue enumObject = stack.pop();
			if (enumObject.isObject())
			{
				ActionObject* object = enumObject.getObject();
				const ActionObject::property_map_t& properties = object->getProperties();
				for (ActionObject::property_map_t::const_iterator i = properties.begin(); i != properties.end(); ++i)
					stack.push(ActionValue(i->first));
			}
			stack.push(ActionValue(avm_number_t(0)));
		VM_END()

		VM_BEGIN(AopBitwiseAnd)
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
		VM_END()

		VM_BEGIN(AopBitwiseOr)
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
		VM_END()

		VM_BEGIN(AopBitwiseXor)
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
		VM_END()

		VM_BEGIN(AopShiftLeft)
			VM_NOT_IMPLEMENTED
		VM_END()

		VM_BEGIN(AopShiftRight)
			VM_NOT_IMPLEMENTED
		VM_END()

		VM_BEGIN(AopShiftRight2)
			VM_NOT_IMPLEMENTED
		VM_END()

		VM_BEGIN(AopStrictEq)
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
		VM_END()

		VM_BEGIN(AopGreater)
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
		VM_END()

		VM_BEGIN(AopStringGreater)
			ActionValue str2 = stack.pop();
			ActionValue str1 = stack.pop();
			stack.push(ActionValue(bool(str1.getStringSafe().compare(str2.getStringSafe()) > 0)));
		VM_END()

		VM_BEGIN(AopExtends)
			ActionValue superClass = stack.pop();
			ActionValue subClass = stack.pop();

			VM_LOG(subClass.getObject< ActionFunction >()->getName() << L" extends " << superClass.getObject< ActionFunction >()->getName());

			ActionValue superPrototype;
			superClass.getObject()->getMember(context, "prototype", superPrototype);

			Ref< ActionObject > prototype = new ActionObject();
			prototype->setMember("__proto__", superPrototype);
			prototype->setMember("__constructor__", ActionValue(superClass.getObject()));

			subClass.getObject()->setMember("prototype", ActionValue(prototype));
		VM_END()

		VM_BEGIN(AopConstantPool)
			uint16_t dictionaryCount = readUInt16(data);
			const char T_UNALIGNED * dictionaryEntry = reinterpret_cast< const char T_UNALIGNED * >(data + 2);

			VM_LOG(L"Dictionary count = " << dictionaryCount);
			frame->setDictionary(new ActionDictionary(
				dictionaryCount,
				dictionaryEntry
			));
		VM_END()

		VM_BEGIN(AopDefineFunction2)
			const char T_UNALIGNED * functionName = reinterpret_cast< const char T_UNALIGNED * >(data);
			data += strlen(functionName) + 1;

			uint16_t argumentCount = readUInt16(data);
			data += sizeof(uint16_t);

			uint8_t registerCount = *data;
			data += sizeof(uint8_t);

			uint16_t flags = readUInt16(data);
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

			uint16_t codeSize = readUInt16(data);
			data += sizeof(uint16_t);

			Ref< ActionFunction > function = new ActionFunction2(
				functionName,
				npc,
				codeSize,
				registerCount,
				flags,
				argumentsIntoRegisters,
				frame->getVariables(),
				frame->getDictionary()
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

			npc += codeSize;
		VM_END()

		VM_BEGIN(AopTry)
			VM_NOT_IMPLEMENTED
		VM_END()

		VM_BEGIN(AopWith)
			VM_NOT_IMPLEMENTED
		VM_END()

		VM_BEGIN(AopDefineFunction)
			const char T_UNALIGNED * functionName = reinterpret_cast< const char T_UNALIGNED * >(data);
			data += strlen(functionName) + 1;

			uint16_t argumentCount = readUInt16(data);
			data += sizeof(uint16_t);

			//std::vector< std::wstring > arguments(argumentCount);
			for (int i = 0; i  < argumentCount; ++i)
			{
				const char T_UNALIGNED * argumentName = reinterpret_cast< const char T_UNALIGNED * >(data);
				data += strlen(argumentName) + 1;
				//arguments[i] = mbstows(argumentName);
			}

			uint16_t codeSize = readUInt16(data);
			data += sizeof(uint16_t);

			Ref< ActionFunction > function = new ActionFunction1(
				functionName,
				npc,
				codeSize,
				argumentCount,
				frame->getDictionary()
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

			npc += codeSize;
		VM_END()

		VM_BEGIN(AopSetRegister)
			uint8_t registerIndex = *data;
			data += sizeof(uint8_t);
			frame->setRegister(registerIndex, !stack.empty() ? stack.top() : ActionValue());
			VM_LOG(L"Set register " << int32_t(registerIndex) << L" = " << (!stack.empty() ? stack.top().getWideStringSafe() : L"< Stack empty >"));
		VM_END()
		}

		pc = npc;
	}

	VM_LOG(L"-- Execute end --");
}

	}
}
