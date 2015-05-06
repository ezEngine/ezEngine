#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>

class ezMaterialAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMaterialAssetProperties);

public:
  ezMaterialAssetProperties();


private:

};

class ezMaterialAssetObject : public ezDocumentObjectDirectMember<ezReflectedClass, ezMaterialAssetProperties>
{
public:
  ezMaterialAssetObject();
  ~ezMaterialAssetObject();


};


