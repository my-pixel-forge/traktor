#ifndef traktor_parade_Layer_H
#define traktor_parade_Layer_H

#include "Core/Object.h"
#include "Render/Types.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_PARADE_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace amalgam
	{

class IUpdateControl;
class IUpdateInfo;

	}

	namespace parade
	{

class Stage;

class T_DLLCLASS Layer : public Object
{
	T_RTTI_CLASS;

public:
	Layer(
		Stage* stage,
		const std::wstring& name
	);

	virtual ~Layer();

	void destroy();

	virtual void prepare() = 0;

	virtual void update(amalgam::IUpdateControl& control, const amalgam::IUpdateInfo& info) = 0;

	virtual void build(const amalgam::IUpdateInfo& info, uint32_t frame) = 0;

	virtual void render(render::EyeType eye, uint32_t frame) = 0;

	virtual void reconfigured() = 0;

	Stage* getStage() const { return m_stage; }

	const std::wstring& getName() const { return m_name; }

private:
	Stage* m_stage;
	std::wstring m_name;
};

	}
}

#endif	// traktor_parade_Layer_H
