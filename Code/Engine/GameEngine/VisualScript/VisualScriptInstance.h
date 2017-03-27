#pragma once

#include <GameEngine/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Containers/Map.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

class ezVisualScriptNode;
class ezMessage;
struct ezVisualScriptResourceDescriptor;
class ezGameObject;

typedef ezUInt32 ezVisualScriptNodeConnectionID;
typedef ezUInt32 ezVisualScriptPinConnectionID;

typedef void(*ezVisualScriptDataPinAssignFunc)(const void* src, void* dst);

class EZ_GAMEENGINE_DLL ezVisualScriptInstance
{
public:
  ezVisualScriptInstance();

  static void SetupPinDataTypeConversions();

  ~ezVisualScriptInstance();

  void Configure(const ezVisualScriptResourceDescriptor& resource, ezGameObject* pOwner);
  void ExecuteScript();
  void HandleMessage(ezMessage& msg);

  void SetOutputPinValue(const ezVisualScriptNode* pNode, ezUInt8 uiPin, const void* pValue);
  void ExecuteConnectedNodes(const ezVisualScriptNode* pNode, ezUInt16 uiNthTarget);

  static void RegisterDataPinAssignFunction(ezVisualScriptDataPinType sourceType, ezVisualScriptDataPinType dstType, ezVisualScriptDataPinAssignFunc func);
  static ezVisualScriptDataPinAssignFunc FindDataPinAssignFunction(ezVisualScriptDataPinType sourceType, ezVisualScriptDataPinType dstType);

  ezGameObject* GetOwner() const { return m_pOwner; }

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

  ezGameObject* m_pOwner = nullptr;
  ezDynamicArray<ezVisualScriptNode*> m_Nodes;
  ezDynamicArray<ezHybridArray<ezUInt16, 2>> m_NodeDependencies;
  ezHashTable<ezVisualScriptNodeConnectionID, ezUInt16> m_ExecutionConnections;
  ezHashTable<ezVisualScriptPinConnectionID, ezHybridArray<DataPinConnection, 2> > m_DataConnections;

  struct AssignFuncKey
  {
    EZ_DECLARE_POD_TYPE();

    ezVisualScriptDataPinType m_SourceType;
    ezVisualScriptDataPinType m_DstType;

    EZ_ALWAYS_INLINE bool operator==(const AssignFuncKey& rhs) const
    {
      return m_SourceType == rhs.m_SourceType && m_DstType == rhs.m_DstType;
    }

    EZ_ALWAYS_INLINE bool operator<(const AssignFuncKey& rhs) const
    {
      if (m_SourceType < rhs.m_SourceType)
        return true;
      if (m_SourceType > rhs.m_SourceType)
        return false;

      return m_DstType < rhs.m_DstType;
    }
  };

  static ezMap<AssignFuncKey, ezVisualScriptDataPinAssignFunc> s_DataPinAssignFunctions;
};
