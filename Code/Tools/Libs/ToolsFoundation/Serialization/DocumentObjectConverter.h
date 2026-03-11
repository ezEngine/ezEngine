#pragma once

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezObjectAccessorBase;

/// \brief Writes the state of an ezDocumentObject to an abstract graph.
///
/// This information can then be applied to another ezDocument object through ezDocumentObjectConverterReader,
/// or to entirely different class using ezRttiConverterReader.
class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectConverterWriter
{
public:
  using FilterFunction = ezDelegate<bool(const ezDocumentObject*, const ezAbstractProperty*)>;
  ezDocumentObjectConverterWriter(ezAbstractObjectGraph* pGraph, const ezDocumentObjectManager* pManager, FilterFunction filter = FilterFunction())
  {
    m_pGraph = pGraph;
    m_pManager = pManager;
    m_Filter = filter;
  }

  ezAbstractObjectNode* AddObjectToGraph(const ezDocumentObject* pObject, ezStringView sNodeName = nullptr);

private:
  void AddProperty(ezAbstractObjectNode* pNode, const ezAbstractProperty* pProp, const ezDocumentObject* pObject);
  void AddProperties(ezAbstractObjectNode* pNode, const ezDocumentObject* pObject);

  ezAbstractObjectNode* AddSubObjectToGraph(const ezDocumentObject* pObject, ezStringView sNodeName);

  const ezDocumentObjectManager* m_pManager;
  ezAbstractObjectGraph* m_pGraph;
  FilterFunction m_Filter;
  ezSet<const ezDocumentObject*> m_QueuedObjects;
};


/// \brief Reads document objects from an abstract graph and reconstructs them in a document.
class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectConverterReader
{
public:
  enum class Mode
  {
    CreateOnly,
    CreateAndAddToDocument,
  };
  ezDocumentObjectConverterReader(const ezAbstractObjectGraph* pGraph, ezDocumentObjectManager* pManager, Mode mode);

  ezDocumentObject* CreateObjectFromNode(const ezAbstractObjectNode* pNode);
  void ApplyPropertiesToObject(const ezAbstractObjectNode* pNode, ezDocumentObject* pObject);

  ezUInt32 GetNumUnknownObjectCreations() const { return m_uiUnknownTypeInstances; }
  const ezSet<ezString>& GetUnknownObjectTypes() const { return m_UnknownTypes; }

  static void ApplyDiffToObject(ezObjectAccessorBase* pObjectAccessor, const ezDocumentObject* pObject, ezDeque<ezAbstractGraphDiffOperation>& ref_diff);

private:
  void AddObject(ezDocumentObject* pObject, ezDocumentObject* pParent, ezStringView sParentProperty, ezVariant index);
  void ApplyProperty(ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezAbstractObjectNode::Property* pSource);
  static void ApplyDiff(ezObjectAccessorBase* pObjectAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp,
    ezAbstractGraphDiffOperation& op, ezDeque<ezAbstractGraphDiffOperation>& diff);

  Mode m_Mode;
  ezDocumentObjectManager* m_pManager;
  const ezAbstractObjectGraph* m_pGraph;
  ezSet<ezString> m_UnknownTypes;
  ezUInt32 m_uiUnknownTypeInstances;
};
