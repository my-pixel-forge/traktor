#ifndef traktor_input_InReadValue_H
#define traktor_input_InReadValue_H

#include "Input/Binding/IInputNode.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_INPUT_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace input
	{

class T_DLLCLASS InReadValue : public IInputNode
{
	T_RTTI_CLASS;
	
public:
	InReadValue();
	
	InReadValue(const std::wstring& valueId);

	virtual InputValue evaluate(const InputValueSet& valueSet, float T, float dT, float currentStateValue) const;
	
	virtual bool serialize(ISerializer& s);
	
private:
	std::wstring m_valueId;
};

	}
}

#endif	// traktor_input_InReadValue_H
