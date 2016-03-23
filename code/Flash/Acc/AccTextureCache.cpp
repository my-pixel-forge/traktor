#include <cstring>
#include "Core/Log/Log.h"
#include "Flash/FlashBitmapImage.h"
#include "Flash/FlashBitmapResource.h"
#include "Flash/Acc/AccBitmapRect.h"
#include "Flash/Acc/AccTextureCache.h"
#include "Render/IRenderSystem.h"
#include "Render/ISimpleTexture.h"
#include "Resource/IResourceManager.h"
#include "Resource/Proxy.h"

namespace traktor
{
	namespace flash
	{
		namespace
		{

class AccCachedTexture : public render::ISimpleTexture
{
public:
	AccCachedTexture(AccTextureCache* cache, render::ISimpleTexture* texture)
	:	m_cache(cache)
	,	m_texture(texture)
	{
	}

	virtual ~AccCachedTexture()
	{
		destroy();
	}

	virtual void destroy()
	{
		if (m_texture)
		{
			m_cache->freeTexture(m_texture);
			m_texture = 0;
		}
	}

	virtual render::ITexture* resolve()
	{
		return m_texture;
	}

	virtual int getWidth() const
	{
		return m_texture->getWidth();
	}

	virtual int getHeight() const
	{
		return m_texture->getHeight();
	}

	virtual bool lock(int level, Lock& lock)
	{
		return m_texture->lock(level, lock);
	}

	virtual void unlock(int level)
	{
		m_texture->unlock(level);
	}

private:
	Ref< AccTextureCache > m_cache;
	Ref< render::ISimpleTexture > m_texture;
};

		}

AccTextureCache::AccTextureCache(
	resource::IResourceManager* resourceManager,
	render::IRenderSystem* renderSystem
)
:	m_resourceManager(resourceManager)
,	m_renderSystem(renderSystem)
{
}

AccTextureCache::~AccTextureCache()
{
	T_EXCEPTION_GUARD_BEGIN

	destroy();

	T_EXCEPTION_GUARD_END
}

void AccTextureCache::destroy()
{
	m_resourceManager = 0;
	m_renderSystem = 0;
}

void AccTextureCache::clear()
{
	m_freeTextures.clear();
}

Ref< AccBitmapRect > AccTextureCache::getBitmapTexture(const FlashBitmap& bitmap)
{
	AccBitmapRect* bmr = static_cast< AccBitmapRect* >(bitmap.getCacheObject());
	if (bmr)
		return bmr;

	if (const FlashBitmapResource* bitmapResource = dynamic_type_cast< const FlashBitmapResource* >(&bitmap))
	{
		resource::Proxy< render::ISimpleTexture > texture;

		m_resourceManager->bind(
			resource::Id< render::ISimpleTexture >(bitmapResource->getResourceId()),
			texture
		);

		float w = float(bitmapResource->getAtlasWidth());
		float h = float(bitmapResource->getAtlasHeight());

		Ref< AccBitmapRect > br = new AccBitmapRect(
			texture,
			false,
			bitmapResource->getX() / w,
			bitmapResource->getY() / h,
			bitmapResource->getWidth() / w,
			bitmapResource->getHeight() / h
		);

		bitmap.setCacheObject(br);
		return br;
	}
	else if (const FlashBitmapImage* bitmapData = dynamic_type_cast< const FlashBitmapImage* >(&bitmap))
	{
		Ref< render::ISimpleTexture > texture;

		// Check if any free texture matching requested size.
		for (RefArray< render::ISimpleTexture >::iterator i = m_freeTextures.begin(); i != m_freeTextures.end(); ++i)
		{
			if (i->getWidth() == bitmapData->getWidth() && i->getHeight() == bitmapData->getHeight())
			{
				render::ITexture::Lock tl;
				if (i->lock(0, tl))
				{
					std::memcpy(
						tl.bits,
						bitmapData->getBits(),
						bitmapData->getWidth() * bitmapData->getHeight() * 4
					);
					i->unlock(0);

					texture = *i;
					m_freeTextures.erase(i);
					break;
				}
			}
		}

		// No such texture found; create new texture.
		if (!texture)
		{
			render::SimpleTextureCreateDesc desc;

			desc.width = bitmapData->getWidth();
			desc.height = bitmapData->getHeight();
			desc.mipCount = 1;
			desc.format = render::TfR8G8B8A8;
			desc.immutable = false;
			desc.initialData[0].data = bitmapData->getBits();
			desc.initialData[0].pitch = desc.width * 4;
			desc.initialData[0].slicePitch = desc.width * desc.height * 4;

			texture = resource::Proxy< render::ISimpleTexture >(m_renderSystem->createSimpleTexture(desc));
		}

		if (!texture)
			return 0;

		Ref< AccBitmapRect > br = new AccBitmapRect(
			resource::Proxy< render::ISimpleTexture >(new AccCachedTexture(this, texture)),
			false,
			0.0f,
			0.0f,
			1.0f,
			1.0f
		);

		bitmap.setCacheObject(br);
		return br;
	}

	return 0;
}

void AccTextureCache::freeTexture(render::ISimpleTexture* texture)
{
	m_freeTextures.push_back(texture);
}

	}
}
