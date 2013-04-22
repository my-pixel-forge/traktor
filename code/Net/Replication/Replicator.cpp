#include <cstring>
#include "Core/Io/MemoryStream.h"
#include "Core/Log/Log.h"
#include "Core/Math/Const.h"
#include "Core/Math/Float.h"
#include "Core/Math/Random.h"
#include "Core/Memory/IAllocator.h"
#include "Core/Memory/MemoryConfig.h"
#include "Core/Misc/SafeDestroy.h"
#include "Core/Thread/ThreadManager.h"
#include "Core/Thread/Thread.h"
#include "Core/Timer/Timer.h"
#include "Net/Replication/CompactSerializer.h"
#include "Net/Replication/IReplicatorPeers.h"
#include "Net/Replication/Message.h"
#include "Net/Replication/Replicator.h"
#include "Net/Replication/State/StateTemplate.h"

namespace traktor
{
	namespace net
	{
		namespace
		{

const handle_t c_broadcastHandle = 0UL;
const float c_initialTimeOffset = 0.05f;
const float c_nearDistance = 14.0f;
const float c_farDistance = 150.0f;
const float c_nearTimeUntilTx = 1.0f / 16.0f;
const float c_farTimeUntilTx = 1.0f / 9.0f;
const float c_timeUntilIAm = 3.0f;
const float c_timeUntilPing = 1.5f;
const float c_errorStateThreshold = 0.2f;
const float c_remoteOffsetThreshold = 0.1f;
const float c_remoteOffsetLimit = 0.05f;
const uint32_t c_maxPendingPing = 4;
const uint32_t c_maxErrorCount = 2;
const uint32_t c_maxDeltaStates = 8;

Timer g_timer;
Random g_random;

#define T_USE_DELTA_FRAMES 0
#define T_REPLICATOR_DEBUG(x) traktor::log::info << x << traktor::Endl

		}

T_IMPLEMENT_RTTI_CLASS(L"traktor.net.Replicator", Replicator, Object)

T_IMPLEMENT_RTTI_CLASS(L"traktor.net.Replicator.IListener", Replicator::IListener, Object)

Replicator::Replicator()
:	m_id(0)
,	m_origin(0.0f, 0.0f, 0.0f, 1.0f)
,	m_time0(0.0f)
,	m_time(0.0f)
,	m_pingCount(0)
,	m_timeUntilPing(0.0f)
{
	m_id = uint32_t(g_timer.getElapsedTime());
}

Replicator::~Replicator()
{
	destroy();
}

bool Replicator::create(IReplicatorPeers* replicatorPeers)
{
	std::vector< handle_t > handles;

	m_replicatorPeers = replicatorPeers;
	m_state = 0;

	m_replicatorPeers->update();
	m_replicatorPeers->getPeerHandles(handles);

	// Discard any pending data.
	for (;;)
	{
		Message discard;
		handle_t fromHandle;

		if (receive(&discard, fromHandle) <= 0)
			break;
	}

	// Create non-established entries for each peer.
	for (std::vector< handle_t >::const_iterator i = handles.begin(); i != handles.end(); ++i)
	{
		Peer& peer = m_peers[*i];
		peer.state = PsInitial;
		peer.global = m_replicatorPeers->getPeerGlobalId(*i);
		peer.precursor = true;
	}

	return true;
}

void Replicator::destroy()
{
	if (!m_peers.empty())
	{
		Message discard;
		handle_t fromHandle;

		// Send Bye message to all peers.
		for (std::map< handle_t, Peer >::iterator i = m_peers.begin(); i != m_peers.end(); ++i)
			sendBye(i->first);

		// Massage transportation to ensure all messages have been sent.
		while (m_replicatorPeers)
		{
			receive(&discard, fromHandle);
			if (m_replicatorPeers->update() <= 0)
				break;
		}

		// Delete all peer control.
		for (std::map< handle_t, Peer >::iterator i = m_peers.begin(); i != m_peers.end(); ++i)
		{
			if (i->second.ghost)
			{
				i->second.ghost->~Ghost();
				getAllocator()->free(i->second.ghost);
				i->second.ghost = 0;
			}
		}
	}

	m_peers.clear();
	m_eventsIn.clear();
	m_eventsOut.clear();
	m_listeners.clear();
	m_eventTypes.clear();

	m_state = 0;
	m_replicatorPeers = 0;
}

void Replicator::addEventType(const TypeInfo& eventType)
{
	m_eventTypes.push_back(&eventType);
}

void Replicator::addListener(IListener* listener)
{
	m_listeners.push_back(listener);
}

bool Replicator::update(float T, float dT)
{
	if (!m_replicatorPeers)
		return false;

	double T_0 = g_timer.getElapsedTime();

	updatePeers(dT);

	double T_1 = g_timer.getElapsedTime();

	if (!m_replicatorPeers)
		return false;

	sendState(dT);

	double T_2 = g_timer.getElapsedTime();

	sendEvents();

	double T_3 = g_timer.getElapsedTime();

	sendPings(dT);

	double T_4 = g_timer.getElapsedTime();

	receiveMessages();

	double T_5 = g_timer.getElapsedTime();

	dispatchEventListeners();

	double T_6 = g_timer.getElapsedTime();

	if (T_6 - T_0 > 1.0)
	{
		log::debug << L"updatePeers " << int32_t((T_1 - T_0) * 1000.0) << L" ms" << Endl;
		log::debug << L"sendState " << int32_t((T_2 - T_1) * 1000.0) << L" ms" << Endl;
		log::debug << L"sendEvents " << int32_t((T_3 - T_2) * 1000.0) << L" ms" << Endl;
		log::debug << L"sendPings " << int32_t((T_4 - T_3) * 1000.0) << L" ms" << Endl;
		log::debug << L"receiveMessages " << int32_t((T_5 - T_4) * 1000.0) << L" ms" << Endl;
		log::debug << L"dispatchEventListeners " << int32_t((T_6 - T_5) * 1000.0) << L" ms" << Endl;
	}

	m_time0 += dT;
	m_time += dT;

	return bool(m_replicatorPeers != 0);
}

void Replicator::setOrigin(const Vector4& origin)
{
	m_origin = origin;
}

void Replicator::setStateTemplate(const StateTemplate* stateTemplate)
{
	// Replace template.
	m_stateTemplate = stateTemplate;

	// Nuke states from previous template.
	m_state = 0;
	for (std::map< handle_t, Peer >::iterator i = m_peers.begin(); i != m_peers.end(); ++i)
		i->second.iframe = 0;
}

void Replicator::setState(const State* state)
{
	m_state = state;
}

void Replicator::sendEvent(handle_t peerHandle, const ISerializable* eventObject)
{
	Event e;
	e.time = 0.0f;
	e.eventId = 0;
	e.handle = peerHandle;
	e.object = eventObject;
	m_eventsOut.push_back(e);
}

void Replicator::broadcastEvent(const ISerializable* eventObject)
{
	Event e;
	e.time = 0.0f;
	e.eventId = 0;
	e.handle = c_broadcastHandle;
	e.object = eventObject;
	m_eventsOut.push_back(e);
}

bool Replicator::isPrimary() const
{
	return m_replicatorPeers->getPrimaryPeerHandle() == m_replicatorPeers->getGlobalId();
}

uint32_t Replicator::getPeerCount() const
{
	return uint32_t(m_peers.size());
}

handle_t Replicator::getPeerHandle(uint32_t peerIndex) const
{
	std::map< handle_t, Peer >::const_iterator it = m_peers.begin();
	std::advance(it, peerIndex);
	return it->first;
}

std::wstring Replicator::getPeerName(handle_t peerHandle) const
{
	return m_replicatorPeers->getPeerName(peerHandle);
}

int32_t Replicator::getPeerLatency(handle_t peerHandle) const
{
	std::map< handle_t, Peer >::const_iterator i = m_peers.find(peerHandle);
	return i != m_peers.end() ? int32_t(i->second.latencyMinimum * 1000.0f) : 0;
}

int32_t Replicator::getPeerReversedLatency(handle_t peerHandle) const
{
	std::map< handle_t, Peer >::const_iterator i = m_peers.find(peerHandle);
	return i != m_peers.end() ? int32_t(i->second.latencyReversed * 1000.0f) : 0;
}

int32_t Replicator::getBestReversedLatency() const
{
	int32_t latencyBest = 0;
	for (std::map< handle_t, Peer >::const_iterator i = m_peers.begin(); i != m_peers.end(); ++i)
	{
		if (latencyBest > 0)
			latencyBest = std::min(latencyBest, int32_t(i->second.latencyReversed * 1000.0f));
		else
			latencyBest = int32_t(i->second.latencyReversed * 1000.0f);
	}
	return latencyBest;
}

int32_t Replicator::getWorstReversedLatency() const
{
	int32_t latencyWorst = 0;
	for (std::map< handle_t, Peer >::const_iterator i = m_peers.begin(); i != m_peers.end(); ++i)
		latencyWorst = std::max(latencyWorst, int32_t(i->second.latencyReversed * 1000.0f));
	return latencyWorst;
}

bool Replicator::isPeerConnected(handle_t peerHandle) const
{
	std::map< handle_t, Peer >::const_iterator i = m_peers.find(peerHandle);
	if (i == m_peers.end() || i->second.state != PsEstablished)
		return false;
	else
		return true;
}

bool Replicator::isPeerRelayed(handle_t peerHandle) const
{
	std::map< handle_t, Peer >::const_iterator i = m_peers.find(peerHandle);
	if (i == m_peers.end() || i->second.state != PsEstablished)
		return false;
	else
		return i->second.relay;
}

handle_t Replicator::getPrimaryPeerHandle() const
{
	for (std::map< handle_t, Peer >::const_iterator i = m_peers.begin(); i != m_peers.end(); ++i)
	{
		if (isPeerPrimary(i->first))
			return i->first;
	}
	return 0;
}

bool Replicator::isPeerPrimary(handle_t peerHandle) const
{
	return m_replicatorPeers->getPrimaryPeerHandle() == peerHandle;
}

bool Replicator::areAllPeersConnected() const
{
	for (std::map< handle_t, Peer >::const_iterator i = m_peers.begin(); i != m_peers.end(); ++i)
	{
		if (i->second.state != PsEstablished)
			return false;
	}
	return true;
}

void Replicator::setGhostObject(handle_t peerHandle, Object* ghostObject)
{
	std::map< handle_t, Peer >::iterator i = m_peers.find(peerHandle);
	if (i != m_peers.end() && i->second.ghost)
		i->second.ghost->object = ghostObject;
	else
		T_REPLICATOR_DEBUG(L"ERROR: Trying to set ghost object of unknown peer handle " << peerHandle);
}

Object* Replicator::getGhostObject(handle_t peerHandle) const
{
	std::map< handle_t, Peer >::const_iterator i = m_peers.find(peerHandle);
	if (i != m_peers.end() && i->second.ghost)
		return i->second.ghost->object;
	else
	{
		T_REPLICATOR_DEBUG(L"ERROR: Trying to get ghost object of unknown peer handle " << peerHandle);
		return 0;
	}
}

void Replicator::setGhostOrigin(handle_t peerHandle, const Vector4& origin)
{
	std::map< handle_t, Peer >::iterator i = m_peers.find(peerHandle);
	if (i != m_peers.end() && i->second.ghost)
		i->second.ghost->origin = origin;
	else
		T_REPLICATOR_DEBUG(L"ERROR: Trying to set ghost origin of unknown peer handle " << peerHandle);
}

void Replicator::setGhostStateTemplate(handle_t peerHandle, const StateTemplate* stateTemplate)
{
	std::map< handle_t, Peer >::iterator i = m_peers.find(peerHandle);
	if (i != m_peers.end() && i->second.ghost)
	{
		i->second.ghost->stateTemplate = stateTemplate;
		i->second.ghost->Sn2 = 0;
		i->second.ghost->Sn1 = 0;
		i->second.ghost->S0 = 0;
		i->second.ghost->Tn2 = 0.0f;
		i->second.ghost->Tn1 = 0.0f;
		i->second.ghost->T0 = 0.0f;
	}
	else
		T_REPLICATOR_DEBUG(L"ERROR: Trying to get ghost state of unknown peer handle " << peerHandle);
}

Ref< const State > Replicator::getGhostState(handle_t peerHandle, const State* currentState) const
{
	std::map< handle_t, Peer >::const_iterator i = m_peers.find(peerHandle);
	if (i != m_peers.end() && i->second.ghost)
	{
		const StateTemplate* stateTemplate = i->second.ghost->stateTemplate;
		if (stateTemplate)
			return stateTemplate->extrapolate(
				i->second.ghost->Sn2,
				i->second.ghost->Tn2,
				i->second.ghost->Sn1,
				i->second.ghost->Tn1,
				i->second.ghost->S0,
				i->second.ghost->T0,
				currentState,
				m_time
			);
		else
			return i->second.ghost->S0;
	}
	else
	{
		T_REPLICATOR_DEBUG(L"ERROR: Trying to get ghost state of unknown peer handle " << peerHandle);
		return currentState;
	}
}

Ref< const State > Replicator::getLoopBackState() const
{
	if (m_stateTemplate)
	{
		uint8_t data[Message::MessageSize];
		uint32_t size = m_stateTemplate->pack(m_state, data, sizeof(data));
		return m_stateTemplate->unpack(data, size);
	}
	else
		return 0;
}

void Replicator::updatePeers(float dT)
{
	std::vector< handle_t > handles;
	handles.reserve(m_peers.size());

	// Massage replicator peers back-end first and
	// then get fresh list of peer handles.
	m_replicatorPeers->update();
	m_replicatorPeers->getPeerHandles(handles);

	// Keep list of un-fresh handles.
	for (std::map< handle_t, Peer >::iterator i = m_peers.begin(); i != m_peers.end(); )
	{
		if (std::find(handles.begin(), handles.end(), i->first) != handles.end())
		{
			++i;
			continue;
		}

		Peer& peer = i->second;
		if (peer.state == PsEstablished)
		{
			T_REPLICATOR_DEBUG(L"WARNING: Peer " << i->first << L" connection suddenly terminated");

			// Need to notify listeners immediately as peer becomes dismounted.
			for (RefArray< IListener >::iterator j = m_listeners.begin(); j != m_listeners.end(); ++j)
				(*j)->notify(this, 0, IListener::ReDisconnected, i->first, 0);

			if (peer.ghost)
			{
				peer.ghost->~Ghost(); getAllocator()->free(peer.ghost);
				peer.ghost = 0;
			}

			peer.state = PsDisconnected;
			peer.iframe = 0;

			++i;
		}
		else
			m_peers.erase(i++);
	}

	// Iterate all handles, check error state or send "I am" to new peers.
	for (std::vector< handle_t >::const_iterator i = handles.begin(); i != handles.end(); ++i)
	{
		Peer& peer = m_peers[*i];

		// Issue "I am" to unestablished peers.
		if (peer.state == PsInitial)
		{
			peer.global = m_replicatorPeers->getPeerGlobalId(*i);
			if ((peer.timeUntilTx -= dT) <= 0.0f)
			{
				T_REPLICATOR_DEBUG(L"OK: Unestablished peer found; sending \"I am\" to peer " << *i);
				
				if (peer.pendingIAm >= 2)
				{
					peer.relay = !peer.relay;
					peer.pendingIAm = 0;

					if (peer.relay)
						{ T_REPLICATOR_DEBUG(L"WARNING: Unable to handshake directly with peer " << *i << L"; relaying through other peer(s)"); }
					else
						{ T_REPLICATOR_DEBUG(L"WARNING: Unable to handshake with peer " << *i << L" through relay; using direct"); }
				}
				
				if (sendIAm(*i, 0, m_id))
					peer.pendingIAm++;

				peer.timeUntilTx = c_timeUntilIAm * (1.0f + g_random.nextFloat());
			}
		}

		// Check if peer doesn't respond, timeout;ed or unable to communicate.
		else if (peer.state == PsEstablished)
		{
			bool failing = false;

			if (peer.pendingPing >= c_maxPendingPing)
			{
				T_REPLICATOR_DEBUG(L"WARNING: Peer " << *i << L" doesn't respond to ping");
				failing = true;
			}

			if (peer.errorCount >= c_maxErrorCount)
			{
				T_REPLICATOR_DEBUG(L"WARNING: Peer " << *i << L" failing, unable to communicate with peer");
				failing = true;
			}

			if (failing)
			{
				peer.relay = !peer.relay;
				peer.pendingPing = 0;
				peer.errorCount = 0;

				if (peer.relay)
					{ T_REPLICATOR_DEBUG(L"WARNING: Unable to communcate directly with peer " << *i << L"; relaying through other peer(s)"); }
				else
					{ T_REPLICATOR_DEBUG(L"WARNING: Unable to communcate with peer " << *i << L" through relay; using direct"); }
			}
		}
	}
}

void Replicator::sendState(float dT)
{
	uint8_t iframeData[Message::DataSize];
#if T_USE_DELTA_FRAMES
	uint8_t frameData[Message::DataSize];
#endif
	Message msg;

	// Broadcast my state to all peers.
	if (!m_state || !m_stateTemplate)
		return;

	// Pack iframe from current state.
	uint32_t iframeSize = m_stateTemplate->pack(
		m_state,
		iframeData,
		sizeof(iframeData)
	);

	// Send state to all connected peers.
	msg.time = uint32_t(m_time * 1000.0f);
	for (std::map< handle_t, Peer >::iterator i = m_peers.begin(); i != m_peers.end(); ++i)
	{
		Peer& peer = i->second;

		if (peer.state != PsEstablished || !peer.ghost)
			continue;
		if ((peer.timeUntilTx -= dT) > 0.0f)
			continue;

		uint32_t msgSize = Message::HeaderSize + sizeof(uint8_t);

#if T_USE_DELTA_FRAMES
		// Send delta frames only if we've successfully sent
		// an iframe and we're not experiencing package loss.
		if (
			peer.iframe &&
			peer.packetCount > 0 &&
			peer.stateCount % c_maxDeltaStates > 0
		)
		{
			// Pack delta frame from last sent state.
			uint32_t frameSize = m_stateTemplate->pack(
				peer.iframe,
				m_state,
				frameData,
				sizeof(frameData)
			);

			if (frameSize < iframeSize)
			{
				std::memcpy(msg.state.data, frameData, frameSize);
				msgSize += frameSize;
				msg.type = MtDeltaState;
			}
			else
			{
				std::memcpy(msg.state.data, iframeData, iframeSize);
				msgSize += iframeSize;
				msg.type = MtFullState;
				peer.stateCount = 0;
			}
		}
		else
#endif
		{
			std::memcpy(msg.state.data, iframeData, iframeSize);
			msgSize += iframeSize;
			msg.type = MtFullState;
			peer.stateCount = 0;
		}

		if (send(i->first, &msg, msgSize, false))
		{
			if (peer.ghost->stateTemplate)
			{
				float distanceToPeer = (peer.ghost->origin - m_origin).xyz0().length();
				float t = clamp((distanceToPeer - c_nearDistance) / (c_farDistance - c_nearDistance), 0.0f, 1.0f);
				peer.timeUntilTx = lerp(c_nearTimeUntilTx, c_farTimeUntilTx, t);
			}
			else
				peer.timeUntilTx = c_farTimeUntilTx;

			peer.errorCount = 0;
			peer.stateCount++;
			peer.iframe = m_state;
		}
		else
		{
			log::error << L"ERROR: Unable to send state to peer " << i->first << L" (" << peer.errorCount << L")" << Endl;
			peer.timeUntilTx = c_farTimeUntilTx;
			peer.errorCount++;
			peer.stateCount = 0;
			peer.iframe = 0;
		}
	}
}

void Replicator::sendEvents()
{
	std::list< Event > eventsOut;
	Message msg;

	if (m_eventsOut.empty())
		return;

	for (std::map< handle_t, Peer >::iterator i = m_peers.begin(); i != m_peers.end(); ++i)
	{
		std::vector< Event > peerEventsOut;

		// Collect events to peer.
		for (std::list< Event >::const_iterator j = m_eventsOut.begin(); j != m_eventsOut.end(); ++j)
		{
			if (j->handle == c_broadcastHandle || j->handle == i->first)
			{
				peerEventsOut.push_back(*j);
				peerEventsOut.back().handle = i->first;
			}
		}

		if (peerEventsOut.empty())
			continue;

		// Pack as many events into same message as possible.
		for (uint32_t j = 0; j < peerEventsOut.size(); )
		{
			uint32_t count = peerEventsOut.size() - j;
			T_ASSERT (count > 0);

			msg.type = MtEvent0;
			msg.time = uint32_t(m_time * 1000.0f);

			uint32_t totalPayLoadSize = 0;

			uint8_t data[sizeof(msg.event.data) + 1];	// Add a padding to be able to detect if a single object exceed max size.
			uint8_t* msgDataPtr = msg.event.data;
			uint8_t* msgDataEndPtr = msgDataPtr + sizeof(msg.event.data);

			uint32_t jj = j;
			while (jj < peerEventsOut.size() && msg.type < MtEvent4)
			{
				MemoryStream s(data, sizeof(data), false, true);
				CompactSerializer cs(&s, &m_eventTypes[0]);
				cs.writeObject(peerEventsOut[jj].object);
				cs.flush();

				uint32_t dataSize = s.tell();
				if (dataSize > uint32_t(msgDataEndPtr - msgDataPtr))
					break;

				std::memcpy(msgDataPtr, data, dataSize);
				msgDataPtr += dataSize;

				++jj;
				++msg.type;
			}

			uint32_t msgSize = Message::HeaderSize + uint32_t(msgDataPtr - msg.event.data);

			if (send(i->first, &msg, msgSize, true))
			{
				i->second.errorCount = 0;
				j = jj;
			}
			else
			{
				log::error << L"ERROR: Unable to send event(s) to peer " << i->first << L" (" << i->second.errorCount << L")" << Endl;
				
				for (; j < jj; ++j)
					eventsOut.push_back(peerEventsOut[j]);

				i->second.errorCount++;
				break;
			}
		}
	}

	m_eventsOut.swap(eventsOut);
}

void Replicator::sendPings(float dT)
{
	m_timeUntilPing -= dT;
	if (m_timeUntilPing > 0.0f)
		return;

	if (!m_peers.empty())
	{
		std::map< handle_t, Peer >::iterator i = m_peers.begin();

		// Ping one peer at a time.
		m_pingCount = (m_pingCount + 1) % m_peers.size();
		std::advance(i, m_pingCount);

		Peer& peer = i->second;
		if (peer.state == PsEstablished)
		{
			sendPing(i->first);
			++peer.pendingPing;
		}

		m_timeUntilPing = c_timeUntilPing / m_peers.size();
	}
	else
		m_timeUntilPing = c_timeUntilPing;
}

void Replicator::receiveMessages()
{
	handle_t handle;
	Message msg;
	int32_t size;

	// Read messages from any peer.
	while (m_replicatorPeers)
	{
		std::memset(&msg, 0, sizeof(msg));
		if ((size = receive(&msg, handle)) <= 0)
			break;

		// Convert time from ms to seconds.
		float time = msg.time / 1000.0f;

		// Always handle handshake messages.
		if (msg.type == MtIAm)
		{
			// Assume peer time is correct if exceeding my time.
			float offset = time + c_initialTimeOffset - m_time;
			if (offset > 0.0f)
				adjustTime(offset);

			if (msg.iam.sequence == 0)
			{
				sendIAm(handle, 1, msg.iam.id);
			}
			else if (msg.iam.sequence == 1 || msg.iam.sequence == 2)
			{
				// "I am" with sequence 1 can only be received if I was the handshake initiator.
				// "I am" with sequence 2 can only be received if I was NOT the handshake initiator.

				if (msg.iam.sequence == 1)
				{
					if (msg.iam.id != m_id)
					{
						T_REPLICATOR_DEBUG(L"ERROR: \"I am\" message with incorrect id from peer " << handle << L"; ignoring");
						continue;
					}
					sendIAm(handle, 2, msg.iam.id);
				}

				Peer& peer = m_peers[handle];

				if (!peer.ghost)
				{
					// Create ghost data.
					void* ghostMem = getAllocator()->alloc(sizeof(Ghost), 16, "Ghost");

					peer.ghost = new (ghostMem) Ghost();
					peer.ghost->origin = m_origin;
					peer.ghost->Tn2 = 0.0f;
					peer.ghost->Tn1 = 0.0f;
					peer.ghost->T0 = 0.0f;
				}

				if (peer.state != PsEstablished)
				{
					peer.state = PsEstablished;
					peer.timeUntilTx = 0.0f;

					// Send ping to peer.
					sendPing(handle);
					++peer.pendingPing;

					// Issue connect event to listeners.
					Event evt;
					evt.eventId = IListener::ReConnected;
					evt.handle = handle;
					evt.object = 0;
					m_eventsIn.push_back(evt);

					T_REPLICATOR_DEBUG(L"OK: Peer " << handle << L" connection established");
				}

				peer.lastTimeRemote = std::max(time, peer.lastTimeRemote + 1e-4f);
				peer.lastTimeLocal = m_time;

				--peer.pendingIAm;
			}
		}
		else if (msg.type == MtBye)
		{
			T_REPLICATOR_DEBUG(L"OK: Got \"Bye\" from peer " << handle);

			Peer& peer = m_peers[handle];

			if (
				peer.state == PsEstablished &&
				peer.ghost
			)
			{
				T_REPLICATOR_DEBUG(L"OK: Established peer " << handle << L" gracefully disconnected; issue listener event");

				// Need to notify listeners immediately as peer becomes dismounted.
				for (RefArray< IListener >::iterator i = m_listeners.begin(); i != m_listeners.end(); ++i)
					(*i)->notify(this, 0, IListener::ReDisconnected, handle, 0);
			}

			if (peer.ghost)
			{
				peer.ghost->~Ghost(); getAllocator()->free(peer.ghost);
				peer.ghost = 0;
			}

			peer.state = PsDisconnected;
			peer.iframe = 0;

			peer.lastTimeRemote = std::max(time, peer.lastTimeRemote + 1e-4f);
			peer.lastTimeLocal = m_time;
		}
		else if (msg.type == MtPing)
		{
			// I've got pinged; reply with a pong.
			sendPong(handle, msg.time);
		}
		else if (msg.type == MtPong)
		{
			// I've received a pong reply from an earlier ping;
			// calculate round-trip time.
			Peer& peer = m_peers[handle];

			float pingTime = float(msg.pong.time0 / 1000.0f);
			float pongTime = float(msg.time / 1000.0f);
			float roundTrip = max(m_time0 - pingTime, 0.0f);

			peer.roundTrips.push_back(roundTrip);

			// Get lowest and median round-trips and calculate latencies.
			float minrt = roundTrip;
			float sorted[MaxRoundTrips];
			for (uint32_t i = 0; i < peer.roundTrips.size(); ++i)
			{
				sorted[i] = peer.roundTrips[i];
				minrt = min(minrt, peer.roundTrips[i]);
			}

			std::sort(&sorted[0], &sorted[peer.roundTrips.size()]);

			peer.latencyMedian = sorted[peer.roundTrips.size() / 2] / 2.0f;
			peer.latencyMinimum = minrt / 2.0f;
			peer.latencyReversed = float(msg.pong.latency / 1000.0f);

			if (peer.pendingPing > 0)
				--peer.pendingPing;
		}
		else if (msg.type == MtFullState || msg.type == MtDeltaState)	// Data message.
		{
			Peer& peer = m_peers[handle];
			if (!peer.ghost)
				continue;

			bool stateValid = false;

			// Ignore old messages; as we're using unreliable transportation
			// messages can arrive out-of-order.
			if (time > peer.lastTimeRemote + 1e-4f)
			{
				// Check network time.
				if (m_replicatorPeers->getPrimaryPeerHandle() == handle)
				{
					float offset = time + peer.latencyReversed - m_time;
					float k = clamp((abs(offset) - 0.05f) / (1.0f - 0.05f), 0.0f, 1.0f);
					float adjust = offset * lerp(0.1f, 1.0f, k);
					if (abs(adjust) > FUZZY_EPSILON)
						adjustTime(adjust);
				}

				if (peer.ghost->stateTemplate)
				{
					Ref< const State > state;

					if (msg.type == MtFullState)
						state = peer.ghost->stateTemplate->unpack(msg.state.data, sizeof(msg.state.data));
					else if (peer.ghost->S0)
						state = peer.ghost->stateTemplate->unpack(peer.ghost->S0, msg.state.data, sizeof(msg.state.data));

					if (state)
					{
						float localOffset = m_time - peer.lastTimeLocal;
						float remoteOffset = time - peer.lastTimeRemote;

						float offset = 0.0f;
						if (localOffset > 0 && remoteOffset > 0.0f)
						{
							offset = std::max(remoteOffset - localOffset - c_remoteOffsetThreshold, 0.0f);
							offset = std::min(offset, c_remoteOffsetLimit);
						}

						peer.ghost->Sn2 = peer.ghost->Sn1;
						peer.ghost->Tn2 = peer.ghost->Tn1 + offset;
						peer.ghost->Sn1 = peer.ghost->S0;
						peer.ghost->Tn1 = peer.ghost->T0 + offset;
						peer.ghost->S0 = state;
						peer.ghost->T0 = time + offset;

						stateValid = true;
					}
				}

				peer.packetCount++;
				peer.lastTimeLocal = m_time;
				peer.lastTimeRemote = time;
			}
			else
			{
				// Received an old out-of-order package.
				if (peer.ghost->stateTemplate)
				{
					Ref< const State > state;

					if (msg.type == MtFullState)
						state = peer.ghost->stateTemplate->unpack(msg.state.data, sizeof(msg.state.data));
					else
					{
						Ref< const State > Sn;

						if (time > peer.ghost->Tn1)
							Sn = peer.ghost->Sn1;
						else if (time > peer.ghost->Tn2)
							Sn = peer.ghost->Sn2;

						if (Sn)
							state = peer.ghost->stateTemplate->unpack(Sn, msg.state.data, sizeof(msg.state.data));
						else
							T_REPLICATOR_DEBUG(L"ERROR: Received delta state from peer " << handle << L" but have no iframe; state ignored (2)");
					}

					if (state)
					{
						if (time > peer.ghost->Tn1)
						{
							peer.ghost->Sn2 = peer.ghost->Sn1;
							peer.ghost->Tn2 = peer.ghost->Tn1;
							peer.ghost->Sn1 = state;
							peer.ghost->Tn1 = time;
							stateValid = true;
						}
						else if (time > peer.ghost->Tn2)
						{
							peer.ghost->Sn2 = state;
							peer.ghost->Tn2 = time;
							stateValid = true;
						}
					}
					else
						T_REPLICATOR_DEBUG(L"ERROR: Unable to unpack ghost state (2)");
				}

				peer.packetCount++;
			}

			// Put an input event to notify listeners about new state.
			if (stateValid)
			{
				Event evt;
				evt.eventId = IListener::ReState;
				evt.handle = handle;
				evt.object = 0;
				m_eventsIn.push_back(evt);
			}
		}
		else if (msg.type >= MtEvent1 && msg.type <= MtEvent4)	// Event message(s).
		{
			Peer& peer = m_peers[handle];
			if (!peer.ghost)
			{
				T_REPLICATOR_DEBUG(L"ERROR: Peer " << handle << L" partially connected but received MtEvent; ignoring");
				continue;
			}

			peer.packetCount++;
			peer.lastTimeLocal = m_time;
			peer.lastTimeRemote = time;

			if (!m_eventTypes.empty())
			{
				const uint8_t* msgDataPtr = msg.event.data;
				const uint8_t* msgDataEndPtr = msgDataPtr + sizeof(msg.event.data);

				uint32_t eventObjectCount = uint32_t(msg.type - MtEvent0);
				for (uint32_t i = 0; i < eventObjectCount; ++i)
				{
					MemoryStream s(msgDataPtr, uint32_t(msgDataEndPtr - msgDataPtr));
					Ref< ISerializable > eventObject = CompactSerializer(&s, &m_eventTypes[0]).readObject< ISerializable >();
					if (eventObject)
					{
						Event e;
						e.time = time;
						e.eventId = IListener::ReBroadcastEvent;
						e.handle = handle;
						e.object = eventObject;
						m_eventsIn.push_back(e);
					}
					msgDataPtr += s.tell();
				}
			}
		}
	}
}

void Replicator::dispatchEventListeners()
{
	for (std::list< Event >::const_iterator i = m_eventsIn.begin(); i != m_eventsIn.end(); ++i)
	{
		const Event& event = *i;
		for (RefArray< IListener >::iterator j = m_listeners.begin(); j != m_listeners.end(); ++j)
			(*j)->notify(this, event.time, event.eventId, event.handle, event.object);
	}
	m_eventsIn.clear();
}

bool Replicator::sendIAm(handle_t peerHandle, uint8_t sequence, uint32_t id)
{
	Message msg;

	msg.type = MtIAm;
	msg.time = uint32_t(m_time * 1000.0f);
	msg.iam.sequence = sequence;
	msg.iam.id = id;

	uint32_t msgSize = sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t);
	return send(peerHandle, &msg, msgSize, false);
}

bool Replicator::sendBye(handle_t peerHandle)
{
	Message msg;

	msg.type = MtBye;
	msg.time = uint32_t(m_time * 1000.0f);

	uint32_t msgSize = sizeof(uint8_t) + sizeof(uint32_t);
	return send(peerHandle, &msg, msgSize, true);
}

bool Replicator::sendPing(handle_t peerHandle)
{
	Message msg;

	msg.type = MtPing;
	msg.time = uint32_t(m_time0 * 1000.0f);

	uint32_t msgSize = sizeof(uint8_t) + sizeof(uint32_t);
	return send(peerHandle, &msg, msgSize, false);
}

bool Replicator::sendPong(handle_t peerHandle, uint32_t time0)
{
	std::map< handle_t, Peer >::const_iterator i = m_peers.find(peerHandle);
	Message msg;

	msg.type = MtPong;
	msg.time = uint32_t(m_time0 * 1000.0f);
	msg.pong.time0 = time0;
	msg.pong.latency = (i != m_peers.end()) ? uint32_t(i->second.latencyMedian * 1000.0f) : 0;	// Report back my perception of latency to this peer.

	uint32_t msgSize = sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t);
	return send(peerHandle, &msg, msgSize, false);
}

void Replicator::adjustTime(float offset)
{
	m_time += offset;

	// Also adjust all old states as well.
	for (std::map< handle_t, Peer >::iterator i = m_peers.begin(); i != m_peers.end(); ++i)
	{
		Ghost* ghost = i->second.ghost;
		if (!ghost)
			continue;

		ghost->Tn2 += offset;
		ghost->Tn1 += offset;
		ghost->T0 += offset;
	}

	// Adjust on all queued events also.
	for (std::list< Event >::iterator i = m_eventsIn.begin(); i != m_eventsIn.end(); ++i)
		i->time += offset;
	for (std::list< Event >::iterator i = m_eventsOut.begin(); i != m_eventsOut.end(); ++i)
		i->time += offset;
}

bool Replicator::sendMasqueraded(handle_t fromPeerHandle, handle_t targetPeerHandle, const Message* msg, uint32_t size, bool reliable)
{
	Message mm;
	mm.type = MtMasquerade;
	mm.time = uint32_t(m_time * 1000.0f);
	mm.masquerade.fromGlobalId = fromPeerHandle;
	std::memcpy(mm.masquerade.data, msg, size);
	return m_replicatorPeers->send(targetPeerHandle, &mm, Message::HeaderSize + sizeof(uint64_t) + size, reliable);
}

bool Replicator::sendRelay(handle_t peerHandle, const Message* msg, uint32_t size, bool reliable)
{
	handle_t relayPeerHandle;
	if (findOptimalRelay(peerHandle, peerHandle, relayPeerHandle))
	{
		Message mr;
		mr.type = reliable ? MtRelayReliable1 : MtRelayUnreliable1;
		mr.time = uint32_t(m_time * 1000.0f);
		mr.relay.fromGlobalId = m_replicatorPeers->getGlobalId();
		mr.relay.targetGlobalId = peerHandle;
		std::memcpy(mr.relay.data, msg, size);
		return m_replicatorPeers->send(relayPeerHandle, &mr, Message::HeaderSize + 2 * sizeof(uint64_t) + size, reliable);
	}
	else
		return m_replicatorPeers->send(peerHandle, msg, size, reliable);
}

bool Replicator::send(handle_t peerHandle, const Message* msg, uint32_t size, bool reliable)
{
	T_ASSERT (size <= Message::MessageSize);

	std::map< handle_t, Peer >::const_iterator i = m_peers.find(peerHandle);
	if (i == m_peers.end())
		return false;

	if (!i->second.relay)
		return m_replicatorPeers->send(peerHandle, msg, size, reliable);
	else
		return sendRelay(peerHandle, msg, size, reliable);
}

int32_t Replicator::receive(Message* msg, handle_t& outPeerHandle)
{
	handle_t relayPeerHandle;
	int32_t size;

	for (;;)
	{
		size = m_replicatorPeers->receive(msg, sizeof(Message), outPeerHandle);
		if (size <= 0)
			return size;

		// Reset relaying to peer from whom I just received data; if we can receive from
		// peer then we should be able to send to it as well.
		Peer& peer = m_peers[outPeerHandle];
		peer.relay = false;

		if (
			msg->type == MtRelayUnreliable1 ||
			msg->type == MtRelayUnreliable2 ||
			msg->type == MtRelayReliable1 ||
			msg->type == MtRelayReliable2
		)
		{
			size -= Message::HeaderSize + 2 * sizeof(uint64_t);
			if (size <= 0)
				return size;

			bool first = bool(msg->type == MtRelayUnreliable1 || msg->type == MtRelayReliable1);
			bool reliable = bool(msg->type == MtRelayReliable1 || msg->type == MtRelayReliable2);

			handle_t fromPeerHandle = msg->relay.fromGlobalId;
			handle_t targetPeerHandle = msg->relay.targetGlobalId;

			if (targetPeerHandle != m_replicatorPeers->getGlobalId())
			{
				Peer& targetPeer = m_peers[targetPeerHandle];
				if (
					first &&
					targetPeer.relay &&
					findOptimalRelay(fromPeerHandle, outPeerHandle, relayPeerHandle)
				)
				{
					T_REPLICATOR_DEBUG(L"OK: Relay message from peer " << outPeerHandle << L" passed on further as no direct connection to target exist");
					
					// Increment message type; assume in order as to become
					// second level relay message type.
					msg->type++;

					// Resend relaying request further as we also depend
					// on relaying to reach target peer.
					bool result = m_replicatorPeers->send(
						relayPeerHandle,
						&msg,
						sizeof(Message),
						reliable
					);

					if (!result)
						T_REPLICATOR_DEBUG(L"ERROR: Unable to relay message from peer " << outPeerHandle << L"; message lost (1)");
				}
				else
				{
					std::memmove(
						msg,
						msg->relay.data,
						size
					);

					bool result = sendMasqueraded(
						fromPeerHandle,
						targetPeerHandle,
						msg,
						size,
						reliable
					);

					if (!result)
						T_REPLICATOR_DEBUG(L"ERROR: Unable to relay message from peer " << outPeerHandle << L"; message lost (2)");
				}
				continue;
			}
			else
			{
				// A relay message to myself; this should never happen but to be safe
				// we just unwrap message and treat it as an ordinary message.
				std::memmove(
					msg,
					msg->relay.data,
					size
				);
			}
		}
		else if (msg->type == MtMasquerade)
		{
			size -= Message::HeaderSize + sizeof(uint64_t);
			if (size <= 0)
				return size;

			outPeerHandle = msg->masquerade.fromGlobalId;
			std::memmove(
				msg,
				msg->masquerade.data,
				size
			);
		}

		break;
	}

	return size;
}

bool Replicator::findOptimalRelay(handle_t ignorePeerHandle0, handle_t ignorePeerHandle1, handle_t& outRelayPeerHandle) const
{
	float relayPeerLatency = std::numeric_limits< float >::max();
	outRelayPeerHandle = 0;

	// Find optimal relay peer; use peer with least latency and no errors.
	for (std::map< handle_t, Peer >::const_iterator i = m_peers.begin(); i != m_peers.end(); ++i)
	{
		if (
			i->first != ignorePeerHandle0 &&
			i->first != ignorePeerHandle1 &&
			i->second.state == PsEstablished &&
			i->second.relay == false &&
			i->second.errorCount == 0 &&
			i->second.latencyMedian < relayPeerLatency
		)
		{
			outRelayPeerHandle = i->first;
			relayPeerLatency = i->second.latencyMedian;
		}
	}

	// If no optimal relay peer found; use least bad.
	if (!outRelayPeerHandle)
	{
		for (std::map< handle_t, Peer >::const_iterator i = m_peers.begin(); i != m_peers.end(); ++i)
		{
			if (
				i->first != ignorePeerHandle0 &&
				i->first != ignorePeerHandle1 &&
				i->second.state == PsEstablished &&
				i->second.relay == false &&
				i->second.latencyMedian < relayPeerLatency
			)
			{
				outRelayPeerHandle = i->first;
				relayPeerLatency = i->second.latencyMedian;
			}
		}
	}

	return outRelayPeerHandle != 0;
}

	}
}
