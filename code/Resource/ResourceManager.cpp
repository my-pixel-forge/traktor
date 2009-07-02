#include <algorithm>
#include "Resource/ResourceManager.h"
#include "Resource/ResourceHandle.h"
#include "Resource/IResourceFactory.h"
#include "Core/Heap/New.h"
#include "Core/Thread/Acquire.h"
#include "Core/Log/Log.h"

namespace traktor
{
	namespace resource
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.resource.ResourceManager", ResourceManager, IResourceManager)

ResourceManager::ResourceManager()
{
}

void ResourceManager::addFactory(IResourceFactory* factory)
{
	m_factories.push_back(factory);
}

void ResourceManager::removeFactory(IResourceFactory* factory)
{
	m_factories.remove(factory);
}

void ResourceManager::removeAllFactories()
{
	m_factories.clear();
}

IResourceHandle* ResourceManager::bind(const Type& type, const Guid& guid)
{
	Acquire< Semaphore > scope(m_lock);
	Ref< ResourceHandle > handle;

	Ref< IResourceFactory > factory = findFactory(type);
	if (!factory)
		return 0;

	bool cacheable = factory->isCacheable();

	if (!cacheable)
		handle = gc_new< ResourceHandle >(cref(type));
	else
	{
		handle = m_cache[guid];
		if (!handle)
		{
			handle = gc_new< ResourceHandle >(cref(type));
			m_cache[guid] = handle;
		}
	}
	
	T_ASSERT (handle);
	T_ASSERT_M(&handle->getResourceType() == &type, L"Incorrect resource type; already bound as another type");

	if (!handle->get())
		load(guid, factory, handle);

	return handle;
}

void ResourceManager::update(const Guid& guid, bool force)
{
	Acquire< Semaphore > scope(m_lock);
	Ref< ResourceHandle > handle = m_cache[guid];
	if (!handle)
		return;

	if (!force && handle->get())
		return;

	Ref< IResourceFactory > factory = findFactory(handle->getResourceType());
	if (!factory)
		return;

	load(guid, factory, handle);
}

void ResourceManager::flush(const Guid& guid)
{
	Acquire< Semaphore > scope(m_lock);
	Ref< ResourceHandle > handle = m_cache[guid];
	if (handle)
		handle->flush();
}

void ResourceManager::flush()
{
	Acquire< Semaphore > scope(m_lock);
	for (std::map< Guid, Ref< ResourceHandle > >::iterator i = m_cache.begin(); i != m_cache.end(); ++i)
		i->second->flush();
}

IResourceFactory* ResourceManager::findFactory(const Type& type)
{
	for (RefList< IResourceFactory >::iterator i = m_factories.begin(); i != m_factories.end(); ++i)
	{
		TypeSet typeSet = (*i)->getResourceTypes();
		if (std::find(typeSet.begin(), typeSet.end(), &type) != typeSet.end())
			return *i;
	}
	return 0;
}

void ResourceManager::load(const Guid& guid, IResourceFactory* factory, ResourceHandle* handle)
{
	const Type& resourceType = handle->getResourceType();
	Ref< Object > object = factory->create(this, resourceType, guid);
	if (object)
	{
		T_ASSERT_M (is_type_of(resourceType, object->getType()), L"Incorrect type of created resource");
		handle->replace(object);
	}
	else
		log::error << L"Unable to create resource \"" << guid.format() << L"\" (" << resourceType.getName() << L")" << Endl;
}

	}
}
