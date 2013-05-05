#include "Amalgam/IEnvironment.h"
#include "Core/Serialization/AttributeType.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/MemberRefArray.h"
#include "Core/Serialization/MemberStl.h"
#include "Database/Database.h"
#include "Parade/LayerData.h"
#include "Parade/Stage.h"
#include "Parade/StageData.h"
#include "Resource/IResourceManager.h"
#include "Resource/Member.h"
#include "Resource/ResourceBundle.h"
#include "Script/IScriptContext.h"

namespace traktor
{
	namespace parade
	{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"traktor.parade.StageData", 1, StageData, ISerializable)

Ref< Stage > StageData::createInstance(amalgam::IEnvironment* environment, const Object* params) const
{
	resource::IResourceManager* resourceManager = environment->getResource()->getResourceManager();
	resource::Proxy< script::IScriptContext > script;

#if !defined(_DEBUG)
	// Load resource bundle.
	if (m_resourceBundle.isNotNull())
	{
		Ref< const resource::ResourceBundle > resourceBundle = environment->getDatabase()->getObjectReadOnly< resource::ResourceBundle >(m_resourceBundle);
		if (resourceBundle)
			resourceManager->load(resourceBundle);
	}
#endif

	// Bind proxies to resource manager.
	if (m_script && !resourceManager->bind(m_script, script))
		return 0;

	// Create layers.
	Ref< Stage > stage = new Stage(environment, script, m_transitions, params);
	for (RefArray< LayerData >::const_iterator i = m_layers.begin(); i != m_layers.end(); ++i)
	{
		Ref< Layer > layer = (*i)->createInstance(stage, environment);
		if (!layer)
			return 0;

		stage->addLayer(layer);
	}

	return stage;
}

bool StageData::serialize(ISerializer& s)
{
	s >> MemberRefArray< LayerData >(L"layers", m_layers);
	s >> resource::Member< script::IScriptContext >(L"script", m_script);
	s >> MemberStlMap< std::wstring, Guid >(L"transitions", m_transitions);

	if (s.getVersion() >= 1)
		s >> Member< Guid >(L"resourceBundle", m_resourceBundle, AttributeType(type_of< resource::ResourceBundle >()));

	return true;
}

	}
}
