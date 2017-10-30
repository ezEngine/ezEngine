#pragma once

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezObjectAccessorBase;

class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectConverterWriter
{
public:
  typedef ezDelegate<bool(const ezAbstractProperty*)> FilterFunction;
  ezDocumentObjectConverterWriter(ezAbstractObjectGraph* pGraph, const ezDocumentObjectManager* pManager,
    FilterFunction filter = FilterFunction())
  {
    m_pGraph = pGraph;
    m_pManager = pManager;
    m_Filter = filter;
  }

  ezAbstractObjectNode* AddObjectToGraph(const ezDocumentObject* pObject, const char* szNodeName = nullptr);

private:
  void AddProperty(ezAbstractObjectNode* pNode, const ezAbstractProperty* pProp, const ezDocumentObject* pObject);
  void AddProperties(ezAbstractObjectNode* pNode, const ezDocumentObject* pObject);

  ezAbstractObjectNode* AddSubObjectToGraph(const ezDocumentObject* pObject, const char* szNodeName);

  const ezDocumentObjectManager* m_pManager;
  ezAbstractObjectGraph* m_pGraph;
  FilterFunction m_Filter;
  ezSet<const ezDocumentObject*> m_QueuedObjects;
};


class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectConverterReader
{
public:
  enum class Mode
  {
    CreateOnly,
    CreateAndAddToDocument,
  };
  ezDocumentObjectConverterReader(const ezAbstractObjectGraph* pGraph, ezDocumentObjectManager* pManager, Mode mode);

  ezDocumentObject* CreateObjectFromNode(const ezAbstractObjectNode* pNode, ezDocumentObject* pParent, const char* szParentProperty, ezVariant index);
  void ApplyPropertiesToObject(const ezAbstractObjectNode* pNode, ezDocumentObject* pObject);

  ezUInt32 GetNumUnknownObjectCreations() const { return m_uiUnknownTypeInstances; }
  const ezSet<ezString>& GetUnknownObjectTypes() const { return m_UnknownTypes; }

  static void ApplyDiffToObject(ezObjectAccessorBase* pObjectAccessor, const ezDocumentObject* pObject, ezDeque<ezAbstractGraphDiffOperation>& diff);

private:
  void ApplyProperty(ezDocumentObject* pObject, ezAbstractProperty* pProp, const ezAbstractObjectNode::Property* pSource);
  static void ApplyDiff(ezObjectAccessorBase* pObjectAccessor, const ezDocumentObject* pObject, ezAbstractProperty* pProp, ezAbstractGraphDiffOperation& op, ezDeque<ezAbstractGraphDiffOperation>& diff);

  Mode m_Mode;
  ezDocumentObjectManager* m_pManager;
  const ezAbstractObjectGraph* m_pGraph;
  ezSet<ezString> m_UnknownTypes;
  ezUInt32 m_uiUnknownTypeInstances;
};

