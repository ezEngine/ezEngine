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
