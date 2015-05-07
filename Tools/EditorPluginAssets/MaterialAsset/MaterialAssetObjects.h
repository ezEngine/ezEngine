#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

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
  ezMaterialAssetObject()
  {
  }

};

class ezMaterialAssetObjectManager : public ezSimpleDocumentObjectManager<ezMaterialAssetProperties, ezMaterialAssetObject>
{
public:

};



