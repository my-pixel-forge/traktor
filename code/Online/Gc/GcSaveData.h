#ifndef traktor_online_GcSaveData_H
#define traktor_online_GcSaveData_H

#include "Online/Provider/ISaveDataProvider.h"

namespace traktor
{
	namespace online
	{

class GcSaveData : public ISaveDataProvider
{
	T_RTTI_CLASS;

public:
	GcSaveData();

	virtual bool enumerate(std::set< std::wstring >& outSaveDataIds);

	virtual bool get(const std::wstring& saveDataId, Ref< ISerializable >& outAttachment);

	virtual bool set(const std::wstring& saveDataId, const SaveDataDesc& saveDataDesc, const ISerializable* attachment, bool replace);
};

	}
}

#endif	// traktor_online_GcSaveData_H
