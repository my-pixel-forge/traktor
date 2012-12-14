#ifndef traktor_net_Replicator_H
#define traktor_net_Replicator_H

#include <list>
#include <map>
#include "Core/RefArray.h"
#include "Core/Containers/CircularVector.h"
#include "Core/Math/Vector4.h"
#include "Core/Serialization/ISerializable.h"
#include "Net/Replication/ReplicatorTypes.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NET_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace net
	{

//#define T_PROFILE_REPLICATOR

class IReplicatorPeers;
class State;
class StateTemplate;

class T_DLLCLASS Replicator : public Object
{
	T_RTTI_CLASS;

public:
	class T_DLLCLASS IListener : public Object
	{
		T_RTTI_CLASS;

	public:
		enum
		{
			ReConnected = 1,
			ReDisconnected = 2,
			ReBroadcastEvent = 3,
			ReState = 4
		};

		virtual void notify(
			Replicator* replicator,
			float eventTime,
			uint32_t eventId,
			handle_t peerHandle,
			const Object* eventObject
		) = 0;
	};

	Replicator();

	virtual ~Replicator();

	/*! \brief
	 */
	bool create(IReplicatorPeers* replicatorPeers);

	/*! \brief
	 */
	void destroy();

	/*! \brief
	 */
	void addEventType(const TypeInfo& eventType);

	/*! \brief
	 */
	void addListener(IListener* listener);

	/*! \brief
	 */
	void update(float T, float dT);

	/*! \brief Set our origin.
	 *
	 * Origin is used to determine which frequency
	 * of transmission to use to each peer.
	 */
	void setOrigin(const Vector4& origin);

	/*!
	 */
	void setStateTemplate(const StateTemplate* stateTemplate);

	/*! \brief Set our replication state.
	 *
	 * Each peer have multiple ghost states which mirrors
	 * each peer real state.
	 */
	void setState(const State* state);

	/*! \brief Send high priority event to a single peer.
	 */
	void sendEvent(handle_t peerHandle, const ISerializable* eventObject);

	/*! \brief Broadcast high priority event to all peers.
	 */
	void broadcastEvent(const ISerializable* eventObject);

	/*! \brief
	 */
	bool isPrimary() const;

	/*! \brief
	 */
	uint32_t getPeerCount() const;

	/*! \brief
	 */
	handle_t getPeerHandle(uint32_t peerIndex) const;

	/*! \brief Get peer name.
	 */
	std::wstring getPeerName(handle_t peerHandle) const;

	/*! \brief Get peer minimum latency.
	 * \return Latency in milliseconds.
	 */
	int32_t getPeerLatency(handle_t peerHandle) const;

	/*! \brief Get peer reversed minimum latency of myself.
	 * \return Latency in milliseconds.
	 */
	int32_t getPeerReversedLatency(handle_t peerHandle) const;

	/*! \brief Get best peer reversed minimum latency of myself.
	 * \return Latency in milliseconds.
	 */
	int32_t getBestReversedLatency() const;

	/*! \brief Get worst peer reversed minimum latency of myself.
	 * \return Latency in milliseconds.
	 */
	int32_t getWorstReversedLatency() const;

	/*! \brief Check if peer is connected.
	 */
	bool isPeerConnected(handle_t peerHandle) const;

	/*! \brief Check if all peers are connected.
	 */
	bool areAllPeersConnected() const;

	/*! \brief Attach an object to a ghost peer.
	 *
	 * This permits attaching user objects to ghost
	 * peers at anytime.
	 */
	void setGhostObject(handle_t peerHandle, Object* ghostObject);

	/*! \brief
	 */
	Object* getGhostObject(handle_t peerHandle) const;

	/*! \brief Set ghost origin.
	 *
	 * Ghost origin is used to determine which frequency
	 * of transmission to use for each peer.
	 */
	void setGhostOrigin(handle_t peerHandle, const Vector4& origin);

	/*!
	 */
	void setGhostStateTemplate(handle_t peerHandle, const StateTemplate* stateTemplate);

	/*! \brief Get state of ghost peer.
	 *
	 * The state of ghost peers are extrapolated
	 * in order to have a virtually identical
	 * state as the actual peer.
	 *
	 * \param peerHandle Peer handle.
	 * \param currentState Current ghost local state.
	 * \return Extrapolated ghost state.
	 */
	Ref< const State > getGhostState(uint32_t peerHandle, const State* currentState) const;

	/*! \brief Get loopback state.
	 *
	 * Loopback state is our own state but mangled
	 * through the state template to enable
	 * debugging of packing/unpacking of state variables.
	 */
	Ref< const State > getLoopBackState() const;

	/*! \brief Get state.
	 */
	const State* getState() const { return m_state; }

	/*! \brief Get network time.
	 */
	float getTime() const { return m_time; }

private:
	enum { MaxRoundTrips = 9 };

	struct Event
	{
		float time;
		uint32_t eventId;
		handle_t handle;
		Ref< const ISerializable > object;
	};

	struct Ghost
	{
		Vector4 origin;
		Ref< Object > object;
		Ref< const StateTemplate > stateTemplate;
		Ref< const State > Sn2;
		Ref< const State > Sn1;
		Ref< const State > S0;
		float Tn2;
		float Tn1;
		float T0;
	};

	enum PeerState
	{
		PsInitial,
		PsEstablished,
		PsDisconnected
	};

	struct Peer
	{
		PeerState state;
		Ghost* ghost;
		float timeUntilTx;
		float lastTime;
		CircularVector< float, MaxRoundTrips > roundTrips;
		float latencyMedian;
		float latencyMinimum;
		float latencyReversed;
		uint32_t pendingPing;
		uint32_t lossDelta;
		uint32_t packetCount;
		uint32_t stateCount;
		uint32_t errorCount;
		uint8_t txSequence;
		uint8_t rxSequence;
		Ref< const State > iframe;

		Peer()
		:	state(PsInitial)
		,	ghost(0)
		,	timeUntilTx(0.0f)
		,	lastTime(0.0f)
		,	latencyMedian(0.0f)
		,	latencyMinimum(0.0f)
		,	latencyReversed(0.0f)
		,	pendingPing(0)
		,	lossDelta(0)
		,	packetCount(0)
		,	stateCount(0)
		,	errorCount(0)
		,	txSequence(0)
		,	rxSequence(0)
		{
		}
	};

	uint32_t m_id;
	std::vector< const TypeInfo* > m_eventTypes;
	Ref< IReplicatorPeers > m_replicatorPeers;
	RefArray< IListener > m_listeners;
	Vector4 m_origin;
	Ref< const StateTemplate > m_stateTemplate;
	Ref< const State > m_state;
	std::map< handle_t, Peer > m_peers;
	std::list< Event > m_eventsIn;
	std::list< Event > m_eventsOut;
	float m_time;
	uint32_t m_pingCount;
	float m_timeUntilPing;

	void sendIAm(handle_t peerHandle, uint8_t sequence, uint32_t id);

	void sendBye(handle_t peerHandle);

	void sendPing(handle_t peerHandle);

	void sendPong(handle_t peerHandle, uint32_t time0);

	void sendThrottle(handle_t peerHandle);

	void sendDisconnect(handle_t peerHandle);
};

	}
}

#endif	// traktor_net_Replicator_H
