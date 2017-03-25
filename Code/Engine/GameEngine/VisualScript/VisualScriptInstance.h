#pragma once

#include <GameEngine/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Types/Variant.h>

class ezVisualScriptNode;
class ezMessage;
struct ezVisualScriptResourceDescriptor;

typedef ezUInt32 ezVisualScriptNodeConnectionID;
typedef ezUInt32 ezVisualScriptPinConnectionID;

class EZ_GAMEENGINE_DLL ezVisualScriptInstance
{
public:
  ezVisualScriptInstance();
  ~ezVisualScriptInstance();

  void Configure(const ezVisualScriptResourceDescriptor& resource);
  void ExecuteScript();
  void HandleMessage(ezMessage& msg);

  void SetOutputPinValue(const ezVisualScriptNode* pNode, ezUInt8 uiPin, const ezVariant& value);
  void ExecuteConnectedNodes(const ezVisualScriptNode* pNode, ezUInt16 uiNthTarget);

private:
  friend class ezVisualScriptNode;

  void Clear();
  void ComputeNodeDependencies();
  void ExecuteDependentNodes(ezUInt16 uiNode);

  void ConnectNodes(ezUInt16 uiSourceNode, ezUInt8 uiOutputSlot, ezUInt16 uiTargetNode);
  void ConnectPins(ezUInt16 uiSourceNode, ezUInt8 uiOutputPin, ezUInt16 uiTargetNode, ezUInt8 uiTargetPin);

  ezDynamicArray<ezVisualScriptNode*> m_Nodes;
  ezDynamicArray<ezHybridArray<ezUInt16, 2>> m_NodeDependencies;
  ezHashTable<ezVisualScriptNodeConnectionID, ezUInt16> m_ExecutionConnections;
  ezHashTable<ezVisualScriptPinConnectionID, ezHybridArray<ezUInt32, 2> > m_DataConnections;
};
