#pragma once

#include <functional>
#include "Core/Object.h"
#include "Core/Guid.h"

namespace traktor
{

class PropertyGroup;

	namespace net
	{

class BidirectionalObjectTransport;
class StreamServer;

	}

	namespace editor
	{

class PipelineDependency;

/*! \brief
 * \ingroup Editor
 */
class PipelineAgent : public Object
{
	T_RTTI_CLASS;

public:
	static Ref< PipelineAgent > create(
		net::StreamServer* streamServer,
		const Guid& sessionId,
		const PropertyGroup* settings,
		const std::wstring& description,
		const std::wstring& host,
		uint16_t port,
		uint16_t databasePort
	);

	bool build(const PipelineDependency* dependency, const std::function< void(bool) >& result);

	void update();

	bool isConnected() const;

	bool isIdle() const;

	net::BidirectionalObjectTransport* getTransport() const { return m_transport; }

	const std::wstring& getDescription() const { return m_description; }

private:
	Ref< net::StreamServer > m_streamServer;
	Ref< net::BidirectionalObjectTransport > m_transport;
	std::wstring m_description;
	uint32_t m_buildingCount;
	std::function< void(bool) > m_result;

	PipelineAgent(
		net::StreamServer* streamServer,
		net::BidirectionalObjectTransport* transport,
		const std::wstring& description
	);
};

	}
}

