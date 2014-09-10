#ifndef traktor_input_MouseDevicePNaCl_H
#define traktor_input_MouseDevicePNaCl_H

#include "Input/IInputDevice.h"

namespace traktor
{
	namespace input
	{

class MouseDevicePNaCl : public IInputDevice
{
	T_RTTI_CLASS;

public:
	MouseDevicePNaCl();

	virtual std::wstring getName() const;

	virtual InputCategory getCategory() const;

	virtual bool isConnected() const;

	virtual int32_t getControlCount();

	virtual std::wstring getControlName(int32_t control);

	virtual bool isControlAnalogue(int32_t control) const;

	virtual bool isControlStable(int32_t control) const;

	virtual float getControlValue(int32_t control);

	virtual bool getControlRange(int32_t control, float& outMin, float& outMax) const;

	virtual bool getDefaultControl(InputDefaultControlType controlType, bool analogue, int32_t& control) const;

	virtual bool getKeyEvent(KeyEvent& outEvent);

	virtual void resetState();

	virtual void readState();

	virtual bool supportRumble() const;

	virtual void setRumble(const InputRumble& rumble);

	virtual void setExclusive(bool exclusive);

private:
	friend class InputDriverPNaCl;

	float m_axisX;
	float m_axisY;
	float m_positionX;
	float m_positionY;
	float m_button1;
	float m_button2;
	float m_button3;

	void consumeEvent(const pp::InputEvent& event);
};

	}
}

#endif	// traktor_input_MouseDevicePNaCl_H
