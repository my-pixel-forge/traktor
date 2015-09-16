#ifndef traktor_spark_FontResourceFactory_H
#define traktor_spark_FontResourceFactory_H

#include "Resource/IResourceFactory.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_SPARK_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace db
	{

class Database;

	}

	namespace render
	{

class IRenderSystem;

	}

	namespace spark
	{

/*! \brief Font resource factory.
 * \ingroup Spark
 */
class T_DLLCLASS FontResourceFactory : public resource::IResourceFactory
{
	T_RTTI_CLASS;

public:
	FontResourceFactory(db::Database* db, render::IRenderSystem* renderSystem);

	virtual const TypeInfoSet getResourceTypes() const T_FINAL;

	virtual const TypeInfoSet getProductTypes() const T_FINAL;

	virtual bool isCacheable() const T_FINAL;

	virtual Ref< Object > create(resource::IResourceManager* resourceManager, const TypeInfo& resourceType, const Guid& guid, const Object* current) const T_FINAL;

private:
	Ref< db::Database > m_db;
	Ref< render::IRenderSystem > m_renderSystem;
};

	}
}

#endif	// traktor_spark_FontResourceFactory_H
