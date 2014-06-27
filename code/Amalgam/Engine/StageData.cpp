#include "Amalgam/IEnvironment.h"
#include "Amalgam/Engine/LayerData.h"
#include "Amalgam/Engine/Stage.h"
#include "Amalgam/Engine/StageData.h"
#include "Core/Log/Log.h"
#include "Core/Serialization/AttributeType.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/MemberRefArray.h"
#include "Core/Serialization/MemberStl.h"
#include "Core/Settings/PropertyBoolean.h"
#include "Core/Settings/PropertyGroup.h"
#include "Database/Database.h"
#include "Render/Shader.h"
#include "Resource/IResourceManager.h"
#include "Resource/Member.h"
#include "Resource/ResourceBundle.h"
#include "Script/IScriptContext.h"

namespace traktor
{
	namespace amalgam
	{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"traktor.amalgam.StageData", 4, StageData, ISerializable)

Ref< Stage > StageData::createInstance(amalgam::IEnvironment* environment, const Object* params) const
{
	resource::IResourceManager* resourceManager = environment->getResource()->getResourceManager();
	resource::Proxy< script::IScriptContext > script;
	resource::Proxy< render::Shader > shaderFade;

#if !defined(_DEBUG)
	// Load resource bundle.
	if (m_resourceBundle.isNotNull())
	{
		bool skipPreload = environment->getSettings()->getProperty< PropertyBoolean >(L"Amalgam.SkipPreloadResources", false);
		if (!skipPreload)
		{
			Ref< const resource::ResourceBundle > resourceBundle = environment->getDatabase()->getObjectReadOnly< resource::ResourceBundle >(m_resourceBundle);
			if (resourceBundle)
				resourceManager->load(resourceBundle);
		}
		else
			log::warning << L"Pre-loading of resources skipped" << Endl;
	}
#endif

	// Bind proxies to resource manager.
	if (m_script && !resourceManager->bind(m_script, script))
		return 0;
	if (m_shaderFade && !resourceManager->bind(m_shaderFade, shaderFade))
		return 0;

	// Create layers.
	Ref< Stage > stage = new Stage(environment, script, shaderFade, m_transitions, params);
	for (RefArray< LayerData >::const_iterator i = m_layers.begin(); i != m_layers.end(); ++i)
	{
		Ref< Layer > layer = (*i)->createInstance(stage, environment);
		if (!layer)
			return 0;

		stage->addLayer(layer);
	}

	return stage;
}

void StageData::serialize(ISerializer& s)
{
	s >> MemberRefArray< LayerData >(L"layers", m_layers);
	s >> resource::Member< script::IScriptContext >(L"script", m_script);

	if (s.getVersion() >= 4)
		s >> resource::Member< render::Shader >(L"shaderFade", m_shaderFade);
	else
	{
		const resource::Id< render::Shader > c_shaderFade(Guid(L"{DC104971-11AE-5743-9AB1-53B830F74391}"));
		m_shaderFade = c_shaderFade;
	}

	s >> MemberStlMap< std::wstring, Guid >(L"transitions", m_transitions);

	if (s.getVersion() >= 1)
		s >> Member< Guid >(L"resourceBundle", m_resourceBundle, AttributeType(type_of< resource::ResourceBundle >()));

	if (s.getVersion() == 2)
	{
		Guid dummy;
		s >> Member< Guid >(L"localizationDictionary", dummy);
	}
}

	}
}
