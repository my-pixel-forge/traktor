#include "Runtime/IEnvironment.h"
#include "Runtime/IStateManager.h"
#include "Runtime/UpdateControl.h"
#include "Runtime/UpdateInfo.h"
#include "Runtime/Engine/Layer.h"
#include "Runtime/Engine/Stage.h"
#include "Runtime/Engine/StageLoader.h"
#include "Runtime/Engine/StageState.h"
#include "Core/Class/IRuntimeClass.h"
#include "Core/Class/IRuntimeDispatch.h"
#include "Core/Log/Log.h"
#include "Core/Math/Const.h"
#include "Core/Misc/SafeDestroy.h"
#include "Core/Misc/TString.h"
#include "Core/Timer/Profiler.h"
#include "Render/ScreenRenderer.h"
#include "Render/Shader.h"
#include "Resource/IResourceManager.h"

namespace traktor
{
	namespace runtime
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.runtime.Stage", Stage, Object)

Stage::Stage(
	const std::wstring& name,
	IEnvironment* environment,
	const resource::Proxy< IRuntimeClass >& clazz,
	const resource::Proxy< render::Shader >& shaderFade,
	float fadeRate,
	const std::map< std::wstring, Guid >& transitions,
	const Object* params
)
:	m_name(name)
,	m_environment(environment)
,	m_class(clazz)
,	m_shaderFade(shaderFade)
,	m_fadeRate(fadeRate)
,	m_transitions(transitions)
,	m_params(params)
,	m_initialized(false)
,	m_running(true)
,	m_fade(1.0f)
{
	m_screenRenderer = new render::ScreenRenderer();
	m_screenRenderer->create(m_environment->getRender()->getRenderSystem());
}

Stage::~Stage()
{
	destroy();
}

void Stage::destroy()
{
	m_environment = nullptr;

	if (m_object)
	{
		const IRuntimeDispatch* methodFinalize = findRuntimeClassMethod(m_class, "finalize");
		if (m_initialized && methodFinalize != nullptr)
		{
			Any argv[] =
			{
				Any::fromObject(const_cast< Object* >(m_params.c_ptr()))
			};
			methodFinalize->invoke(m_object, sizeof_array(argv), argv);
		}
		m_object = nullptr;
		m_class.clear();
	}

	safeDestroy(m_screenRenderer);

	m_shaderFade.clear();

	for (auto layer : m_layers)
		layer->destroy();

	m_layers.clear();

	m_params = nullptr;
	m_pendingStage = nullptr;
	m_transitionStage = nullptr;
}

void Stage::addLayer(Layer* layer)
{
	m_layers.push_back(layer);
}

void Stage::removeLayer(Layer* layer)
{
	m_layers.remove(layer);
}

void Stage::removeAllLayers()
{
	m_layers.resize(0);
}

Layer* Stage::getLayer(const std::wstring& name) const
{
	for (auto layer : m_layers)
	{
		if (layer->getName() == name)
			return layer;
	}
	return nullptr;
}

void Stage::terminate()
{
	m_running = false;
}

Any Stage::invokeScript(const std::string& fn, uint32_t argc, const Any* argv)
{
	T_PROFILER_SCOPE(L"Script invoke");

	if (!validateScriptContext())
		return Any();

	if (m_object)
	{
		const IRuntimeDispatch* method = findRuntimeClassMethod(m_class, fn);
		if (method != nullptr)
			return method->invoke(m_object, sizeof_array(argv), argv);
	}

	log::error << L"No such script function \"" << mbstows(fn) << L"\"" << Endl;
	return Any();
}

Ref< Stage > Stage::loadStage(const std::wstring& name, const Object* params)
{
	std::map< std::wstring, Guid >::const_iterator i = m_transitions.find(name);
	if (i == m_transitions.end())
	{
		log::error << L"No transition \"" << name << L"\" found" << Endl;
		return nullptr;
	}

	Ref< StageLoader > stageLoader = StageLoader::create(m_environment, i->second, params);
	if (stageLoader->failed())
	{
		log::error << L"Stage loader failed" << Endl;
		return nullptr;
	}

	return stageLoader->get();
}

Ref< StageLoader > Stage::loadStageAsync(const std::wstring& name, const Object* params)
{
	std::map< std::wstring, Guid >::const_iterator i = m_transitions.find(name);
	if (i == m_transitions.end())
	{
		log::error << L"No transition \"" << name << L"\" found" << Endl;
		return nullptr;
	}

	return StageLoader::createAsync(m_environment, i->second, params);
}

bool Stage::gotoStage(Stage* stage)
{
	m_pendingStage = stage;
	return true;
}

bool Stage::update(IStateManager* stateManager, const UpdateInfo& info)
{
	if (!m_running)
		return false;

	if (!m_pendingStage)
	{
		// Prepare all layers.
		for (auto layer : m_layers)
			layer->prepare(info);

		// Issue script update.
		if (validateScriptContext())
		{
			T_PROFILER_SCOPE(L"Script update");
			if (m_object)
			{
				Any argv[] =
				{
					Any::fromObject(const_cast< UpdateInfo* >(&info))
				};

				const IRuntimeDispatch* methodUpdate = findRuntimeClassMethod(m_class, "update");
				if (methodUpdate != nullptr)
					methodUpdate->invoke(m_object, sizeof_array(argv), argv);
			}
		}

		// Update each layer.
		for (auto layer : m_layers)
			layer->update(info);

		// Issue script post update.
		if (validateScriptContext())
		{
			T_PROFILER_SCOPE(L"Script post update");
			if (m_object)
			{
				Any argv[] =
				{
					Any::fromObject(const_cast< UpdateInfo* >(&info))
				};

				const IRuntimeDispatch* methodPostUpdate = findRuntimeClassMethod(m_class, "postUpdate");
				if (methodPostUpdate != nullptr)
					methodPostUpdate->invoke(m_object, sizeof_array(argv), argv);
			}
		}

		m_fade = max(0.0f, m_fade - info.getSimulationDeltaTime() * m_fadeRate);
	}
	else
	{
		m_fade += info.getSimulationDeltaTime() * m_fadeRate;
		if (m_fade > 1.0f)
		{
			stateManager->enter(new StageState(m_environment, m_pendingStage));
			m_transitionStage = m_pendingStage;
			m_pendingStage = nullptr;
		}
	}

	return true;
}

bool Stage::build(const UpdateInfo& info, uint32_t frame)
{
	for (auto layer : m_layers)
		layer->build(info, frame);
	return true;
}

void Stage::render(uint32_t frame)
{
	for (auto layer : m_layers)
		layer->render(frame);

	if (m_shaderFade && m_fade > FUZZY_EPSILON)
	{
		m_shaderFade->setFloatParameter(L"Fade", m_fade);
		m_screenRenderer->draw(
			m_environment->getRender()->getRenderView(),
			m_shaderFade
		);
	}
}

void Stage::flush()
{
	for (auto layer : m_layers)
		layer->flush();
}

void Stage::transition()
{
	if (!m_transitionStage)
		return;

	// Transition layers; layers of same type and name has the chance
	// to transition data, such as playing music etc.
	for (auto layer : m_transitionStage->m_layers)
	{
		std::wstring layerName = layer->getName();
		if (layerName.empty())
			continue;

		Layer* currentLayer = getLayer(layerName);
		if (currentLayer != layer && &type_of(currentLayer) == &type_of(layer))
			layer->transition(currentLayer);
	}

	m_transitionStage = nullptr;
}

void Stage::preReconfigured()
{
	for (auto layer : m_layers)
		layer->preReconfigured();
}

void Stage::postReconfigured()
{
	for (auto layer : m_layers)
		layer->postReconfigured();

	if (m_object && m_initialized)
	{
		const IRuntimeDispatch* methodReconfigured = findRuntimeClassMethod(m_class, "reconfigured");
		if (methodReconfigured != nullptr)
			methodReconfigured->invoke(m_object, 0, 0);
	}
}

void Stage::suspend()
{
	if (m_object && m_initialized)
	{
		const IRuntimeDispatch* methodSuspend = findRuntimeClassMethod(m_class, "suspend");
		if (methodSuspend != nullptr)
			methodSuspend->invoke(m_object, 0, 0);
	}

	for (auto layer : m_layers)
		layer->suspend();
}

void Stage::resume()
{
	for (auto layer : m_layers)
		layer->resume();

	if (m_object && m_initialized)
	{
		const IRuntimeDispatch* methodResume = findRuntimeClassMethod(m_class, "resume");
		if (methodResume != nullptr)
			methodResume->invoke(m_object, 0, 0);
	}
}

bool Stage::validateScriptContext()
{
	if (!m_class)
		return false;

	if (!m_initialized)
	{
		T_PROFILER_SCOPE(L"Script validate");
		if (m_class)
		{
			// Call script constructor.
			Any argv[] =
			{
				Any::fromObject(const_cast< Object* >(m_params.c_ptr())),
				Any::fromObject(m_environment)
			};
			m_object = createRuntimeClassInstance(m_class, this, sizeof_array(argv), argv);
		}
		m_initialized = true;
	}

	return true;
}

	}
}
