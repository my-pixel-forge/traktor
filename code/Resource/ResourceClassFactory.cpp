#include "Core/Class/AutoRuntimeClass.h"
#include "Core/Class/Boxes.h"
#include "Core/Class/IRuntimeClassRegistrar.h"
#include "Resource/IResourceFactory.h"
#include "Resource/IResourceManager.h"
#include "Resource/ResourceBundle.h"
#include "Resource/ResourceClassFactory.h"

namespace traktor
{
	namespace resource
	{
		namespace
		{

Ref< ResourceHandle > IResourceManager_bind(IResourceManager* self, const TypeInfo& type, const Guid& guid)
{
	return self->bind(type, guid);
}

void IResourceManager_reload(IResourceManager* self, const Any& guidOrType, bool flushedOnly)
{
	if (CastAny< Guid >::accept(guidOrType))
		self->reload(CastAny< Guid >::get(guidOrType), flushedOnly);
	else if (CastAny< TypeInfo >::accept(guidOrType))
		self->reload(CastAny< TypeInfo >::get(guidOrType), flushedOnly);
}

		}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.resource.ResourceClassFactory", 0, ResourceClassFactory, IRuntimeClassFactory)

void ResourceClassFactory::createClasses(IRuntimeClassRegistrar* registrar) const
{
	Ref< AutoRuntimeClass< ResourceBundle > > classResourceBundle = new AutoRuntimeClass< ResourceBundle >();
	registrar->registerClass(classResourceBundle);

	Ref< AutoRuntimeClass< IResourceFactory > > classIResourceFactory = new AutoRuntimeClass< IResourceFactory >();
	classIResourceFactory->addMethod("isCacheable", &IResourceFactory::isCacheable);
	registrar->registerClass(classIResourceFactory);

	Ref< AutoRuntimeClass< ResourceHandle > > classIResourceHandle = new AutoRuntimeClass< ResourceHandle >();
	classIResourceHandle->addMethod("replace", &ResourceHandle::replace);
	classIResourceHandle->addMethod("get", &ResourceHandle::get);
	classIResourceHandle->addMethod("flush", &ResourceHandle::flush);
	registrar->registerClass(classIResourceHandle);

	Ref< AutoRuntimeClass< IResourceManager > > classIResourceManager = new AutoRuntimeClass< IResourceManager >();
	classIResourceManager->addMethod("addFactory", &IResourceManager::addFactory);
	classIResourceManager->addMethod("removeFactory", &IResourceManager::removeFactory);
	classIResourceManager->addMethod("removeAllFactories", &IResourceManager::removeAllFactories);
	classIResourceManager->addMethod("load", &IResourceManager::load);
	classIResourceManager->addMethod("bind", &IResourceManager_bind);
	classIResourceManager->addMethod("reload", &IResourceManager_reload);
	classIResourceManager->addMethod("unload", &IResourceManager::unload);
	classIResourceManager->addMethod("unloadUnusedResident", &IResourceManager::unloadUnusedResident);
	registrar->registerClass(classIResourceManager);
}

	}
}
