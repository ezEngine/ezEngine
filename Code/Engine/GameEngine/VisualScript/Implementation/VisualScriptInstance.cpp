#include <PCH.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>
#include <Foundation/Strings/HashedString.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>

ezVisualScriptInstance::ezVisualScriptInstance()
{
}

ezVisualScriptInstance::~ezVisualScriptInstance()
{
  Clear();
}

void ezVisualScriptInstance::Clear()
{
  for (ezUInt32 i = 0; i < m_Nodes.GetCount(); ++i)
  {
    m_Nodes[i]->GetDynamicRTTI()->GetAllocator()->Deallocate(m_Nodes[i]);
  }

  m_Nodes.Clear();
  m_ExecutionConnections.Clear();
  m_DataConnections.Clear();
}

void ezVisualScriptInstance::Configure()
{
  Clear();

  ezVisualScriptNode_Counter* pCounterNode = (ezVisualScriptNode_Counter*)ezGetStaticRTTI<ezVisualScriptNode_Counter>()->GetAllocator()->Allocate();
  ezVisualScriptNode_If* pIfLessNode = (ezVisualScriptNode_If*)ezGetStaticRTTI<ezVisualScriptNode_If>()->GetAllocator()->Allocate();
  ezVisualScriptNode_Printer* pPrinterNode1 = (ezVisualScriptNode_Printer*)ezGetStaticRTTI<ezVisualScriptNode_Printer>()->GetAllocator()->Allocate();
  ezVisualScriptNode_Printer* pPrinterNode2 = (ezVisualScriptNode_Printer*)ezGetStaticRTTI<ezVisualScriptNode_Printer>()->GetAllocator()->Allocate();
  ezVisualScriptNode_Input* pInputNode = (ezVisualScriptNode_Input*)ezGetStaticRTTI<ezVisualScriptNode_Input>()->GetAllocator()->Allocate();

  m_Nodes.PushBack(pCounterNode);
  m_Nodes.PushBack(pIfLessNode);
  m_Nodes.PushBack(pPrinterNode1);
  m_Nodes.PushBack(pPrinterNode2);
  m_Nodes.PushBack(pInputNode);

  ConnectNodes(0, 0, 1);
  ConnectNodes(1, 0, 2);
  ConnectNodes(1, 1, 3);
  ConnectNodes(4, 0, 0);

  ConnectPins(0, 0, 1, 0);
  ConnectPins(0, 0, 2, 0);
  ConnectPins(0, 0, 3, 0);

  pIfLessNode->SetInputPinValue(1, 1000);
  pPrinterNode1->m_sPrint = "Value {0} is less than 1000";
  pPrinterNode2->m_sPrint = "Value {0} is more than 1000";
  pInputNode->m_UsageStringHash = ezTempHashedString("Jump").GetHash();

  SetupNodeIDs();
}

void ezVisualScriptInstance::Configure(const ezVisualScriptResourceDescriptor& resource)
{
  Clear();

  m_Nodes.Reserve(resource.m_Nodes.GetCount());

  ezUInt16 uiNodeId = 0;
  for (const auto& node : resource.m_Nodes)
  {
    ezVisualScriptNode* pNode = static_cast<ezVisualScriptNode*>(node.m_pType->GetAllocator()->Allocate());
    pNode->m_uiNodeID = uiNodeId++;

    m_Nodes.PushBack(pNode);

    /// \todo Properties
  }

  m_ExecutionConnections.Reserve(resource.m_ExecutionPaths.GetCount());

  for (const auto& con : resource.m_ExecutionPaths)
  {
    ConnectNodes(con.m_uiSourceNode, con.m_uiOutputPin, con.m_uiTargetNode);
  }

  for (const auto& con : resource.m_DataPaths)
  {
    ConnectPins(con.m_uiSourceNode, con.m_uiOutputPin, con.m_uiTargetNode, con.m_uiInputPin);
  }
}

void ezVisualScriptInstance::ExecuteScript()
{
  for (ezUInt32 i = 0; i < m_Nodes.GetCount(); ++i)
  {
    if (m_Nodes[i]->m_bStepNode)
    {
      m_Nodes[i]->m_bStepNode = false;
      m_Nodes[i]->Execute(this);
    }
  }
}

void ezVisualScriptInstance::SetupNodeIDs()
{
  for (ezUInt32 i = 0; i < m_Nodes.GetCount(); ++i)
  {
    m_Nodes[i]->m_uiNodeID = i;
  }
}


void ezVisualScriptInstance::HandleMessage(ezMessage& msg)
{
  for (ezUInt32 i = 0; i < m_Nodes.GetCount(); ++i)
  {
    m_Nodes[i]->GetDynamicRTTI()->DispatchMessage(m_Nodes[i], msg);
  }
}

void ezVisualScriptInstance::ConnectNodes(ezUInt16 uiSourceNode, ezUInt8 uiOutputSlot, ezUInt16 uiTargetNode)
{
  m_ExecutionConnections[((ezUInt32)uiSourceNode << 16) | (ezUInt32)uiOutputSlot] = uiTargetNode;
}

void ezVisualScriptInstance::ConnectPins(ezUInt16 uiSourceNode, ezUInt8 uiOutputPin, ezUInt16 uiTargetNode, ezUInt8 uiTargetPin)
{
  m_DataConnections[((ezUInt32)uiSourceNode << 16) | (ezUInt32)uiOutputPin].PushBack(((ezUInt32)uiTargetNode << 16) | (ezUInt32)uiTargetPin);
}
