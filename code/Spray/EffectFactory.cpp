/*
 * TRAKTOR
 * Copyright (c) 2022 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Core/Io/Reader.h"
#include "Database/Instance.h"
#include "Spray/Effect.h"
#include "Spray/EffectData.h"
#include "Spray/EffectFactory.h"
#include "Spray/PointSet.h"
#include "Spray/PointSetResource.h"

namespace traktor::spray
{

T_IMPLEMENT_RTTI_CLASS(L"traktor.spray.EffectFactory", EffectFactory, resource::IResourceFactory)

EffectFactory::EffectFactory(const world::IEntityBuilder* entityBuilder)
:	m_entityBuilder(entityBuilder)
{
}

const TypeInfoSet EffectFactory::getResourceTypes() const
{
	return makeTypeInfoSet< EffectData, PointSetResource >();
}

const TypeInfoSet EffectFactory::getProductTypes(const TypeInfo& resourceType) const
{
	if (is_type_a< EffectData >(resourceType))
		return makeTypeInfoSet< Effect >();
	else if (is_type_a< PointSetResource >(resourceType))
		return makeTypeInfoSet< PointSet >();
	else
		return TypeInfoSet();
}

bool EffectFactory::isCacheable(const TypeInfo& productType) const
{
	return true;
}

Ref< Object > EffectFactory::create(resource::IResourceManager* resourceManager, const db::Database* database, const db::Instance* instance, const TypeInfo& productType, const Object* current) const
{
	if (is_type_a< Effect >(productType))
	{
		Ref< EffectData > effectData = instance->getObject< EffectData >();
		if (effectData)
			return effectData->createEffect(resourceManager, m_entityBuilder);
		else
			return nullptr;
	}
	else if (is_type_a< PointSet >(productType))
	{
		Ref< IStream > stream = instance->readData(L"Data");
		if (!stream)
			return nullptr;

		Ref< PointSet > pointSet = new PointSet();
		if (!pointSet->read(stream))
			return nullptr;

		return pointSet;
	}
	else
		return nullptr;
}

}
