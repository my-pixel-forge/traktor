#include "Core/Io/FileSystem.h"
#include "Core/Misc/Adler32.h"
#include "Core/Misc/String.h"
#include "Drawing/Image.h"
#include "Drawing/Filters/ScaleFilter.h"
#include "Drawing/Filters/SwizzleFilter.h"
#include "Editor/App/ThumbnailGenerator.h"

namespace traktor
{
	namespace editor
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.editor.ThumbnailGenerator", ThumbnailGenerator, IThumbnailGenerator)

ThumbnailGenerator::ThumbnailGenerator(const Path& thumbsPath)
:	m_thumbsPath(thumbsPath)
{
}

Ref< drawing::Image > ThumbnailGenerator::get(const Path& fileName, int32_t width, int32_t height, AlphaMode alphaMode)
{
	const wchar_t* alphaPrefix[] = { L"o", L"a", L"ao" };
	std::wstring pathName = fileName.getPathName();

	// Stat source file.
	Ref< File > file = FileSystem::getInstance().get(fileName);
	if (!file)
		return nullptr;

	// Generate checksum of full path to source image.
	Adler32 adler;
	adler.begin();
	adler.feed(pathName.c_str(), pathName.length());
	adler.end();

	Path thumbFileName =
		m_thumbsPath.getPathName() + L"/" +
		fileName.getFileNameNoExtension() + L"_" +
		toString(adler.get()) + L"_" +
		toString(width) + L"x" + toString(height) + L"_" +
		alphaPrefix[int32_t(alphaMode)] +
		toString(file->getLastWriteTime().getSecondsSinceEpoch()) +
		L".png";

	if (FileSystem::getInstance().exist(thumbFileName))
	{
		// Cached thumb exist; load thumb and assume it's up to date.
		return drawing::Image::load(thumbFileName);
	}

	Ref< drawing::Image > image = drawing::Image::load(fileName);
	if (!image)
		return nullptr;

	drawing::ScaleFilter scale(
		width,
		height,
		drawing::ScaleFilter::MnAverage,
		drawing::ScaleFilter::MgNearest
	);
	image->apply(&scale);

	if (image->getPixelFormat().getAlphaBits() > 0 && alphaMode == AmWithAlpha)
	{
		Color4f pixel;
		for (int32_t y = 0; y < height; ++y)
		{
			for (int32_t x = 0; x < width; ++x)
			{
				Color4f alpha =
					((x >> 2) & 1) ^ ((y >> 2) & 1) ?
					Color4f(0.4f, 0.4f, 0.4f) :
					Color4f(0.6f, 0.6f, 0.6f);

				image->getPixelUnsafe(x, y, pixel);

				pixel = pixel * pixel.getAlpha() + alpha * (Scalar(1.0f) - pixel.getAlpha());
				pixel.setAlpha(Scalar(1.0f));

				image->setPixelUnsafe(x, y, pixel);
			}
		}
	}
	else if (alphaMode == AmAlphaOnly)
	{
		drawing::SwizzleFilter swizzleFilter(L"aaa1");
		image->apply(&swizzleFilter);
	}
	else	// Create solid alpha channel.
	{
		image->convert(drawing::PixelFormat::getR8G8B8A8());
		image->clearAlpha(1.0f);
	}

	// Ensure thumb path exist; then save thumb image.
	if (FileSystem::getInstance().makeAllDirectories(m_thumbsPath))
		image->save(thumbFileName);

	return image;
}

	}
}
