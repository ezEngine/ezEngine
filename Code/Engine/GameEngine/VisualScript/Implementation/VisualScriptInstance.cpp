#include <PCH.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>
#include <Foundation/Strings/HashedString.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>

ezMap<ezVisualScriptInstance::AssignFuncKey, ezVisualScriptDataPinAssignFunc> ezVisualScriptInstance::s_DataPinAssignFunctions;

void ezVisualScriptAssignDoubleDouble(const void* src, void* dst)
{
  *reinterpret_cast<double*>(dst) = *reinterpret_cast<const double*>(src);
}

void ezVisualScriptAssignDoubleFloat(const void* src, void* dst)
{
  *reinterpret_cast<float*>(dst) = static_cast<float>(*reinterpret_cast<const double*>(src));
}

void ezVisualScriptAssignFloatDouble(const void* src, void* dst)
{
  *reinterpret_cast<double*>(dst) = *reinterpret_cast<const float*>(src);
}

ezVisualScriptInstance::ezVisualScriptInstance()
{
  RegisterDataPinAssignFunction(ezGetStaticRTTI<double>(), ezGetStaticRTTI<double>(), ezVisualScriptAssignDoubleDouble);
  RegisterDataPinAssignFunction(ezGetStaticRTTI<double>(), ezGetStaticRTTI<float>(), ezVisualScriptAssignDoubleFloat);
  RegisterDataPinAssignFunction(ezGetStaticRTTI<float>(), ezGetStaticRTTI<double>(), ezVisualScriptAssignFloatDouble);
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

    const ezHybridArray<DataPinConnection, 2>& dst = it.Value();

    for (const DataPinConnection& target : dst)
    {
      m_NodeDependencies[target.m_uiTargetNode].PushBack(uiSourceNode);
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
    ConnectExecutionPins(con.m_uiSourceNode, con.m_uiOutputPin, con.m_uiTargetNode);
  }

  for (const auto& con : resource.m_DataPaths)
  {
    ConnectDataPins(con.m_uiSourceNode, con.m_uiOutputPin, con.m_uiTargetNode, con.m_uiInputPin);
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

void ezVisualScriptInstance::ConnectExecutionPins(ezUInt16 uiSourceNode, ezUInt8 uiOutputSlot, ezUInt16 uiTargetNode)
{
  m_ExecutionConnections[((ezUInt32)uiSourceNode << 16) | (ezUInt32)uiOutputSlot] = uiTargetNode;
}

void ezVisualScriptInstance::ConnectDataPins(ezUInt16 uiSourceNode, ezUInt8 uiSourcePin, ezUInt16 uiTargetNode, ezUInt8 uiTargetPin)
{
  DataPinConnection& con = m_DataConnections[((ezUInt32)uiSourceNode << 16) | (ezUInt32)uiSourcePin].ExpandAndGetRef();
  con.m_uiTargetNode = uiTargetNode;
  con.m_uiTargetPin = uiTargetPin;
  con.m_AssignFunc = nullptr;
  con.m_pTargetData = nullptr;

  const ezRTTI* pSourceType = nullptr;
  const ezRTTI* pTargetType = nullptr;

  {
    con.m_pTargetData = m_Nodes[uiTargetNode]->GetInputPinDataPointer(uiTargetPin);

    const ezRTTI* pRtti = m_Nodes[uiTargetNode]->GetDynamicRTTI();
    ezHybridArray<ezAbstractProperty*, 32> properties;
    pRtti->GetAllProperties(properties);

    for (const ezAbstractProperty* pProp : properties)
    {
      if (const ezVisScriptDataPinInAttribute* pAttr = pProp->GetAttributeByType<ezVisScriptDataPinInAttribute>())
      {
        if (pAttr->m_uiPinSlot == uiTargetPin)
        {
          pTargetType = pAttr->m_pDataType;
          break;
        }
      }
    }
  }

  {
    const ezRTTI* pRtti = m_Nodes[uiSourceNode]->GetDynamicRTTI();
    ezHybridArray<ezAbstractProperty*, 32> properties;
    pRtti->GetAllProperties(properties);

    for (const ezAbstractProperty* pProp : properties)
    {
      if (const ezVisScriptDataPinOutAttribute* pAttr = pProp->GetAttributeByType<ezVisScriptDataPinOutAttribute>())
      {
        if (pAttr->m_uiPinSlot == uiSourcePin)
        {
          pSourceType = pAttr->m_pDataType;
          break;
        }
      }
    }
  }

  con.m_AssignFunc = FindDataPinAssignFunction(pSourceType, pTargetType);
}

void ezVisualScriptInstance::SetOutputPinValue(const ezVisualScriptNode* pNode, ezUInt8 uiPin, const void* pValue)
{
  const ezUInt32 uiConnectionID = ((ezUInt32)pNode->m_uiNodeID << 16) | (ezUInt32)uiPin;

  ezHybridArray<DataPinConnection, 2>* TargetNodeAndPins;
  if (!m_DataConnections.TryGetValue(uiConnectionID, TargetNodeAndPins))
    return;

  for (const DataPinConnection& TargetNodeAndPin : *TargetNodeAndPins)
  {
    if (TargetNodeAndPin.m_AssignFunc)
    {
      TargetNodeAndPin.m_AssignFunc(pValue, TargetNodeAndPin.m_pTargetData);
    }
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

void ezVisualScriptInstance::RegisterDataPinAssignFunction(const ezRTTI* pSourceType, const ezRTTI* pDstType, ezVisualScriptDataPinAssignFunc func)
{
  AssignFuncKey key;
  key.m_pSourceType = pSourceType;
  key.m_pDstType = pDstType;

  s_DataPinAssignFunctions[key] = func;
}

ezVisualScriptDataPinAssignFunc ezVisualScriptInstance::FindDataPinAssignFunction(const ezRTTI* pSourceType, const ezRTTI* pDstType)
{
  AssignFuncKey key;
  key.m_pSourceType = pSourceType;
  key.m_pDstType = pDstType;

  auto it = s_DataPinAssignFunctions.Find(key);

  if (it.IsValid())
    return it.Value();

  return nullptr;
}

