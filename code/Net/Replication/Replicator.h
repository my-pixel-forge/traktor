#ifndef traktor_net_Replicator_H
#define traktor_net_Replicator_H

#include <list>
#include <map>
#include "Core/RefArray.h"
#include "Core/Containers/CircularVector.h"
#include "Core/Math/Transform.h"
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
struct Message;
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
	 *
	 * \param T Time since application started.
	 * \param dT Delta time since last update.
	 * \return True if still connected.
	 */
	bool update(float T, float dT);

	/*! \brief Get our handle.
	 */
	handle_t getHandle() const;

	/*! \brief Get our name.
	 */
	std::wstring getName() const;

	/*! \brief Set our status.
	 */
	void setStatus(uint8_t status);

	/*! \brief Set our origin.
	 *
	 * Origin is used to determine which frequency
	 * of transmission to use to each peer.
	 */
	void setOrigin(const Transform& origin);

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

	/*! \brief Get peer status.
	 */
	uint8_t getPeerStatus(handle_t peerHandle) const;

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

	/*! \brief Check if peer is relayed.
	 */
	bool isPeerRelayed(handle_t peerHandle) const;

	/*! \brief Set primary peer; only valid if we're primary in the first place.
	 */
	bool setPeerPrimary(handle_t peerHandle);

	/*! \brief Get handle of primary peer.
	 *
	 * \return Primary peer handle.
	 */
	handle_t getPrimaryPeerHandle() const;

	/*! \brief Check if peer is primary.
	 */
	bool isPeerPrimary(handle_t peerHandle) const;

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
	void setGhostOrigin(handle_t peerHandle, const Transform& origin);

	/*!
	 */
	void setGhostStateTemplate(handle_t peerHandle, const StateTemplate* stateTemplate);


	float getGhostStateTime(handle_t peerHandle) const;


	Ref< const State > getGhostState(handle_t peerHandle, float T) const;


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
	enum { MaxRoundTrips = 17 };

	struct EventIn
	{
		float time;
		uint32_t eventId;
		handle_t handle;
		Ref< const Object > object;
	};

	struct EventOut
	{
		float time;
		uint32_t eventId;
		handle_t handle;
		Ref< const ISerializable > object;
	};

	struct Ghost
	{
		Transform origin;
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
		std::wstring name;
		Ghost* ghost;
		bool precursor;
		bool direct;
		uint8_t status;
		float timeUntilTx;
		float timeUntilTxMin;
		float lastTimeLocal;
		float lastTimeRemote;
		CircularVector< float, MaxRoundTrips > roundTrips;
		float latencyMedian;
		float latencyMinimum;
		float latencyReversed;
		int32_t pendingIAm;
		int32_t pendingPing;
		int32_t stateCount;
		int32_t errorCount;
		Ref< const State > iframe;

		Peer()
		:	state(PsInitial)
		,	ghost(0)
		,	precursor(false)
		,	direct(false)
		,	status(0)
		,	timeUntilTx(0.0f)
		,	timeUntilTxMin(0.0f)
		,	lastTimeLocal(0.0f)
		,	lastTimeRemote(0.0f)
		,	latencyMedian(0.05f)
		,	latencyMinimum(0.05f)
		,	latencyReversed(0.05f)
		,	pendingIAm(0)
		,	pendingPing(0)
		,	stateCount(0)
		,	errorCount(0)
		{
		}
	};

	uint32_t m_id;
	std::vector< const TypeInfo* > m_eventTypes;
	Ref< IReplicatorPeers > m_replicatorPeers;
	RefArray< IListener > m_listeners;
	Transform m_origin;
	Ref< const StateTemplate > m_stateTemplate;
	Ref< const State > m_state;
	std::map< handle_t, Peer > m_peers;
	std::list< EventIn > m_eventsIn;
	std::list< EventOut > m_eventsOut;
	float m_time0;
	float m_time;
	uint32_t m_pingCount;
	float m_timeUntilPing;

	void updatePeers(float dT);

	void sendState(float dT);

	void sendEvents();

	void sendPings(float dT);

	void receiveMessages();

	void dispatchEventListeners();

	bool sendIAm(handle_t peerHandle, uint8_t sequence, uint32_t id);

	bool sendBye(handle_t peerHandle);

	bool sendPing(handle_t peerHandle);

	bool sendPong(handle_t peerHandle, uint32_t time0);

	void adjustTime(float offset);

	bool send(handle_t peerHandle, const Message* msg, uint32_t size, bool reliable);

	int32_t receive(Message* msg, handle_t& outPeerHandle);
};

	}
}

#endif	// traktor_net_Replicator_H
