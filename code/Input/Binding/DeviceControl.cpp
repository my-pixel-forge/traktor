#include "Input/IInputDevice.h"
#include "Input/Binding/DeviceControl.h"

namespace traktor
{
	namespace input
	{

std::wstring DeviceControl::getControlName() const
{
	if (m_device)
		return m_device->getControlName(m_control);
	else
		return L"";
}

float DeviceControl::getPreviousValue() const
{
	return m_previousValue;
}

float DeviceControl::getCurrentValue() const
{
	return m_currentValue;
}

DeviceControl::DeviceControl()
:	m_category(CtUnknown)
,	m_controlType(DtInvalid)
,	m_index(0)
,	m_control(0)
,	m_previousValue(0.0f)
,	m_currentValue(0.0f)
{
}

	}
}
