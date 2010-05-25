#include "Core/Log/Log.h"
#include "Core/Math/MathUtils.h"
#include "Flash/Action/ActionContext.h"
#include "Flash/Action/ActionFunctionNative.h"
#include "Flash/Action/Classes/Tween.h"
#include "Flash/Action/Avm1/Classes/As_mx_transitions_Tween.h"

namespace traktor
{
	namespace flash
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.flash.Tween", Tween, ActionObject)

Tween::Tween(
	ActionContext* context,
	ActionObject* target,
	const std::wstring& propertyName,
	ActionFunction* function,
	avm_number_t begin,
	avm_number_t finish,
	avm_number_t duration,
	bool useSeconds
)
:	ActionObject(As_mx_transitions_Tween::getInstance())
,	m_context(context)
,	m_target(target)
,	m_function(function)
,	m_begin(begin)
,	m_finish(finish)
,	m_duration(duration)
,	m_useSeconds(useSeconds)
,	m_timeStart(avm_number_t(-1))
,	m_current(begin)
,	m_playing(false)
{
	if (m_target)
		m_target->getPropertySet(propertyName, m_property);
	
	setMember(L"onFrame", ActionValue(createNativeFunction(this, &Tween::onFrame)));
	start();
}

Tween::~Tween()
{
	stop();
}

void Tween::continueTo(avm_number_t finish, avm_number_t duration)
{
	m_begin = m_current;
	m_finish = finish;
	m_duration = duration;
}

void Tween::fforward()
{
}

void Tween::nextFrame()
{
}

void Tween::prevFrame()
{
}

void Tween::resume()
{
	if (!m_playing)
		m_context->addFrameListener(this);

	m_playing = true;
}

void Tween::rewind(avm_number_t t)
{
	m_timeStart = avm_number_t(-1);
}

void Tween::start()
{
	m_timeStart = avm_number_t(-1);

	if (!m_playing)
	{
		// Ensure property is set to initial value.
		if (m_property && m_function)
		{
			ActionValue value;

			// Calculate eased value.
			if (m_duration > 0.0f)
			{
				ActionValueArray argv0(m_context->getPool(), 4);
				argv0[0] = ActionValue(avm_number_t(0));
				argv0[1] = ActionValue(m_begin);
				argv0[2] = ActionValue(m_finish - m_begin);
				argv0[3] = ActionValue(m_duration);
				value = m_function->call(m_context, this, argv0);
			}
			else
				value = ActionValue(m_begin);

			m_current = value.getNumberSafe();

			// Set property value.
			if (!value.isUndefined())
			{
				ActionValueArray argv1(m_context->getPool(), 1);
				argv1[0] = value;
				m_property->call(m_context, m_target, argv1);
			}
		}

		m_context->addFrameListener(this);
	}

	m_playing = true;
}

void Tween::stop()
{
	if (m_playing)
		m_context->removeFrameListener(this);

	m_playing = false;
}

void Tween::yoyo()
{
	std::swap(m_begin, m_finish);
	start();
}

void Tween::onFrame(CallArgs& ca)
{
	if (!m_function || !m_property || !m_playing || m_duration <= 0.0f)
		return;

	ActionValue value;

	avm_number_t time = ca.args[0].getNumberSafe();
	if (m_timeStart < 0)
		m_timeStart = time;

	// Calculate interpolated value.
	avm_number_t t = time - m_timeStart;
	avm_number_t T = clamp(t, avm_number_t(0), m_duration);

	// Calculate eased value.
	if (m_duration > 0.0f)
	{
		ActionValueArray argv0(ca.context->getPool(), 4);
		argv0[0] = ActionValue(T);
		argv0[1] = ActionValue(m_begin);
		argv0[2] = ActionValue(m_finish - m_begin);
		argv0[3] = ActionValue(m_duration);
		value = m_function->call(ca.context, this, argv0);
	}
	else
		value = ActionValue(m_begin);

	m_current = value.getNumberSafe();

	// Set property value.
	if (!value.isUndefined())
	{
		ActionValueArray argv1(ca.context->getPool(), 1);
		argv1[0] = value;
		m_property->call(ca.context, m_target, argv1);
	}

	// Stop after duration expired.
	if (t >= m_duration)
	{
		stop();

		// Notify listener when we've reached the end.
		ActionValue memberValue;
		if (getLocalMember(L"onMotionFinished", memberValue))
		{
			Ref< ActionFunction > motionFinished = memberValue.getObject< ActionFunction >();
			if (motionFinished)
				motionFinished->call(ca.context, this, ActionValueArray());
		}
	}
}

	}
}
