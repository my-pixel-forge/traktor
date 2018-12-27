/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_flash_Edit_H
#define traktor_flash_Edit_H

#include "Flash/Character.h"
#include "Flash/SwfTypes.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_FLASH_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace flash
	{

#if defined (_MSC_VER)
#	pragma warning( disable:4324 )
#endif

/*! \brief Dynamic text field.
 * \ingroup Flash
 */
class T_DLLCLASS Edit : public Character
{
	T_RTTI_CLASS;

public:
	Edit();

	Edit(
		uint16_t id,
		uint16_t fontId,
		uint16_t fontHeight,
		const Aabb2& textBounds,
		const Color4f& textColor,
		uint16_t maxLength,
		const std::wstring& initialText,
		SwfTextAlignType align,
		uint16_t leftMargin,
		uint16_t rightMargin,
		int16_t indent,
		int16_t leading,
		bool readOnly,
		bool wordWrap,
		bool multiLine,
		bool password,
		bool renderHtml
	);

	virtual Ref< CharacterInstance > createInstance(
		ActionContext* context,
		Dictionary* dictionary,
		CharacterInstance* parent,
		const std::string& name,
		const Matrix33& transform,
		const ActionObject* initObject,
		const SmallMap< uint32_t, Ref< const IActionVMImage > >* events
	) const override final;

	/*! \brief Get font identifier.
	 *
	 * \return Font identifier.
	 */
	uint16_t getFontId() const;

	/*! \brief Get font height.
	 *
	 * \return Font height.
	 */
	uint16_t getFontHeight() const;

	/*! \brief Get text character bounding box.
	 *
	 * \return Bounding box.
	 */
	const Aabb2& getTextBounds() const;

	/*! \brief Get text color.
	 *
	 * \return Text color.
	 */
	const Color4f& getTextColor() const;

	/*! \brief Get max length of input text.
	 *
	 * \return Max length.
	 */
	uint16_t getMaxLength() const;

	/*! \brief Set initial text string.
	 */
	void setInitialText(const std::wstring& initialText);

	/*! \brief Initial text string.
	 *
	 * \return Initial text, can be HTML.
	 */
	const std::wstring& getInitialText() const;

	/*! \brief Get text alignment within bounding box.
	 *
	 * \return Alignment.
	 */
	SwfTextAlignType getAlign() const;

	/*! \brief Get left margin.
	 *
	 * \return Left margin.
	 */
	uint16_t getLeftMargin() const;

	/*! \brief Get right margin.
	 *
	 * \return Right margin.
	 */
	uint16_t getRightMargin() const;

	/*! \brief Get indent.
	 *
	 * \return Indent.
	 */
	int16_t getIndent() const;

	/*! \brief Get leading.
	 *
	 * \return Leading.
	 */
	int16_t getLeading() const;

	/*! \brief Read only.
	 *
	 * \return True if read-only.
	 */
	bool readOnly() const;

	/*! \brief Word wrap enabled.
	 *
	 * \return True if word wrap is enabled.
	 */
	bool wordWrap() const;

	/*! \brief Multiline text field.
	 *
	 * \return True if multiline.
	 */
	bool multiLine() const;

	/*! \brief Password text field.
	 *
	 * \return True if password field.
	 */
	bool password() const;

	/*! \brief Render HTML content.
	 *
	 * \return True if text is HTML.
	 */
	bool renderHtml() const;

	virtual void serialize(ISerializer& s) override final;

private:
	uint16_t m_fontId;
	uint16_t m_fontHeight;
	Aabb2 m_textBounds;
	Color4f m_textColor;
	std::wstring m_initialText;
	uint16_t m_maxLength;
	SwfTextAlignType m_align;
	uint16_t m_leftMargin;
	uint16_t m_rightMargin;
	int16_t m_indent;
	int16_t m_leading;
	bool m_readOnly;
	bool m_wordWrap;
	bool m_multiLine;
	bool m_password;
	bool m_renderHtml;
};

#if defined (_MSC_VER)
#	pragma warning( default:4324 )
#endif

	}
}

#endif	// traktor_flash_Edit_H
