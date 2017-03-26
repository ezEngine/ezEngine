#pragma once

#include <GameEngine/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Containers/Map.h>

class ezVisualScriptNode;
class ezMessage;
struct ezVisualScriptResourceDescriptor;

typedef ezUInt32 ezVisualScriptNodeConnectionID;
typedef ezUInt32 ezVisualScriptPinConnectionID;

typedef void(*ezVisualScriptDataPinAssignFunc)(const void* src, void* dst);

class EZ_GAMEENGINE_DLL ezVisualScriptInstance
{
public:
  ezVisualScriptInstance();
  ~ezVisualScriptInstance();

  void Configure(const ezVisualScriptResourceDescriptor& resource);
  void ExecuteScript();
  void HandleMessage(ezMessage& msg);

  void SetOutputPinValue(const ezVisualScriptNode* pNode, ezUInt8 uiPin, const void* pValue);
  void ExecuteConnectedNodes(const ezVisualScriptNode* pNode, ezUInt16 uiNthTarget);

  static void RegisterDataPinAssignFunction(const ezRTTI* pSourceType, const ezRTTI* pDstType, ezVisualScriptDataPinAssignFunc func);
  static ezVisualScriptDataPinAssignFunc FindDataPinAssignFunction(const ezRTTI* pSourceType, const ezRTTI* pDstType);

private:
  friend class ezVisualScriptNode;

  void Clear();
  void ComputeNodeDependencies();
  void ExecuteDependentNodes(ezUInt16 uiNode);

  void ConnectExecutionPins(ezUInt16 uiSourceNode, ezUInt8 uiOutputSlot, ezUInt16 uiTargetNode);
  void ConnectDataPins(ezUInt16 uiSourceNode, ezUInt8 uiSourcePin, ezUInt16 uiTargetNode, ezUInt8 uiTargetPin);

  struct DataPinConnection
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt16 m_uiTargetNode;
    ezUInt8 m_uiTargetPin;
    ezVisualScriptDataPinAssignFunc m_AssignFunc = nullptr;
    void* m_pTargetData = nullptr;
  };

  ezDynamicArray<ezVisualScriptNode*> m_Nodes;
  ezDynamicArray<ezHybridArray<ezUInt16, 2>> m_NodeDependencies;
  ezHashTable<ezVisualScriptNodeConnectionID, ezUInt16> m_ExecutionConnections;
  ezHashTable<ezVisualScriptPinConnectionID, ezHybridArray<DataPinConnection, 2> > m_DataConnections;

  struct AssignFuncKey
  {
    EZ_DECLARE_POD_TYPE();

    const ezRTTI* m_pSourceType;
    const ezRTTI* m_pDstType;

    EZ_ALWAYS_INLINE bool operator==(const AssignFuncKey& rhs) const
    {
      return m_pSourceType == rhs.m_pSourceType && m_pDstType == rhs.m_pDstType;
    }

    EZ_ALWAYS_INLINE bool operator<(const AssignFuncKey& rhs) const
    {
      if (m_pSourceType < rhs.m_pSourceType)
        return true;
      if (m_pSourceType > rhs.m_pSourceType)
        return false;

      return m_pDstType < rhs.m_pDstType;
    }
  };

  static ezMap<AssignFuncKey, ezVisualScriptDataPinAssignFunc> s_DataPinAssignFunctions;
};
