#include "Amalgam/Impl/AudioServer.h"
#include "Amalgam/Impl/Environment.h"
#include "Amalgam/Impl/InputServer.h"
#include "Amalgam/Impl/OnlineServer.h"
#include "Amalgam/Impl/PhysicsServer.h"
#include "Amalgam/Impl/RenderServer.h"
#include "Amalgam/Impl/ResourceServer.h"
#include "Amalgam/Impl/ScriptServer.h"
#include "Amalgam/Impl/WorldServer.h"

namespace traktor
{
	namespace amalgam
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.amalgam.Environment", Environment, Object)

Environment::Environment(
	Settings* settings,
	db::Database* database,
	AudioServer* audioServer,
	InputServer* inputServer,
	OnlineServer* onlineServer,
	PhysicsServer* physicsServer,
	RenderServer* renderServer,
	ResourceServer* resourceServer,
	ScriptServer* scriptServer,
	WorldServer* worldServer
)
:	m_settings(settings)
,	m_database(database)
,	m_audioServer(audioServer)
,	m_inputServer(inputServer)
,	m_onlineServer(onlineServer)
,	m_physicsServer(physicsServer)
,	m_renderServer(renderServer)
,	m_resourceServer(resourceServer)
,	m_scriptServer(scriptServer)
,	m_worldServer(worldServer)
,	m_shouldReconfigure(false)
{
}

db::Database* Environment::getDatabase()
{
	return m_database;
}

IAudioServer* Environment::getAudio()
{
	return m_audioServer;
}

IInputServer* Environment::getInput()
{
	return m_inputServer;
}

IOnlineServer* Environment::getOnline()
{
	return m_onlineServer;
}

IPhysicsServer* Environment::getPhysics()
{
	return m_physicsServer;
}

IRenderServer* Environment::getRender()
{
	return m_renderServer;
}

IResourceServer* Environment::getResource()
{
	return m_resourceServer;
}

IScriptServer* Environment::getScript()
{
	return m_scriptServer;
}

IWorldServer* Environment::getWorld()
{
	return m_worldServer;
}

Settings* Environment::getSettings()
{
	return m_settings;
}

bool Environment::reconfigure()
{
	m_shouldReconfigure = true;
	return true;
}

int32_t Environment::executeReconfigure()
{
	m_shouldReconfigure = false;

	int32_t result = CrUnaffected;

	if ((result |= m_audioServer->reconfigure(m_settings)) == CrFailed)
		return CrFailed;
	if ((result |= m_inputServer->reconfigure(m_settings)) == CrFailed)
		return CrFailed;
	if ((result |= m_onlineServer->reconfigure(m_settings)) == CrFailed)
		return CrFailed;
	if ((result |= m_physicsServer->reconfigure(m_settings)) == CrFailed)
		return CrFailed;
	if ((result |= m_renderServer->reconfigure(m_settings)) == CrFailed)
		return CrFailed;
	if ((result |= m_resourceServer->reconfigure(m_settings)) == CrFailed)
		return CrFailed;
	if ((result |= m_scriptServer->reconfigure(m_settings)) == CrFailed)
		return CrFailed;
	if ((result |= m_worldServer->reconfigure(m_settings)) == CrFailed)
		return CrFailed;

	return result;
}

	}
}
