#include <cstring>
#include "Flash/FlashMovieFactoryTags.h"
#include "Flash/FlashCharacterInstance.h"
#include "Flash/FlashMovie.h"
#include "Flash/FlashSprite.h"
#include "Flash/FlashFrame.h"
#include "Flash/FlashShape.h"
#include "Flash/FlashMorphShape.h"
#include "Flash/FlashFont.h"
#include "Flash/FlashText.h"
#include "Flash/FlashEdit.h"
#include "Flash/FlashButton.h"
#include "Flash/FlashBitmap.h"
#include "Flash/Action/ActionScript.h"
#include "Flash/SwfReader.h"
#include "Zip/InflateStream.h"
#include "Drawing/Image.h"
#include "Drawing/PixelFormat.h"
#include "Drawing/Formats/ImageFormatJpeg.h"
#include "Core/Heap/GcNew.h"
#include "Core/Io/MemoryStream.h"
#include "Core/Misc/TString.h"
#include "Core/Misc/AutoPtr.h"
#include "Core/Misc/Endian.h"
#include "Core/Log/Log.h"

namespace traktor
{
	namespace flash
	{
		namespace
		{

ActionScript* readActionScript(BitReader& bs)
{
	std::vector< uint8_t > buf;
	for (;;)
	{
		uint8_t opcode = bs.readUInt8();
		buf.push_back(opcode);

		if (opcode & 0x80)
		{
			uint16_t length = bs.readUInt16();
			buf.push_back(reinterpret_cast< uint8_t* >(&length)[0]);
			buf.push_back(reinterpret_cast< uint8_t* >(&length)[1]);
			for (uint16_t i = 0; i < length; ++i)
			{
				uint8_t data = bs.readUInt8();
				buf.push_back(data);
			}
		}

		if (opcode == /*AopEnd*/0)
			break;
	}
	Ref< ActionScript > script = gc_new< ActionScript >(uint32_t(buf.size()));
	memcpy(script->getCode(), &buf[0], buf.size());
	return script;
}

		}

// ============================================================================
// Set background color

bool FlashTagSetBackgroundColor::read(SwfReader* swf, ReadContext& context)
{
	SwfColor color = swf->readRgb();
	context.frame->changeBackgroundColor(color);
	return true;
}

// ============================================================================
// Define shape

FlashTagDefineShape::FlashTagDefineShape(int shapeType)
:	m_shapeType(shapeType)
{
}

bool FlashTagDefineShape::read(SwfReader* swf, ReadContext& context)
{
	BitReader& bs = swf->getBitReader();

	uint16_t shapeId = bs.readUInt16();
	SwfRect shapeBounds = swf->readRect();

	if (m_shapeType == 4)
	{
		SwfRect edgeBounds = swf->readRect();

		bs.skip(5);

		bool useFillWindingRule = bs.readBit();
		bool usesNonScalingStrokes = bs.readBit();
		bool usesScalingStrokes = bs.readBit();
	}

	SwfShape* shape;
	SwfStyles* styles;
	if (!swf->readShapeWithStyle(shape, styles, m_shapeType))
		return false;

	Ref< FlashShape > shapeCharacter = gc_new< FlashShape >(shapeId);
	if (!shapeCharacter->create(shapeBounds, shape, styles))
		return false;

	context.movie->defineCharacter(shapeId, shapeCharacter);
	return true;
}

// ============================================================================
// Define morph shape

FlashTagDefineMorphShape::FlashTagDefineMorphShape(int shapeType)
:	m_shapeType(shapeType)
{
}

bool FlashTagDefineMorphShape::read(SwfReader* swf, ReadContext& context)
{
	BitReader& bs = swf->getBitReader();

	uint16_t shapeId = swf->getBitReader().readUInt16();
	SwfRect startBounds = swf->readRect();
	SwfRect endBounds = swf->readRect();

	if (m_shapeType == 2)
	{
		SwfRect startEdgeBounds = swf->readRect();
		SwfRect endEdgeBounds = swf->readRect();

		bs.skip(6);
		bool nonScalingStrokes = bs.readBit();
		bool scalingStrokes = bs.readBit();
	}

	uint32_t offsetMorph = bs.readUInt32();

	SwfStyles *startStyles, *endStyles;
	if (!swf->readMorphStyles(startStyles, endStyles, m_shapeType))
		return false;

	SwfShape* startShape = swf->readShape(m_shapeType);
	if (!startShape)
		return false;

	SwfShape* endShape = swf->readShape(m_shapeType);
	if (!endShape)
		return false;

	Ref< FlashMorphShape > shape = gc_new< FlashMorphShape >(shapeId);
	if (!shape->create(startBounds, startShape, endShape, startStyles, endStyles))
		return false;

	context.movie->defineCharacter(shapeId, shape);
	return true;
}

// ============================================================================
// Define font

FlashTagDefineFont::FlashTagDefineFont(int fontType)
:	m_fontType(fontType)
{
}

bool FlashTagDefineFont::read(SwfReader* swf, ReadContext& context)
{
	BitReader& bs = swf->getBitReader();

	uint16_t fontId = bs.readUInt16();

	if (m_fontType == 1)
	{
		uint32_t offsetBase = bs.getStream()->tell();
		uint16_t firstOffset = bs.readUInt16();
		uint16_t glyphCount = firstOffset >> 1;

		AlignedVector< uint16_t > offsetTable(glyphCount);
		offsetTable[0] = firstOffset;
		for (uint16_t i = 1; i < glyphCount; ++i)
			offsetTable[i] = bs.readUInt16();

		AlignedVector< SwfShape* > shapeTable(glyphCount);
		for (uint16_t i = 0; i < glyphCount; ++i)
		{
			T_ASSERT (offsetBase + offsetTable[i] < context.tagEndPosition);
			bs.getStream()->seek(Stream::SeekSet, offsetBase + offsetTable[i]);
			bs.alignByte();
			shapeTable[i] = swf->readShape(0);
		}

		Ref< FlashFont > font = gc_new< FlashFont >();
		if (!font->create(shapeTable))
			return false;

		context.movie->defineFont(fontId, font);
	}
	else if (m_fontType == 2 || m_fontType == 3)
	{
		bool hasLayout = bs.readBit();
		bool shiftJIS = bs.readBit();
		bool smallText = bs.readBit();		// SWF 7.0+
		bool ansi = bs.readBit();
		bool wideOffsets = bs.readBit();
		bool wideCodes = bs.readBit();
		bool italic = bs.readBit();
		bool bold = bs.readBit();
		uint8_t languageCode = bs.readUInt8();	// SWF 6.0+
		std::string name = swf->readStringU8();
		
		uint16_t glyphCount = bs.readUInt16();
		if (!glyphCount)
			log::warning << L"Device fonts not supported; must embed fonts if used in dynamic fields" << Endl;

		uint32_t offsetBase = bs.getStream()->tell();

		std::vector< uint32_t > offsetTable(glyphCount);
		for (uint16_t i = 0; i < glyphCount; ++i)
			offsetTable[i] = wideOffsets ? bs.readUInt32() : bs.readUInt16();

		uint32_t codeOffset = wideOffsets ? bs.readUInt32() : bs.readUInt16();

		AlignedVector< SwfShape* > shapeTable(glyphCount);
		for (uint16_t i = 0; i < glyphCount; ++i)
		{
			T_ASSERT (offsetBase + offsetTable[i] < context.tagEndPosition);
			bs.getStream()->seek(Stream::SeekSet, offsetBase + offsetTable[i]);
			bs.alignByte();
			shapeTable[i] = swf->readShape(0);
		}

		uint32_t currentPosition = bs.getStream()->tell();
		T_ASSERT (offsetBase + codeOffset == currentPosition);

		AlignedVector< uint16_t > codeTable(glyphCount);
		for (uint16_t i = 0; i < glyphCount; ++i)
			codeTable[i] = wideCodes ? bs.readUInt16() : bs.readUInt8();

		int16_t ascent = 0, descent = 0, leading = 0;
		AlignedVector< int16_t > advanceTable;
		AlignedVector< SwfRect > boundsTable;
		AlignedVector< SwfKerningRecord > kerningTable;

		if (hasLayout)
		{
			ascent = bs.readInt16();
			descent = bs.readInt16();
			leading = bs.readInt16();

			advanceTable.resize(glyphCount);
			for (uint16_t i = 0; i < glyphCount; ++i)
				advanceTable[i] = bs.readInt16();

			boundsTable.resize(glyphCount);
			for (uint16_t i = 0; i < glyphCount; ++i)
				boundsTable[i] = swf->readRect();

			uint16_t kerningCount = bs.readUInt16();
			kerningTable.resize(kerningCount);
			for (uint16_t i = 0; i < kerningCount; ++i)
				kerningTable[i] = swf->readKerningRecord(wideCodes);
		}

		Ref< FlashFont > font = gc_new< FlashFont >();
		if (!font->create(
			shapeTable,
			ascent,
			descent,
			leading,
			advanceTable,
			boundsTable,
			kerningTable,
			codeTable,
			m_fontType == 3 ? FlashFont::CtEMSquare : FlashFont::CtTwips
		))
			return false;

		context.movie->defineFont(fontId, font);
	}

	return true;
}

// ============================================================================
// Define text

FlashTagDefineText::FlashTagDefineText(int textType)
:	m_textType(textType)
{
}

bool FlashTagDefineText::read(SwfReader* swf, ReadContext& context)
{
	BitReader& bs = swf->getBitReader();

	uint16_t textId = bs.readUInt16();
	SwfRect textBounds = swf->readRect();
	SwfMatrix textMatrix = swf->readMatrix();

	uint8_t numGlyphBits = bs.readUInt8();
	uint8_t numAdvanceBits = bs.readUInt8();

	AlignedVector< SwfTextRecord* > textRecords;
	for (;;)
	{
		SwfTextRecord* textRecord = swf->readTextRecord(numGlyphBits, numAdvanceBits, m_textType);
		if (!textRecord)
			return false;

		if (!textRecord->styleFlag && textRecord->glyph.glyphCount == 0)
			break;

		textRecords.push_back(textRecord);
	}
	
	Ref< FlashText > text = gc_new< FlashText >(textId, cref(textBounds), cref(Matrix33(textMatrix.m)));
	if (!text->create(textRecords))
		return false;

	context.movie->defineCharacter(textId, text);
	return true;
}

// ============================================================================
// Define edit text

bool FlashTagDefineEditText::read(SwfReader* swf, ReadContext& context)
{
	BitReader& bs = swf->getBitReader();

	uint16_t textId = bs.readUInt16();
	SwfRect textBounds = swf->readRect();
	SwfColor textColor = { 255, 255, 255, 255 };

	bs.alignByte();

	bool hasText = bs.readBit();
	bool wordwrap = bs.readBit();
	bool multiline = bs.readBit();
	bool password = bs.readBit();
	bool readonly = bs.readBit();
	bool hasColor = bs.readBit();
	bool hasMaxLength = bs.readBit();
	bool hasFont = bs.readBit();
	bool autoSize = false;
	if (context.version >= 6)
	{
		bs.skip(1);
		autoSize = bs.readBit();
	}
	else
		bs.skip(2);
	bool hasLayout = bs.readBit();
	bool noSelect = bs.readBit();
	bool border = bs.readBit();
	bs.skip(1);
	bool html = bs.readBit();
	bool useOutlines = bs.readBit();

	uint16_t fontId = 0;
	uint16_t fontHeight = 10;
	if (hasFont)
	{
		fontId = bs.readUInt16();
		fontHeight = bs.readUInt16();
	}
	if (hasColor)
		textColor = swf->readRgba();
	if (hasMaxLength)
	{
		uint16_t maxLength = bs.readUInt16();
	}

	uint8_t align = 0;
	uint16_t leftMargin = 0, rightMargin = 0;

	if (hasLayout)
	{
		align = bs.readUInt8();
		leftMargin = bs.readUInt16();
		rightMargin = bs.readUInt16();
		int16_t indent = bs.readInt16();
		int16_t leading = bs.readInt16();
	}

	std::wstring variableName = mbstows(swf->readString());
	std::wstring initialText;
	if (hasText)
		initialText = mbstows(swf->readString());

	Ref< FlashEdit > edit = gc_new< FlashEdit >(
		textId,
		fontId,
		fontHeight,
		cref(textBounds),
		cref(textColor),
		cref(initialText),
		(FlashEdit::Align)align,
		leftMargin,
		rightMargin,
		html
	);

	context.movie->defineCharacter(textId, edit);
	return true;
}

// ============================================================================
// Define button

FlashTagDefineButton::FlashTagDefineButton(int buttonType)
:	m_buttonType(buttonType)
{
}

bool FlashTagDefineButton::read(SwfReader* swf, ReadContext& context)
{
	BitReader& bs = swf->getBitReader();

	uint16_t buttonId = bs.readUInt16();
	Ref< FlashButton > button = gc_new< FlashButton >(buttonId);

	if (m_buttonType == 1)
	{
		// @fixme Not implemented.
		log::error << L"FlashTagDefineButton : Not implemented" << Endl;
		return false;
	}
	else if (m_buttonType == 2)
	{
		bs.skip(7);

		bool trackAsMenu = bs.readBit();
		uint16_t actionOffset = bs.readUInt16();

		// Read button characters.
		for (;;)
		{
			bs.alignByte();
			bs.skip(2);

			bool hasBlendMode = false;
			bool hasFilterList = false;

			if (context.version >= 8)
			{
				hasBlendMode = bs.readBit();
				hasFilterList = bs.readBit();
			}
			else
				bs.skip(2);

			FlashButton::ButtonLayer layer;

			layer.state |= bs.readBit() ? FlashButton::SmHitTest : 0;
			layer.state |= bs.readBit() ? FlashButton::SmDown : 0;
			layer.state |= bs.readBit() ? FlashButton::SmOver: 0;
			layer.state |= bs.readBit() ? FlashButton::SmUp : 0;

			if (!layer.state)
				break;

			layer.characterId = bs.readUInt16();
			layer.placeDepth = bs.readUInt16();
			layer.placeMatrix = Matrix33(swf->readMatrix().m);

			if (m_buttonType == 2)
				layer.cxform = swf->readCxTransform(true);

			if (hasFilterList)
			{
				AlignedVector< SwfFilter* > filterList;
				if (!swf->readFilterList(filterList))
					return false;
			}
			if (hasBlendMode)
			{
				uint8_t blendMode = bs.readUInt8();
			}

			button->addButtonLayer(layer);
		}

		// Read conditions.
		bs.alignByte();
		while (uint32_t(bs.getStream()->tell()) < context.tagEndPosition)
		{
			uint16_t conditionLength = bs.readUInt16();

			FlashButton::ButtonCondition condition;

			condition.mask |= bs.readBit() ? FlashButton::CmIdleToOverDown : 0;
			condition.mask |= bs.readBit() ? FlashButton::CmOutDownToIdle : 0;
			condition.mask |= bs.readBit() ? FlashButton::CmOutDownToOverDown : 0;
			condition.mask |= bs.readBit() ? FlashButton::CmOverDownToOutDown : 0;
			condition.mask |= bs.readBit() ? FlashButton::CmOverDownToOverUp : 0;
			condition.mask |= bs.readBit() ? FlashButton::CmOverUpToOverDown : 0;
			condition.mask |= bs.readBit() ? FlashButton::CmOverUpToIdle : 0;
			condition.mask |= bs.readBit() ? FlashButton::CmIdleToOverUp : 0;

			condition.key = bs.readUnsigned(7);

			condition.mask |= bs.readBit() ? FlashButton::CmOverDownToIdle : 0;

			condition.script = readActionScript(bs);
			bs.alignByte();

			button->addButtonCondition(condition);

			if (!conditionLength)
				break;
		}
	}

	context.movie->defineCharacter(buttonId, button);
	return true;
}

// ============================================================================
// JPEGTables

bool FlashTagJpegTables::read(SwfReader* swf, ReadContext& context)
{
	context.jpegFormat = gc_new< drawing::ImageFormatJpeg >();

	// Strange, seems Flash CS3 emits a 0 length tag, assuming
	// it's valid and there is only a single JPEG in the SWF which
	// will contain the JPEG header locally.
	if (context.tagSize == 0)
		return true;

	BitReader& bs = swf->getBitReader();

	// Read entire tag's content into memory buffer first, need to correct SWF
	// bugs in data.
	std::vector< uint8_t > buffer(context.tagSize);
	bs.getStream()->read(&buffer[0], context.tagSize);

	// Prior to SWF 8.0 there could be a problem with incorrect JFIF start tag->
	if (buffer[0] == 0xff && buffer[1] == 0xd9 && buffer[2] == 0xff && buffer[3] == 0xd8)
	{
		buffer[1] = 0xd8;
		buffer[3] = 0xd9;
	}

	// Ensure data appears to be correct(ed).
	T_ASSERT (buffer[0] == 0xff && buffer[1] == 0xd8);

	MemoryStream bufferStream(&buffer[0], int(buffer.size()), true, false);
	context.jpegFormat->readJpegHeader(&bufferStream);

	return true;
}

// ============================================================================
// Define bits jpeg

FlashTagDefineBitsJpeg::FlashTagDefineBitsJpeg(int bitsType)
:	m_bitsType(bitsType)
{
}

bool FlashTagDefineBitsJpeg::read(SwfReader* swf, ReadContext& context)
{
	BitReader& bs = swf->getBitReader();

	uint16_t bitmapId = bs.readUInt16();

	uint32_t tagSize = context.tagSize - sizeof(uint16_t) - sizeof(uint32_t);
	uint32_t offsetToAlpha = tagSize;
	if (m_bitsType == 3)
		offsetToAlpha = bs.readUInt32();

	// Read entire tag's content into memory buffer first, need to correct SWF
	// bugs in data.
	AutoArrayPtr< uint8_t > buffer(new uint8_t [tagSize]);
	if (bs.getStream()->read(buffer.ptr(), tagSize) != tagSize)
		return false;

	// Prior to SWF 8.0 there could be a problem with incorrect JFIF start tag->
	if (buffer[0] == 0xff && buffer[1] == 0xd9 && buffer[2] == 0xff && buffer[3] == 0xd8)
	{
		buffer[1] = 0xd8;
		buffer[3] = 0xd9;
	}

	// Ensure data appears to be correct(ed).
	T_ASSERT (buffer[0] == 0xff && buffer[1] == 0xd8);
	MemoryStream bufferStream(buffer.ptr(), tagSize, true, false);

	// Decode JPEG image, either by using previously defined encoding tables
	// or, new method, with own encoding tables.
	if (m_bitsType == 1)
	{
		T_ASSERT (context.jpegFormat);

		Ref< drawing::Image > image = context.jpegFormat->readJpegImage(&bufferStream);
		T_ASSERT (image);

		Ref< FlashBitmap > bitmap = gc_new< FlashBitmap >();
		if (!bitmap->create(image))
			return false;

		context.movie->defineBitmap(bitmapId, bitmap);
	}
	else
	{
		Ref< drawing::ImageFormatJpeg > jpegFormat = gc_new< drawing::ImageFormatJpeg >();
		if (!jpegFormat->readJpegHeader(&bufferStream))
			return false;

		Ref< drawing::Image > image;

		if (bufferStream.available() > 0)
		{
			image = jpegFormat->readJpegImage(&bufferStream);
			if (!image)
				return false;
		}

		if (m_bitsType == 3 && offsetToAlpha < tagSize)
		{
			uint32_t inflateSize = image->getWidth() * image->getHeight();

			bufferStream.seek(Stream::SeekSet, offsetToAlpha);
			zip::InflateStream inf(&bufferStream);

			AutoArrayPtr< uint8_t > alphaBuffer(new uint8_t [inflateSize]);
			if (inf.read(alphaBuffer.ptr(), inflateSize) != inflateSize)
				return false;

#if defined(T_LITTLE_ENDIAN)
			image->convert(drawing::PixelFormat::getA8B8G8R8());
#else	// T_BIG_ENDIAN
			image->convert(drawing::PixelFormat::getR8G8B8A8());
#endif

			uint8_t* bits = static_cast< uint8_t* >(image->getData());
			for (uint32_t i = 0; i < inflateSize; ++i)
				bits[i * 4 + 3] = alphaBuffer[i];
		}

		if (image)
		{
			Ref< FlashBitmap > bitmap = gc_new< FlashBitmap >();
			if (!bitmap->create(image))
				return false;

			context.movie->defineBitmap(bitmapId, bitmap);
		}
	}

	return true;
}

// ============================================================================
// Define bits loss less

FlashTagDefineBitsLossLess::FlashTagDefineBitsLossLess(int bitsType)
:	m_bitsType(bitsType)
{
}

bool FlashTagDefineBitsLossLess::read(SwfReader* swf, ReadContext& context)
{
	BitReader& bs = swf->getBitReader();

	uint16_t bitmapId = bs.readUInt16();
	uint8_t format = bs.readUInt8();
	uint16_t width = bs.readUInt16();
	uint16_t height = bs.readUInt16();

	if (format == 3)	// Palette
	{
		uint8_t colorCount = bs.readUInt8() + 1;
		uint8_t colorSize = (m_bitsType == 1) ? 3 : 4;
		uint32_t colorTableSize = colorSize * colorCount;
		uint32_t imagePitch = (width + 3UL) & ~3UL;

		uint32_t bufferSize = colorTableSize + imagePitch * height;
		AutoArrayPtr< uint8_t > buffer(new uint8_t [bufferSize]);
#if defined(_DEBUG)
		std::memset(buffer.ptr(), 0, bufferSize);
#endif

		zip::InflateStream inflateStream(bs.getStream());
		inflateStream.read(&buffer[0], int(bufferSize));

		bs.getStream()->seek(Stream::SeekSet, context.tagEndPosition);
		bs.alignByte();

		Ref< FlashBitmap > bitmap = gc_new< FlashBitmap >();
		if (!bitmap->create(width, height))
			return false;

		for (uint16_t y = 0; y < height; ++y)
		{
			uint8_t* src = &buffer[colorTableSize + y * imagePitch];
			for (uint16_t x = 0; x < width; ++x)
			{
				SwfColor color;
				color.red = buffer[src[x] * colorSize + 0];
				color.green = buffer[src[x] * colorSize + 1];
				color.blue = buffer[src[x] * colorSize + 2];
				color.alpha = (colorSize == 4) ? buffer[src[x] * colorSize + 3] : 255;
				bitmap->setPixel(x, y, color);
			}
		}

		context.movie->defineBitmap(bitmapId, bitmap);
	}
	else if (format == 4 || format == 5)	// RGB/RGBA
	{
		uint8_t colorSize;
		if (m_bitsType == 1)
		{
			if (format == 4)
				colorSize = 2;	// 1555
			else
				colorSize = 4;	// 888
		}
		else
			colorSize = 4;	// 8888

		uint32_t imagePitch = width * colorSize;
		imagePitch = (imagePitch + 3UL) & ~3UL;

		uint32_t bufferSize = imagePitch * height;
		AutoArrayPtr< uint8_t > buffer(new uint8_t [bufferSize]);
#if defined(_DEBUG)
		std::memset(buffer.ptr(), 0, bufferSize);
#endif

		zip::InflateStream inflateStream(bs.getStream());
		inflateStream.read(&buffer[0], int(bufferSize));

		bs.getStream()->seek(Stream::SeekSet, context.tagEndPosition);
		bs.alignByte();

		Ref< FlashBitmap > bitmap = gc_new< FlashBitmap >();
		if (!bitmap->create(width, height))
			return false;

		for (uint16_t y = 0; y < height; ++y)
		{
			uint8_t* src = &buffer[y * imagePitch];
			for (uint16_t x = 0; x < width; ++x)
			{
				SwfColor color;
				if (colorSize == 2)
				{
					uint16_t& c = reinterpret_cast< uint16_t& >(src[x * 2]);
#if defined(T_BIG_ENDIAN)
					swap8in16(c);
#endif
					color.alpha = 255;
					color.red = (c >> 7) & 0xf8;
					color.green = (c >> 2) & 0xf8;
					color.blue = (c << 3) & 0xf8;
				}
				else
				{
					color.alpha = (m_bitsType > 1) ? src[x * 4 + 0] : 255;
					color.red = src[x * 4 + 1];
					color.green = src[x * 4 + 2];
					color.blue = src[x * 4 + 3];
				}
				bitmap->setPixel(x, y, color);
			}
		}

		context.movie->defineBitmap(bitmapId, bitmap);
	}

	return true;
}

// ============================================================================
// Define sprite

bool FlashTagDefineSprite::read(SwfReader* swf, ReadContext& context)
{
	BitReader& bs = swf->getBitReader();

	uint16_t spriteId = bs.readUInt16();
	uint16_t frameCount = bs.readUInt16();

	Ref< FlashSprite > sprite = gc_new< FlashSprite >(spriteId, frameCount);

	// Setup readers for supported sprite tags.
	std::map< uint16_t, Ref< FlashTag > > tagReaders;
	tagReaders[TiDoAction] = gc_new< FlashTagDoAction >();
	tagReaders[TiPlaceObject] = gc_new< FlashTagPlaceObject >(1);
	tagReaders[TiPlaceObject2] = gc_new< FlashTagPlaceObject >(2);
	tagReaders[TiPlaceObject3] = gc_new< FlashTagPlaceObject >(3);
	tagReaders[TiRemoveObject] = gc_new< FlashTagRemoveObject >(1);
	tagReaders[TiRemoveObject2] = gc_new< FlashTagRemoveObject >(2);
	tagReaders[TiShowFrame] = gc_new< FlashTagShowFrame >();
	tagReaders[TiFrameLabel] = gc_new< FlashTagFrameLabel >();

	// Define readers for tags which isn't planed to be implemented.
	tagReaders[13] = gc_new< FlashTagUnsupported >(13);
	tagReaders[14] = gc_new< FlashTagUnsupported >(14);
	tagReaders[15] = gc_new< FlashTagUnsupported >(15);
	tagReaders[18] = gc_new< FlashTagUnsupported >(18);
	tagReaders[19] = gc_new< FlashTagUnsupported >(19);
	tagReaders[45] = gc_new< FlashTagUnsupported >(45);

	// Decode tags.
	FlashTag::ReadContext spriteContext;
	spriteContext.version = context.version;
	spriteContext.movie = context.movie;
	spriteContext.sprite = sprite;
	spriteContext.frame = gc_new< FlashFrame >();
	for (;;)
	{
		swf->enterScope();

		SwfTag* tag = swf->readTag();
		if (!tag || tag->id == TiEnd)
			break;

		Ref< FlashTag > tagReader = tagReaders[tag->id];
		if (tagReader)
		{
			spriteContext.tagSize = tag->length;
			spriteContext.tagEndPosition = swf->getBitReader().getStream()->tell() + tag->length;
			if (!tagReader->read(swf, spriteContext))
			{
				log::error << L"Unable to read flash, error when reading tag " << int32_t(tag->id) << Endl;
				return false;
			}
			if (uint32_t(swf->getBitReader().getStream()->tell()) < spriteContext.tagEndPosition)
			{
				log::warning << L"Read too few bytes (" << spriteContext.tagEndPosition - swf->getBitReader().getStream()->tell() << L" left) in tag " << int32_t(tag->id) << Endl;
				swf->getBitReader().getStream()->seek(Stream::SeekSet, spriteContext.tagEndPosition);
			}
			else if (uint32_t(swf->getBitReader().getStream()->tell()) > spriteContext.tagEndPosition)
			{
				log::error << L"Read too many bytes (" << swf->getBitReader().getStream()->tell() - spriteContext.tagEndPosition << L") in tag " << int32_t(tag->id) << Endl;
				swf->getBitReader().getStream()->seek(Stream::SeekSet, spriteContext.tagEndPosition);
			}
		}
		else
		{
			log::warning << L"Invalid sprite tag " << tag->id << Endl;
			swf->getBitReader().skip(tag->length * 8);
		}

		swf->leaveScope();
	}

	context.movie->defineCharacter(spriteId, sprite);
	return true;
}

// ============================================================================
// Place object

FlashTagPlaceObject::FlashTagPlaceObject(int placeType)
:	m_placeType(placeType)
{
}

bool FlashTagPlaceObject::read(SwfReader* swf, ReadContext& context)
{
	BitReader& bs = swf->getBitReader();

	if (m_placeType == 1)
	{
		FlashFrame::PlaceObject placeObject;
		
		placeObject.hasCharacterId = true;
		placeObject.characterId = bs.readUInt16();
		placeObject.depth = bs.readUInt16();
		placeObject.hasMatrix = true;
		placeObject.matrix = Matrix33(swf->readMatrix().m);

		if (uint32_t(bs.getStream()->tell()) < context.tagEndPosition)
		{
			placeObject.hasCxTransform = true;
			placeObject.cxTransform = swf->readCxTransform(false);
		}

		context.frame->placeObject(placeObject);
	}
	else if (m_placeType == 2 || m_placeType == 3)
	{
		FlashFrame::PlaceObject placeObject;

		if (context.version >= 5)
			placeObject.hasActions = bs.readBit();
		else
			bs.skip(1);

		placeObject.hasClipDepth = bs.readBit();
		placeObject.hasName = bs.readBit();
		placeObject.hasRatio = bs.readBit();
		placeObject.hasCxTransform = bs.readBit();
		placeObject.hasMatrix = bs.readBit();
		placeObject.hasCharacterId = bs.readBit();
		placeObject.hasMove = bs.readBit();

		if (m_placeType == 2)
		{
			placeObject.depth = bs.readUInt16();
		}
		else if (m_placeType == 3)
		{
			bs.skip(3);

			bool hasImage = bs.readBit();
			bool hasClassName = bs.readBit();
			placeObject.hasBitmapCaching = bs.readBit();
			placeObject.hasBlendMode = bs.readBit();
			placeObject.hasFilters = bs.readBit();
			placeObject.depth = bs.readUInt16();

			if (hasClassName || (hasImage && placeObject.hasCharacterId))
			{
				std::string className = swf->readString();
				log::warning << L"Unused class name " << mbstows(className) << L" in PlaceObject" << Endl;
			}
		}
	
		if (placeObject.hasCharacterId)
			placeObject.characterId = bs.readUInt16();

		if (placeObject.hasMatrix)
			placeObject.matrix = Matrix33(swf->readMatrix().m);

		if (placeObject.hasCxTransform)
			placeObject.cxTransform = swf->readCxTransform(true);

		if (placeObject.hasRatio)
			placeObject.ratio = bs.readUInt16();

		if (placeObject.hasName)
			placeObject.name = mbstows(swf->readString());

		if (placeObject.hasClipDepth)
			placeObject.clipDepth = bs.readUInt16();

		if (m_placeType == 3)
		{
			if (placeObject.hasFilters)
			{
				AlignedVector< SwfFilter* > filterList;
				if (!swf->readFilterList(filterList))
					return false;
			}

			if (placeObject.hasBlendMode)
				placeObject.blendMode = bs.readUInt8();

			if (placeObject.hasBitmapCaching)
				placeObject.bitmapCaching = bs.readUInt8();
		}

		if (placeObject.hasActions)
		{
			uint16_t reserved = bs.readUInt16();
			if (reserved)
				log::warning << L"Reserved field in PlaceObject actions not zero, " << reserved << Endl;

			uint32_t allFlags;
			if (context.version >= 6)
				allFlags = bs.readUInt32();
			else
				allFlags = (bs.readUInt16() << 16);

			while (uint32_t(bs.getStream()->tell()) < context.tagEndPosition)
			{
				FlashFrame::PlaceAction placeAction;

				placeAction.eventMask = context.version >= 6 ? bs.readUInt32() : (bs.readUInt16() << 16);
				if (!placeAction.eventMask)
					break;

				uint32_t eventLength = bs.readUInt32();
				if (uint32_t(bs.getStream()->tell()) + eventLength >= context.tagEndPosition)
				{
					log::error << L"PlaceObject, incorrect number of bytes in event tag" << Endl;
					break;
				}

				// "Key Press" events.
				if (placeAction.eventMask & EvtKeyPress)
				{
					uint8_t keyCode = bs.readUInt8();
					log::debug << L"PlaceObject, unused keycode in EvtKeyPress" << Endl;
				}

				placeAction.script = readActionScript(bs);
				bs.alignByte();

				placeObject.actions.push_back(placeAction);
			}
		}

		context.frame->placeObject(placeObject);
	}

	return true;
}

// ============================================================================
// Remove object

FlashTagRemoveObject::FlashTagRemoveObject(int removeType)
:	m_removeType(removeType)
{
}

bool FlashTagRemoveObject::read(SwfReader* swf, ReadContext& context)
{
	FlashFrame::RemoveObject removeObject;
	if (m_removeType == 1)
	{
		removeObject.hasCharacterId = true;
		removeObject.characterId = swf->getBitReader().readUInt16();
		removeObject.depth = swf->getBitReader().readUInt16();
	}
	else if (m_removeType == 2)
	{
		removeObject.hasCharacterId = false;
		removeObject.depth = swf->getBitReader().readUInt16();
	}
	context.frame->removeObject(removeObject);
	return true;
}

// ============================================================================
// Show frame

bool FlashTagShowFrame::read(SwfReader* swf, ReadContext& context)
{
	T_ASSERT (context.frame);
	context.sprite->addFrame(context.frame);
	context.frame = gc_new< FlashFrame >();
	return true;
}

// ============================================================================
// Do action

bool FlashTagDoAction::read(SwfReader* swf, ReadContext& context)
{
	BitReader& bs = swf->getBitReader();

	Ref< ActionScript > script = readActionScript(bs);
	if (script)
		context.frame->addActionScript(script);

	return true;
}

// ============================================================================
// Export assets

bool FlashTagExportAssets::read(SwfReader* swf, ReadContext& context)
{
	BitReader& bs = swf->getBitReader();

	uint16_t exportCount = bs.readUInt16();
	for (int i = 0; i < exportCount; ++i)
	{
		uint16_t id = bs.readUInt16();
		std::wstring symbol = mbstows(swf->readString());
		context.movie->setExport(symbol, id);
	}

	return true;
}

// ============================================================================
// Import assets

FlashTagImportAssets::FlashTagImportAssets(int importType)
:	m_importType(importType)
{
}

bool FlashTagImportAssets::read(SwfReader* swf, ReadContext& context)
{
	BitReader& bs = swf->getBitReader();

	std::string url = swf->readString();

	if (m_importType == 2)
	{
		uint8_t version = bs.readUInt8();
		uint8_t reserved = bs.readUInt8();
	}

	uint16_t count = bs.readUInt16();
	for (int i = 0; i < count; ++i)
	{
		uint16_t id = bs.readUInt16();
		std::string symbol = swf->readString();

		log::info << L"Import symbol \"" << mbstows(symbol) << L"\" as " << id << L" from URL \"" << mbstows(url) << L"\"" << Endl;
	}

	return true;
}

// ============================================================================
// Init action

bool FlashTagInitAction::read(SwfReader* swf, ReadContext& context)
{
	BitReader& bs = swf->getBitReader();

	uint16_t spriteId = bs.readUInt16();

	Ref< ActionScript > script = readActionScript(bs);
	bs.alignByte();

	context.sprite->addInitActionScript(script);
	return true;
}

// ============================================================================
// Protect

FlashTagProtect::FlashTagProtect(int protectType)
{
}

bool FlashTagProtect::read(SwfReader* swf, ReadContext& context)
{
	BitReader& bs = swf->getBitReader();
	bs.getStream()->seek(Stream::SeekSet, context.tagEndPosition);
	bs.alignByte();
	return true;
}

// ============================================================================
// Frame label

bool FlashTagFrameLabel::read(SwfReader* swf, ReadContext& context)
{
	std::string label = swf->readString();
	if (context.frame)
		context.frame->setLabel(mbstows(label));
	return true;
}

// ============================================================================
// Unsupported

FlashTagUnsupported::FlashTagUnsupported(int32_t tagId)
:	m_tagId(tagId)
,	m_visited(false)
{
}

bool FlashTagUnsupported::read(SwfReader* swf, ReadContext& context)
{
	if (!m_visited)
	{
		log::debug << L"Tag " << m_tagId << L" unsupported" << Endl;
		m_visited = true;
	}

	BitReader& bs = swf->getBitReader();
	bs.getStream()->seek(Stream::SeekSet, context.tagEndPosition);
	bs.alignByte();

	return true;
}

	}
}
