#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorFramework/GUI/PropertyEditorBaseWidget.moc.h>

class ezMeshAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshAssetProperties);

public:
  ezMeshAssetProperties();

  ezString m_sMeshFile;
  ezString m_sSlot0;
  ezString m_sSlot1;
  ezString m_sSlot2;
  ezString m_sSlot3;
  ezString m_sSlot4;
  ezString m_sSlot5;
  ezString m_sSlot6;
  ezString m_sSlot7;
  ezString m_sSlot8;
  ezString m_sSlot9;
  ezString m_sSlot10;
  ezString m_sSlot11;
  ezString m_sSlot12;
  ezString m_sSlot13;
  ezString m_sSlot14;
  ezString m_sSlot15;
  ezString m_sSlot16;
  ezString m_sSlot17;
  ezString m_sSlot18;
  ezString m_sSlot19;
  ezString m_sSlot20;
  ezString m_sSlot21;
  ezString m_sSlot22;
  ezString m_sSlot23;
  ezString m_sSlot24;
  ezString m_sSlot25;
  ezString m_sSlot26;
  ezString m_sSlot27;
  ezString m_sSlot28;
  ezString m_sSlot29;
  ezString m_sSlot30;
  ezString m_sSlot31;

  const ezString& GetResourceSlotProperty(ezUInt32 uiSlot) const;

  ezUInt32 m_uiVertices;
  ezUInt32 m_uiTriangles;
  ezDynamicArray<ezString> m_SlotNames;

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



