#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>

class ezTextureAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureAssetProperties);

public:
  ezTextureAssetProperties();

};

class ezTextureAssetObject : public ezDocumentObjectDirectMember<ezReflectedClass, ezTextureAssetProperties>
{
public:
  ezTextureAssetObject();
  ~ezTextureAssetObject();
};


