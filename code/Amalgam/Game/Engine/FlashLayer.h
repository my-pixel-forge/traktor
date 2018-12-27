/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_amalgam_FlashLayer_H
#define traktor_amalgam_FlashLayer_H

#include "Amalgam/Game/Engine/Layer.h"
#include "Core/Class/Any.h"
#include "Core/Math/Vector2.h"
#include "Flash/Action/Common/Classes/As_flash_external_ExternalInterface.h"
#include "Resource/Id.h"
#include "Resource/Proxy.h"
#include "Spray/Feedback/IFeedbackListener.h"

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

	namespace flash
	{

class AccDisplayRenderer;
class ActionObject;
class Movie;
class MoviePlayer;
class MovieRenderer;
class SpriteInstance;
class ISoundRenderer;

	}

	namespace render
	{

class ImageProcess;
class ImageProcessSettings;
class RenderTargetSet;		

	}

	namespace amalgam
	{

/*! \brief Stage Flash layer.
 * \ingroup Amalgam
 */
class T_DLLCLASS FlashLayer
:	public Layer
,	public flash::IExternalCall
,	public spray::IFeedbackListener
{
	T_RTTI_CLASS;

public:
	FlashLayer(
		Stage* stage,
		const std::wstring& name,
		bool permitTransition,
		IEnvironment* environment,
		const resource::Proxy< flash::Movie >& movie,
		const std::map< std::wstring, resource::Proxy< flash::Movie > >& externalMovies,
		const resource::Proxy< render::ImageProcessSettings >& imageProcessSettings,
		bool clearBackground,
		bool enableShapeCache,
		bool enableDirtyRegions,
		bool enableSound,
		uint32_t contextSize
	);

	virtual void destroy() override;

	virtual void transition(Layer* fromLayer) override final;

	virtual void prepare(const UpdateInfo& info) override final;

	virtual void update(const UpdateInfo& info) override final;

	virtual void build(const UpdateInfo& info, uint32_t frame) override final;

	virtual void render(render::EyeType eye, uint32_t frame) override final;

	virtual void flush() override final;

	virtual void preReconfigured() override final;

	virtual void postReconfigured() override final;

	virtual void suspend() override final;

	virtual void resume() override final;

	/*! \brief Get reference to current Flash movie player.
	 * 
	 * \return Flash movie player object.
	 */
	flash::MoviePlayer* getMoviePlayer();

	/*! \brief Get ActionScript context.
	 *
	 * \return ActionScript context.
	 */
	flash::ActionContext* getContext();

	/*! \brief Get "_root" sprite.
	 *
	 * \return "_root" sprite.
	 */
	flash::SpriteInstance* getRoot();

	/*! \brief Get external movie.
	 *
	 * \param id Name of external movie.
	 * \return External movie, null if doesn't exist.
	 */
	flash::Movie* getExternal(const std::wstring& id) const;

	/*! \brief Invoke methods registered through Flash ExternalInterface protocol.
	 *
	 * \param methodName Name of registered method.
	 * \param argc Argument count.
	 * \param argv Arguments.
	 * \return Return value from call.
	 */
	Any externalCall(const std::string& methodName, uint32_t argc, const Any* argv);

	/*! \brief Get "safe" string which only contain glyphs which are valid with loaded fonts.
	 *
	 * \param text Input text with potential unprintable glyphs.
	 * \param empty String to return if no glyphs in input text was valid.
	 * \return String containing only printable glyphs.
	 */
	std::wstring getPrintableString(const std::wstring& text, const std::wstring& empty) const;

	/*! \brief Set if Flash should be rendererd.
	 *
	 * \param visible True if Flash should be rendered.
	 */
	void setVisible(bool visible) { m_visible = visible; }

	/*! \brief Check if Flash is being rendered.
	 *
	 * \return True if Flash is being rendered.
	 */
	bool isVisible() const { return m_visible; }

private:
	struct LastMouseState
	{
		int32_t button;
		int32_t wheel;

		LastMouseState()
		:	button(0)
		,	wheel(0)
		{
		}
	};

	Ref< IEnvironment > m_environment;
	resource::Proxy< flash::Movie > m_movie;
	std::map< std::wstring, resource::Proxy< flash::Movie > > m_externalMovies;
	Ref< flash::MoviePlayer > m_moviePlayer;
	Ref< flash::MovieRenderer > m_movieRenderer;
	Ref< flash::AccDisplayRenderer > m_displayRenderer;
	Ref< flash::ISoundRenderer > m_soundRenderer;
	Ref< render::RenderTargetSet > m_imageTargetSet;
	resource::Proxy< render::ImageProcessSettings > m_imageProcessSettings;
	Ref< render::ImageProcess > m_imageProcess;
	bool m_clearBackground;
	bool m_enableShapeCache;
	bool m_enableDirtyRegions;
	bool m_enableSound;
	uint32_t m_contextSize;
	bool m_visible;
	Vector2 m_offset;
	float m_scale;
	bool m_lastUpValue;
	bool m_lastDownValue;
	bool m_lastConfirmValue;
	bool m_lastEscapeValue;
	LastMouseState m_lastMouse[8];
	int32_t m_lastMouseX;
	int32_t m_lastMouseY;

	void createMoviePlayer();

	virtual flash::ActionValue dispatchExternalCall(const std::string& methodName, int32_t argc, const flash::ActionValue* argv) override final;

	virtual void feedbackValues(spray::FeedbackType type, const float* values, int32_t count) override final;
};

	}
}

#endif	// traktor_amalgam_FlashLayer_H
