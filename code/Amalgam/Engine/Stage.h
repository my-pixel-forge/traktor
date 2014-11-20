#ifndef traktor_amalgam_Stage_H
#define traktor_amalgam_Stage_H

#include <map>
#include "Core/Object.h"
#include "Core/RefArray.h"
#include "Render/Types.h"
#include "Resource/Proxy.h"
#include "Script/Any.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_AMALGAM_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace render
	{

class ScreenRenderer;
class Shader;

	}

	namespace script
	{

class IScriptContext;

	}

	namespace amalgam
	{

class IEnvironment;
class IStateManager;
class IUpdateControl;
class IUpdateInfo;
class Layer;
class StageLoader;

class T_DLLCLASS Stage : public Object
{
	T_RTTI_CLASS;

public:
	Stage(
		IEnvironment* environment,
		const resource::Proxy< script::IScriptContext >& scriptContext,
		const resource::Proxy< render::Shader >& shaderFade,
		float fadeRate,
		const std::map< std::wstring, Guid >& transitions,
		const Object* params
	);

	virtual ~Stage();

	void destroy();

	void addLayer(Layer* layer);

	void removeLayer(Layer* layer);

	void removeAllLayers();

	Layer* findLayer(const std::wstring& name) const;

	void terminate();

	script::Any invokeScript(const std::string& fn, uint32_t argc, const script::Any* argv);

	Ref< Stage > loadStage(const std::wstring& name, const Object* params);

	Ref< StageLoader > loadStageAsync(const std::wstring& name, const Object* params);

	bool gotoStage(Stage* stage);

	bool update(IStateManager* stateManager, IUpdateControl& control, const IUpdateInfo& info);

	bool build(const IUpdateInfo& info, uint32_t frame);

	void render(render::EyeType eye, uint32_t frame);

	void flush();

	void transition();

	void preReconfigured();

	void postReconfigured();

	void suspend();

	void resume();

	IEnvironment* getEnvironment() { return m_environment; }

	const RefArray< Layer >& getLayers() const { return m_layers; }

	const Object* getParams() const { return m_params; }

private:
	Ref< IEnvironment > m_environment;
	resource::Proxy< script::IScriptContext > m_scriptContext;
	Ref< render::ScreenRenderer > m_screenRenderer;
	resource::Proxy< render::Shader > m_shaderFade;
	float m_fadeRate;
	RefArray< Layer > m_layers;
	std::map< std::wstring, Guid > m_transitions;
	Ref< const Object > m_params;
	Ref< Stage > m_pendingStage;
	Ref< Stage > m_transitionStage;
	bool m_initialized;
	bool m_running;
	float m_fade;

	bool validateScriptContext();

	script::Any invokeScriptUpdate(IUpdateControl& control, const IUpdateInfo& info);
};

	}
}

#endif	// traktor_amalgam_Stage_H
