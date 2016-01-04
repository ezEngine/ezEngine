#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Containers/HybridArray.h>

struct EZ_TOOLSFOUNDATION_DLL ezObjectChangeType
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    ObjectAdded,
    ObjectRemoved,
    PropertySet,
    PropertyInserted,
    PropertyRemoved,

    Default = ObjectAdded
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_TOOLSFOUNDATION_DLL, ezObjectChangeType);


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


class EZ_TOOLSFOUNDATION_DLL ezObjectChange
{
public:
  ezObjectChange(){}
  ezObjectChange(const ezObjectChange &);
  void operator=(ezObjectChange&& rhs);
  void operator=(ezObjectChange& rhs);
  void GetGraph(ezAbstractObjectGraph& graph) const;
  void SetGraph(ezAbstractObjectGraph& graph);

  ezEnum<ezObjectChangeType> m_Type;
  ezUuid m_Root; //< The object that is the parent of the op.
  ezHybridArray<ezObjectChangeStep, 4> m_Steps;
  ezObjectChangeStep m_Change;
  ezString m_sGraph;
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
  static void FlattenSteps(const ezArrayPtr<const ezDocumentObject* const> path, ezHybridArray<ezObjectChangeStep, 4>& out_steps);
  static void AddPathToSteps(const char* szPropertyPath, const ezVariant& index, ezHybridArray<ezObjectChangeStep, 4>& out_steps, bool bAddLastProperty = true);

  virtual void ApplyOp(ezObjectChange& change);
  void ApplyOp(ezRttiConverterObject object, const ezObjectChange& change);
  void RetrieveObject(ezRttiConverterObject object, const ezObjectChange& change, const ezArrayPtr<const ezObjectChangeStep> path);

protected:
  ezRttiConverterContext* m_pContext;
  const ezDocumentObjectManager* m_pManager;
};