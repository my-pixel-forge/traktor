#ifndef traktor_amalgam_ScriptDebuggerControl_H
#define traktor_amalgam_ScriptDebuggerControl_H

#include "Core/Serialization/ISerializable.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_AMALGAM_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace amalgam
	{

class T_DLLCLASS ScriptDebuggerControl : public ISerializable
{
	T_RTTI_CLASS;

public:
	enum Action
	{
		AcBreak,
		AcContinue,
		AcStepInto,
		AcStepOver
	};

	ScriptDebuggerControl();

	ScriptDebuggerControl(Action action);

	Action getAction() const { return m_action; }

	virtual bool serialize(ISerializer& s);

private:
	Action m_action;
};

	}
}

#endif	// traktor_amalgam_ScriptDebuggerControl_H
