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

/*! \brief Control debugger on running target.
 * \ingroup Amalgam
 */
class T_DLLCLASS ScriptDebuggerControl : public ISerializable
{
	T_RTTI_CLASS;

public:
	enum Action
	{
		AcBreak,
		AcContinue,
		AcStepInto,
		AcStepOver,
		AcCapture
	};

	ScriptDebuggerControl();

	ScriptDebuggerControl(Action action);

	ScriptDebuggerControl(Action action, uint32_t param);

	Action getAction() const { return m_action; }

	uint32_t getParam() const { return m_param; }

	virtual void serialize(ISerializer& s);

private:
	Action m_action;
	uint32_t m_param;
};

	}
}

#endif	// traktor_amalgam_ScriptDebuggerControl_H
