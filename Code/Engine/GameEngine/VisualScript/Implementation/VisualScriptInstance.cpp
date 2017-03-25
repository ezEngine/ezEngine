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


void ezVisualScriptInstance::ComputeNodeDependencies()
{
  m_NodeDependencies.SetCount(m_Nodes.GetCount());

  for (auto it = m_DataConnections.GetIterator(); it.IsValid(); ++it)
  {
    const ezVisualScriptPinConnectionID src = it.Key();

    const ezUInt16 uiSourceNode = (src >> 16) & 0xFFFF;

    if (m_Nodes[uiSourceNode]->IsManuallyStepped())
      continue;

    const ezHybridArray<ezUInt32, 2>& dst = it.Value();

    for (ezUInt32 target : dst)
    {
      const ezUInt16 uiTargetNode = (target >> 16) & 0xFFFF;

      m_NodeDependencies[uiTargetNode].PushBack(uiSourceNode);
    }
  }
}


void ezVisualScriptInstance::ExecuteDependentNodes(ezUInt16 uiNode)
{
  const auto& dep = m_NodeDependencies[uiNode];
  for (ezUInt32 i = 0; i < dep.GetCount(); ++i)
  {
    const ezUInt32 uiDependency = dep[i];

    // recurse to the most dependent nodes first
    ExecuteDependentNodes(uiDependency);

    m_Nodes[uiDependency]->Execute(this);
  }
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

  ComputeNodeDependencies();
}

void ezVisualScriptInstance::ExecuteScript()
{
  for (ezUInt32 i = 0; i < m_Nodes.GetCount(); ++i)
  {
    if (m_Nodes[i]->m_bStepNode)
    {
      ExecuteDependentNodes(i);

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

void ezVisualScriptInstance::SetOutputPinValue(const ezVisualScriptNode* pNode, ezUInt8 uiPin, const ezVariant& value)
{
  const ezUInt32 uiConnectionID = ((ezUInt32)pNode->m_uiNodeID << 16) | (ezUInt32)uiPin;

  ezHybridArray<ezUInt32, 2>* TargetNodeAndPins;
  if (!m_DataConnections.TryGetValue(uiConnectionID, TargetNodeAndPins))
    return;

  for (ezUInt32 uiTargetNodeAndPin : *TargetNodeAndPins)
  {
    const ezUInt32 uiTargetNode = uiTargetNodeAndPin >> 16;
    const ezUInt8 uiTargetPin = uiTargetNodeAndPin & 0xFF;

    m_Nodes[uiTargetNode]->SetInputPinValue(uiTargetPin, value);
  }
}

void ezVisualScriptInstance::ExecuteConnectedNodes(const ezVisualScriptNode* pNode, ezUInt16 uiNthTarget)
{
  const ezUInt32 uiConnectionID = ((ezUInt32)pNode->m_uiNodeID << 16) | (ezUInt32)uiNthTarget;

  ezUInt16 uiTargetNode = 0;
  if (!m_ExecutionConnections.TryGetValue(uiConnectionID, uiTargetNode))
    return;

  ExecuteDependentNodes(uiTargetNode);
  m_Nodes[uiTargetNode]->Execute(this);
}

