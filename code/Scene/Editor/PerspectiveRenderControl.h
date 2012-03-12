#ifndef traktor_scene_PerspectiveRenderControl_H
#define traktor_scene_PerspectiveRenderControl_H

#include "Core/RefArray.h"
#include "Core/Math/Color4ub.h"
#include "Core/Math/Matrix44.h"
#include "Core/Timer/Timer.h"
#include "Scene/Editor/ISceneRenderControl.h"
#include "Scene/Editor/RenderControlModel.h"
#include "Ui/Point.h"
#include "World/WorldRenderSettings.h"
#include "World/WorldRenderView.h"

namespace traktor
{
	namespace ui
	{

class Container;
class Event;
class Widget;

	}

	namespace render
	{

class IRenderView;
class RenderTargetSet;
class PrimitiveRenderer;

	}

	namespace world
	{

class IWorldRenderer;
class PostProcess;
class Entity;

	}

	namespace scene
	{

class Camera;
class SceneEditorContext;

class PerspectiveRenderControl : public ISceneRenderControl
{
	T_RTTI_CLASS;

public:
	PerspectiveRenderControl();

	bool create(ui::Widget* parent, SceneEditorContext* context, int32_t index);

	virtual void destroy();

	virtual void updateWorldRenderer();

	virtual void setAspect(float aspect);

	virtual bool handleCommand(const ui::Command& command);

	virtual void update();

	virtual bool hitTest(const ui::Point& position) const;

	virtual bool calculateRay(const ui::Point& position, Vector4& outWorldRayOrigin, Vector4& outWorldRayDirection) const;

	virtual void moveCamera(MoveCameraMode mode, const Vector4& mouseDelta, const Vector4& viewDelta);

private:
	Ref< SceneEditorContext > m_context;
	Ref< ui::Container > m_containerAspect;
	Ref< ui::Widget > m_renderWidget;
	Ref< render::IRenderView > m_renderView;
	Ref< render::RenderTargetSet > m_renderTarget;
	Ref< render::PrimitiveRenderer > m_primitiveRenderer;
	Ref< world::IWorldRenderer > m_worldRenderer;
	Ref< world::PostProcess > m_postProcess;
	world::WorldRenderView m_worldRenderView;
	world::WorldRenderSettings m_worldRenderSettings;
	RenderControlModel m_model;
	int32_t m_index;
	bool m_gridEnable;
	bool m_guideEnable;
	bool m_postProcessEnable;
	Color4ub m_colorClear;
	Color4ub m_colorGrid;
	Color4ub m_colorRef;
	float m_fieldOfView;
	float m_mouseWheelRate;
	int32_t m_multiSample;
	bool m_invertPanY;
	Timer m_timer;
	Ref< Camera > m_camera;
	ui::Size m_dirtySize;

	void updateSettings();

	void updateWorldRenderView();

	Matrix44 getProjectionTransform() const;

	Matrix44 getViewTransform() const;

	void eventButtonDown(ui::Event* event);

	void eventButtonUp(ui::Event* event);

	void eventDoubleClick(ui::Event* event);

	void eventMouseMove(ui::Event* event);

	void eventMouseWheel(ui::Event* event);

	void eventSize(ui::Event* event);

	void eventPaint(ui::Event* event);
};

	}
}

#endif	// traktor_scene_PerspectiveRenderControl_H
