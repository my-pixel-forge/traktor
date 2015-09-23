#ifndef traktor_amalgam_SparkLayer_H
#define traktor_amalgam_SparkLayer_H

#include "Amalgam/Game/Engine/Layer.h"
#include "Core/Math/Color4ub.h"
#include "Resource/Proxy.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_AMALGAM_GAME_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace amalgam
	{

class IEnvironment;

	}

	namespace spark
	{

class SparkRenderer;
class Sprite;
class SpriteInstance;

	}

	namespace amalgam
	{

/*! \brief Stage Spark layer.
 * \ingroup Amalgam
 */
class T_DLLCLASS SparkLayer : public Layer
{
	T_RTTI_CLASS;

public:
	SparkLayer(
		Stage* stage,
		const std::wstring& name,
		bool permitTransition,
		IEnvironment* environment,
		const resource::Proxy< spark::Sprite >& sprite,
		const Color4ub& background,
		int32_t width,
		int32_t height
	);

	virtual ~SparkLayer();

	void destroy();

	virtual void transition(Layer* fromLayer);

	virtual void prepare();

	virtual void update(const UpdateInfo& info);

	virtual void build(const UpdateInfo& info, uint32_t frame);

	virtual void render(render::EyeType eye, uint32_t frame);

	virtual void flush();

	virtual void preReconfigured();

	virtual void postReconfigured();

	virtual void suspend();

	virtual void resume();

	spark::SpriteInstance* getSprite() const;

private:
	Ref< IEnvironment > m_environment;
	resource::Proxy< spark::Sprite > m_sprite;
	Ref< spark::SpriteInstance > m_spriteInstance;
	Ref< spark::SparkRenderer > m_sparkRenderer;
	Color4ub m_background;
	int32_t m_width;
	int32_t m_height;
};

	}
}

#endif	// traktor_amalgam_SparkLayer_H
