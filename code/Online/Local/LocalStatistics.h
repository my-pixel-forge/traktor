/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_online_LocalStatistics_H
#define traktor_online_LocalStatistics_H

#include "Core/Ref.h"
#include "Online/Provider/IStatisticsProvider.h"

namespace traktor
{
	namespace sql
	{

class IConnection;

	}

	namespace online
	{

class LocalStatistics : public IStatisticsProvider
{
	T_RTTI_CLASS;

public:
	LocalStatistics(sql::IConnection* db);

	virtual bool enumerate(std::map< std::wstring, int32_t >& outStats) override final;

	virtual bool set(const std::wstring& statId, int32_t value) override final;

private:
	Ref< sql::IConnection > m_db;
};

	}
}

#endif	// traktor_online_LocalStatistics_H
