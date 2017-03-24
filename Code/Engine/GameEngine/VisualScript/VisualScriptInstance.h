#pragma once

#include <GameEngine/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/HybridArray.h>

class ezVisualScriptNode;
class ezMessage;

typedef ezUInt32 ezVisualScriptNodeConnectionID;
typedef ezUInt32 ezVisualScriptPinConnectionID;

class EZ_GAMEENGINE_DLL ezVisualScriptInstance
{
public:
  ezVisualScriptInstance();
  ~ezVisualScriptInstance();

  void Clear();
  void Configure();
  void ExecuteScript();
  void SetupNodeIDs();

  void HandleMessage(ezMessage& msg);

  void ConnectNodes(ezUInt16 uiSourceNode, ezUInt8 uiOutputSlot, ezUInt16 uiTargetNode);
  void ConnectPins(ezUInt16 uiSourceNode, ezUInt8 uiOutputPin, ezUInt16 uiTargetNode, ezUInt16 uiTargetPin);

  ezDynamicArray<ezVisualScriptNode*> m_Nodes;

  ezHashTable<ezVisualScriptNodeConnectionID, ezUInt16> m_TargetNode;
  ezHashTable<ezVisualScriptPinConnectionID, ezHybridArray<ezUInt32, 2> > m_TargetNodeAndPin;
};
