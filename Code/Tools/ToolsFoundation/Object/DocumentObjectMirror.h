#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Containers/HybridArray.h>



struct EZ_TOOLSFOUNDATION_DLL ezObjectChangeStep
{
  ezObjectChangeStep() {}
  ezObjectChangeStep(const char* sProperty, const ezVariant& index)
    : m_sProperty(sProperty), m_Index(index)
  {
  }

  ezString m_sProperty;
  ezVariant m_Index;
  ezVariant m_Value;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_TOOLSFOUNDATION_DLL, ezObjectChangeStep);

/// \brief An object change starts at the heap object m_Root (because we can only safely store pointers to those).
///  From this object we follow m_Steps (member arrays, structs) to execute m_Change at the end target.
///
/// In case of an NodeAdded operation, m_GraphData contains the entire subgraph of this node.
class EZ_TOOLSFOUNDATION_DLL ezObjectChange
{
public:
  ezObjectChange(){}
  ezObjectChange(const ezObjectChange &);
  ezObjectChange(ezObjectChange && rhs );
  void operator=(ezObjectChange&& rhs);
  void operator=(ezObjectChange& rhs);
  void GetGraph(ezAbstractObjectGraph& graph) const;
  void SetGraph(ezAbstractObjectGraph& graph);

  ezDiffOperation m_Change; //< Change at the target.

  ezUuid m_Root; //< The object that is the parent of the op, namely the parent heap object we can store a pointer to.
  ezHybridArray<ezObjectChangeStep, 2> m_Steps; //< Path from root to target of change plus splitting of ezPropertyPath into separate steps.
  ezDataBuffer m_GraphData; //< In case of ObjectAdded, this holds the binary serialized object graph.
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_TOOLSFOUNDATION_DLL, ezObjectChange);


class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectMirror
{
public:
  ezDocumentObjectMirror();
  virtual ~ezDocumentObjectMirror();

  void InitSender(const ezDocumentObjectManager* pManager);
  void InitReceiver(ezRttiConverterContext* pContext);
  void DeInit();

  void SendDocument();
  void Clear();

  void TreeStructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void TreePropertyEventHandler(const ezDocumentObjectPropertyEvent& e);

  void* GetNativeObjectPointer(const ezDocumentObject* pObject);
  const void* GetNativeObjectPointer(const ezDocumentObject* pObject) const;

protected:
  bool IsRootObject(const ezDocumentObject* pParent);
  bool IsHeapAllocated(const ezDocumentObject* pParent, const char* szParentProperty);
  static void CreatePath(ezObjectChange& out_change, const ezDocumentObject* pRoot, const ezPropertyPath& propertyPath);
  static ezUuid FindRootOpObject(const ezDocumentObject* pObject, ezHybridArray<const ezDocumentObject*, 8>& path);
  static void FlattenSteps(const ezArrayPtr<const ezDocumentObject* const> path, ezHybridArray<ezObjectChangeStep, 2>& out_steps);
  static void AddPathToSteps(const char* szPropertyPath, const ezVariant& index, ezHybridArray<ezObjectChangeStep, 2>& out_steps, bool bAddLastProperty = true);

  virtual void ApplyOp(ezObjectChange& change);
  void ApplyOp(ezRttiConverterObject object, const ezObjectChange& change);
  void RetrieveObject(ezRttiConverterObject object, const ezObjectChange& change, const ezArrayPtr<const ezObjectChangeStep> path);

protected:
  ezRttiConverterContext* m_pContext;
  const ezDocumentObjectManager* m_pManager;
};