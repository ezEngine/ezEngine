#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
//#include <EditorFramework/GUI/PropertyEditorBaseWidget.moc.h>

struct ezResourceSlot
{
  ezString m_sLabel;
  ezString m_sResource;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezResourceSlot);

class ezMeshAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshAssetProperties);

public:
  ezMeshAssetProperties();

  ezString m_sMeshFile;
  float m_fMeshScaling;

  ezEnum<ezBasisAxis> m_ForwardDir;
  ezEnum<ezBasisAxis> m_RightDir;
  ezEnum<ezBasisAxis> m_UpDir;

  ezHybridArray<ezResourceSlot, 8> m_Slots;

  const ezString& GetResourceSlotProperty(ezUInt32 uiSlot) const;

  ezUInt32 m_uiVertices;
  ezUInt32 m_uiTriangles;

private:

};

class ezMeshAssetObjectManager : public ezSimpleDocumentObjectManager<ezMeshAssetProperties>
{
public:

};



