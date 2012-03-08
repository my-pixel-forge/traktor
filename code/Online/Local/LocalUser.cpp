#include "Core/System/OS.h"
#include "Online/Local/LocalUser.h"

namespace traktor
{
	namespace online
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.online.LocalUser", LocalUser, IUserProvider)

bool LocalUser::getName(uint64_t userHandle, std::wstring& outName)
{
	outName = OS::getInstance().getCurrentUser();
	return true;
}

bool LocalUser::setPresenceValue(uint64_t userHandle, const std::wstring& key, const std::wstring& value)
{
	return false;
}

bool LocalUser::getPresenceValue(uint64_t userHandle, const std::wstring& key, std::wstring& outValue)
{
	return false;
}

bool LocalUser::sendP2PData(uint64_t userHandle, const void* data, size_t size, bool reliable)
{
	return false;
}

	}
}
