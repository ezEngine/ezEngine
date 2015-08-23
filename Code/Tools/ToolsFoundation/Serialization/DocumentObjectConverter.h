#pragma once

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectConverterWriter
{
public:

  ezDocumentObjectConverterWriter(ezAbstractObjectGraph* pGraph, const ezDocumentObjectManager* pManager, bool bSerializeReadOnly, bool bSerializeOwnerPtrs)
  {
    m_pGraph = pGraph;
    m_pManager = pManager;
    m_bSerializeReadOnly = bSerializeReadOnly;
    m_bSerializeOwnerPtrs = bSerializeOwnerPtrs;
  }

  ezAbstractObjectNode* AddObjectToGraph(const ezDocumentObjectBase* pObject, const char* szNodeName = nullptr);

private:
  void AddProperty(ezAbstractObjectNode* pNode, const ezAbstractProperty* pProp, const ezDocumentObjectBase* pObject);
  void AddProperties(ezAbstractObjectNode* pNode, const ezDocumentObjectBase* pObject);

  ezAbstractObjectNode* AddSubObjectToGraph(const ezDocumentObjectBase* pObject, const char* szNodeName);

  const ezDocumentObjectManager* m_pManager;
  ezAbstractObjectGraph* m_pGraph;
  bool m_bSerializeReadOnly;
  bool m_bSerializeOwnerPtrs;
  ezSet<const ezDocumentObjectBase*> m_QueuedObjects;
};


class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectConverterReader
{
public:
  enum class Mode
  {
    CreateOnly,
    CreateAndAddToDocument,
    CreateAndAddToDocumentUndoable
  };
  ezDocumentObjectConverterReader(const ezAbstractObjectGraph* pGraph, ezDocumentObjectManager* pManager, Mode mode);

  ezDocumentObjectBase* CreateObjectFromNode(const ezAbstractObjectNode* pNode, ezDocumentObjectBase* pParent, const char* szParentProperty, ezVariant index);
  void ApplyPropertiesToObject(const ezAbstractObjectNode* pNode, ezDocumentObjectBase* pObject);

private:
  void ApplyProperty(ezDocumentObjectBase* pObject, ezAbstractProperty* pProperty, const ezAbstractObjectNode::Property* pSource);

  Mode m_Mode;
  ezDocumentObjectManager* m_pManager;
  const ezAbstractObjectGraph* m_pGraph;
};

