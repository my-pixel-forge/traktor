/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_input_InputDriverDi8_H
#define traktor_input_InputDriverDi8_H

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include "Core/RefArray.h"
#include "Core/Misc/ComRef.h"
#include "Input/IInputDriver.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_INPUT_DI8_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{

struct SystemWindow;

	namespace input
	{

class T_DLLCLASS InputDriverDi8 : public IInputDriver
{
	T_RTTI_CLASS;

public:
	InputDriverDi8();

	virtual ~InputDriverDi8();

	void destroy();

	virtual bool create(const SystemApplication& sysapp, const SystemWindow& syswin, uint32_t inputCategories) override final;

	virtual int getDeviceCount() override final;

	virtual Ref< IInputDevice > getDevice(int index) override final;

	virtual UpdateResult update() override final;

private:
	static BOOL CALLBACK enumDevicesCallback(const DIDEVICEINSTANCE* deviceInstance, VOID* context);

	bool addDevice(const DIDEVICEINSTANCE* instance);

	HWND m_hWnd;
	uint32_t m_inputCategories;
	RefArray< IInputDevice > m_devices;
	ComRef< IDirectInput8 > m_directInput;
};

	}
}

#endif	// traktor_input_InputDriverDi8_H
