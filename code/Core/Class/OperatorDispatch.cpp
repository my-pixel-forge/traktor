#include "Core/Class/Any.h"
#include "Core/Class/OperatorDispatch.h"

namespace traktor
{

T_IMPLEMENT_RTTI_CLASS(L"traktor.OperatorDispatch", OperatorDispatch, IRuntimeDispatch)

void OperatorDispatch::add(const IRuntimeDispatch* dispatch)
{
	m_dispatches.push_back(dispatch);
}

std::wstring OperatorDispatch::signature() const
{
	return L"";
}

Any OperatorDispatch::invoke(ITypedObject* self, uint32_t argc, const Any* argv) const
{
	Any result;
	for (auto dispatch : m_dispatches)
	{
		result = dispatch->invoke(self, argc, argv);
		if (!result.isVoid())
			break;
	}
	return result;
}

}
