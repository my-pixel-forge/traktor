#include "Drawing/PixelFormat.h"
#include "Drawing/Palette.h"
#include "Core/Math/MathUtils.h"

namespace traktor
{
	namespace drawing
	{
		namespace
		{

inline float clamp(float v)
{
	return min(max(v, 0.0f), 1.0f);
}

#if defined(T_LITTLE_ENDIAN)

uint32_t unpack(const void* p, int byteSize)
{
	const uint8_t* c = static_cast< const uint8_t* >(p);
	switch (byteSize)
	{
	case 1:
		return c[0];
	case 2:
		return (c[1] << 8) | c[0];
	case 3:
		return (c[2] << 16) | (c[1] << 8) | c[0];
	case 4:
		return (c[3] << 24) | (c[2] << 16) | (c[1] << 8) | c[0];
	}
	return 0;
}

void pack(void* p, int byteSize, uint32_t v)
{
	uint8_t* d = static_cast< uint8_t* >(p);
	switch (byteSize)
	{
	case 4:
		d[3] = (v >> 24) & 255;
	case 3:
		d[2] = (v >> 16) & 255;
	case 2:
		d[1] = (v >> 8) & 255;
	case 1:
		d[0] = v & 255;
	}
}

#elif defined(T_BIG_ENDIAN)

uint32_t unpack(const void* p, int byteSize)
{
	const uint8_t* c = static_cast< const uint8_t* >(p);
	switch (byteSize)
	{
	case 1:
		return c[0];
	case 2:
		return (c[0] << 8) | c[1];
	case 3:
		return (c[0] << 16) | (c[1] << 8) | c[2];
	case 4:
		return (c[0] << 24) | (c[1] << 16) | (c[2] << 8) | c[3];
	}
	return 0;
}

void pack(void* p, int byteSize, uint32_t v)
{
	uint8_t* d = static_cast< uint8_t* >(p);
	switch (byteSize)
	{
	case 4:
		d[0] = (v >> 24) & 255;
		d[1] = (v >> 16) & 255;
		d[2] = (v >> 8) & 255;
		d[3] = v & 255;
		break;
	case 3:
		d[0] = (v >> 16) & 255;
		d[1] = (v >> 8) & 255;
		d[2] = v & 255;
		break;
	case 2:
		d[0] = (v >> 8) & 255;
		d[1] = v & 255;
		break;
	case 1:
		d[0] = v & 255;
		break;
	}
}

#endif

		}

PixelFormat::PixelFormat()
:	m_palettized(false)
,	m_floatPoint(false)
,	m_colorBits(32)
,	m_byteSize(4)
,	m_redBits(8)
,	m_redShift(16)
,	m_greenBits(8)
,	m_greenShift(8)
,	m_blueBits(8)
,	m_blueShift(0)
,	m_alphaBits(8)
,	m_alphaShift(24)
{
}

PixelFormat::PixelFormat(
	int colorBits,
	uint32_t redBits,
	uint32_t redShift,
	uint32_t greenBits,
	uint32_t greenShift,
	uint32_t blueBits,
	uint32_t blueShift,
	uint32_t alphaBits,
	uint32_t alphaShift,
	bool palettized,
	bool floatPoint
)
:	m_palettized(palettized)
,	m_floatPoint(floatPoint)
,	m_colorBits(colorBits)
,	m_byteSize((colorBits + 7) / 8)
,	m_redBits(redBits)
,	m_redShift(redShift)
,	m_greenBits(greenBits)
,	m_greenShift(greenShift)
,	m_blueBits(blueBits)
,	m_blueShift(blueShift)
,	m_alphaBits(alphaBits)
,	m_alphaShift(alphaShift)
{
}	
		
PixelFormat::PixelFormat(
	int colorBits,
	uint32_t redMask,
	uint32_t greenMask,
	uint32_t blueMask,
	uint32_t alphaMask,
	bool palettized,
	bool floatPoint
)
:	m_palettized(palettized)
,	m_floatPoint(floatPoint)
,	m_colorBits(colorBits)
,	m_byteSize((colorBits + 7) / 8)
,	m_redBits(0)
,	m_redShift(0)
,	m_greenBits(0)
,	m_greenShift(0)
,	m_blueBits(0)
,	m_blueShift(0)
,	m_alphaBits(0)
,	m_alphaShift(0)
{
	for (int i = 0; i < colorBits; ++i)
	{
		uint32_t bit = 1 << i;
		if ((redMask & bit) == bit)
		{
			if (m_redBits++ == 0)
				m_redShift = i;
		}
		if ((greenMask & bit) == bit)
		{
			if (m_greenBits++ == 0)
				m_greenShift = i;
		}
		if ((blueMask & bit) == bit)
		{
			if (m_blueBits++ == 0)
				m_blueShift = i;
		}
		if ((alphaMask & bit) == bit)
		{
			if (m_alphaBits++ == 0)
				m_alphaShift = i;
		}
	}
}

void PixelFormat::convert(
	const Palette* srcPalette,
	const void* srcPixels,
	const PixelFormat& dstFormat,
	const Palette* dstPalette,
	void* dstPixels,
	int pixelCount
) const
{
	const uint8_t* src = static_cast< const uint8_t* >(srcPixels);
	uint8_t* dst = static_cast< uint8_t* >(dstPixels);
	uint32_t tmp;
	uint32_t i;
	Color4f clr;

	for (int ii = 0; ii < pixelCount; ++ii)
	{
		// src => rgba
		if (isPalettized())
		{
			uint32_t s = unpack(src, m_byteSize);
			clr = srcPalette->get(s);
		}
		else if (isFloatPoint())
		{
			clr.set(
				*(const float *)&src[getRedShift()   >> 3],
				*(const float *)&src[getGreenShift() >> 3],
				*(const float *)&src[getBlueShift()  >> 3],
				*(const float *)&src[getAlphaShift() >> 3]
			);
		}
		else if (getColorBits() <= 32)
		{
			uint32_t s = unpack(src, m_byteSize);

			uint32_t rmx = ((1 << getRedBits()  ) - 1);
			uint32_t gmx = ((1 << getGreenBits()) - 1);
			uint32_t bmx = ((1 << getBlueBits() ) - 1);
			uint32_t amx = ((1 << getAlphaBits()) - 1);

			uint32_t r = (s >> getRedShift()) & rmx;
			uint32_t g = (s >> getGreenShift()) & gmx;
			uint32_t b = (s >> getBlueShift()) & bmx;
			uint32_t a = (s >> getAlphaShift()) & amx;

			clr.set(
				rmx ? (float(r) / rmx) : 0.0f,
				gmx ? (float(g) / gmx) : 0.0f,
				bmx ? (float(b) / bmx) : 0.0f,
				amx ? (float(a) / amx) : 0.0f
			);
		}
		else	// getColorBits() > 32
		{
			for (tmp = i = 0; i < uint32_t(getRedBits()); ++i)
			{
				uint32_t o = i + getRedShift();
				if ((src[o >> 3] & (1 << (o & 7))) != 0)
					tmp |= 1 << (i + 8 - getRedBits());
			}
			clr.setRed(Scalar(tmp / 255.0f));

			for (tmp = i = 0; i < uint32_t(getGreenBits()); ++i)
			{
				uint32_t o = i + getGreenShift();
				if ((src[o >> 3] & (1 << (o & 7))) != 0)
					tmp |= 1 << (i + 8 - getGreenBits());
			}
			clr.setGreen(Scalar(tmp / 255.0f));

			for (tmp = i = 0; i < uint32_t(getBlueBits()); ++i)
			{
				uint32_t o = i + getBlueShift();
				if ((src[o >> 3] & (1 << (o & 7))) != 0)
					tmp |= 1 << (i + 8 - getBlueBits());
			}
			clr.setBlue(Scalar(tmp / 255.0f));

			for (tmp = i = 0; i < uint32_t(getAlphaBits()); ++i)
			{
				uint32_t o = i + getAlphaShift();
				if ((src[o >> 3] & (1 << (o & 7))) != 0)
					tmp |= 1 << (i + 8 - getAlphaBits());
			}
			clr.setAlpha(Scalar(tmp / 255.0f));
		}

		// rgba => dst
		if (dstFormat.isPalettized())
		{
			pack(
				dst,
				dstFormat.getByteSize(),
				dstPalette->find(clr)
			);
		}
		else if (dstFormat.isFloatPoint())
		{
			*(float *)&dst[dstFormat.getRedShift()   >> 3] = clr.getRed();
			*(float *)&dst[dstFormat.getGreenShift() >> 3] = clr.getGreen();
			*(float *)&dst[dstFormat.getBlueShift()  >> 3] = clr.getBlue();
			*(float *)&dst[dstFormat.getAlphaShift() >> 3] = clr.getAlpha();
		}
		else if (dstFormat.getColorBits() <= 32)
		{
			uint32_t rmx = ((1 << dstFormat.getRedBits()  ) - 1);
			uint32_t gmx = ((1 << dstFormat.getGreenBits()) - 1);
			uint32_t bmx = ((1 << dstFormat.getBlueBits() ) - 1);
			uint32_t amx = ((1 << dstFormat.getAlphaBits()) - 1);

			uint32_t r = uint32_t(clamp(clr.getRed()) * rmx);
			uint32_t g = uint32_t(clamp(clr.getGreen()) * gmx);
			uint32_t b = uint32_t(clamp(clr.getBlue()) * bmx);
			uint32_t a = uint32_t(clamp(clr.getAlpha()) * amx);

			pack(
				dst,
				dstFormat.getByteSize(),
				(r << dstFormat.getRedShift()) |
				(g << dstFormat.getGreenShift()) |
				(b << dstFormat.getBlueShift()) |
				(a << dstFormat.getAlphaShift())
			);
		}
		else	// dstFormat.getColorBits() > 32
		{
			tmp = static_cast< uint8_t >(clamp(clr.getRed()) * 255);
			tmp >>= (8 - dstFormat.getRedBits());
			for (i = 0; i < uint32_t(dstFormat.getRedBits()); ++i)
			{
				uint32_t o = i + dstFormat.getRedShift();
				if ((tmp & (1 << i)) != 0)
					dst[o >> 3] |= 1 << (o & 7);
				else
					dst[o >> 3] &= ~(1 << (o & 7));
			}
		
			tmp = static_cast< uint8_t >(clamp(clr.getGreen()) * 255);
			tmp >>= (8 - dstFormat.getGreenBits());
			for (i = 0; i < uint32_t(dstFormat.getGreenBits()); ++i)
			{
				uint32_t o = i + dstFormat.getGreenShift();
				if ((tmp & (1 << i)) != 0)
					dst[o >> 3] |= 1 << (o & 7);
				else
					dst[o >> 3] &= ~(1 << (o & 7));
			}
			
			tmp = static_cast< uint8_t >(clamp(clr.getBlue()) * 255);
			tmp >>= (8 - dstFormat.getBlueBits());
			for (i = 0; i < uint32_t(dstFormat.getBlueBits()); ++i)
			{
				uint32_t o = i + dstFormat.getBlueShift();
				if ((tmp & (1 << i)) != 0)
					dst[o >> 3] |= 1 << (o & 7);
				else
					dst[o >> 3] &= ~(1 << (o & 7));
			}
			
			tmp = static_cast< uint8_t >(clamp(clr.getAlpha()) * 255);
			tmp >>= (8 - dstFormat.getAlphaBits());
			for (i = 0; i < uint32_t(dstFormat.getAlphaBits()); ++i)
			{
				uint32_t o = i + dstFormat.getAlphaShift();
				if ((tmp & (1 << i)) != 0)
					dst[o >> 3] |= 1 << (o & 7);
				else
					dst[o >> 3] &= ~(1 << (o & 7));
			}
		}

		// Next pixel
		src += getByteSize();
		dst += dstFormat.getByteSize();
	}
}

bool PixelFormat::operator == (const PixelFormat& pf) const
{
	if (m_palettized != pf.m_palettized)
		return false;
	if (m_floatPoint != pf.m_floatPoint)
		return false;
	if (m_colorBits != pf.m_colorBits)
		return false;
	if (m_byteSize != pf.m_byteSize)
		return false;
	if (m_redBits != pf.m_redBits)
		return false;
	if (m_redShift != pf.m_redShift)
		return false;
	if (m_greenBits != pf.m_greenBits)
		return false;
	if (m_greenShift != pf.m_greenShift)
		return false;
	if (m_blueBits != pf.m_blueBits)
		return false;
	if (m_blueShift != pf.m_blueShift)
		return false;
	if (m_alphaBits != pf.m_alphaBits)
		return false;
	if (m_alphaShift != pf.m_alphaShift)
		return false;

	return true;
}

bool PixelFormat::operator != (const PixelFormat& pf) const
{
	return !(*this == pf);
}

	}
}
