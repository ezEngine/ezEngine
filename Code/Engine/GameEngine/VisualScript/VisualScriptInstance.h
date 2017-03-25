#pragma once

#include <GameEngine/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/HybridArray.h>

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

private:
  friend class ezVisualScriptNode;

  void Clear();

  void ConnectNodes(ezUInt16 uiSourceNode, ezUInt8 uiOutputSlot, ezUInt16 uiTargetNode);
  void ConnectPins(ezUInt16 uiSourceNode, ezUInt8 uiOutputPin, ezUInt16 uiTargetNode, ezUInt8 uiTargetPin);

  ezDynamicArray<ezVisualScriptNode*> m_Nodes;
  ezHashTable<ezVisualScriptNodeConnectionID, ezUInt16> m_ExecutionConnections;
  ezHashTable<ezVisualScriptPinConnectionID, ezHybridArray<ezUInt32, 2> > m_DataConnections;
};
