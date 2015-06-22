#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class ezMeshAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshAssetProperties);

public:
  ezMeshAssetProperties();

  ezString m_sMeshFile;

private:

};

class ezMeshAssetObject : public ezDocumentObjectDirectMember<ezReflectedClass, ezMeshAssetProperties>
{
public:
  ezMeshAssetObject()
  {
  }

};

class ezMeshAssetObjectManager : public ezSimpleDocumentObjectManager<ezMeshAssetProperties, ezMeshAssetObject>
{
public:

};



