#include <PCH.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>
#include <Foundation/Strings/HashedString.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Core/World/Declarations.h>
#include <Core/World/GameObject.h>
#include <Foundation/Communication/Message.h>
#include <Core/Messages/EventMessage.h>

ezMap<ezVisualScriptInstance::AssignFuncKey, ezVisualScriptDataPinAssignFunc> ezVisualScriptInstance::s_DataPinAssignFunctions;

bool ezVisualScriptAssignNumberNumber(const void* src, void* dst)
{
  const bool res = *reinterpret_cast<double*>(dst) != *reinterpret_cast<const double*>(src);
  *reinterpret_cast<double*>(dst) = *reinterpret_cast<const double*>(src);
  return res;
}

bool ezVisualScriptAssignBoolBool(const void* src, void* dst)
{
  const bool res = *reinterpret_cast<bool*>(dst) != *reinterpret_cast<const bool*>(src);
  *reinterpret_cast<bool*>(dst) = *reinterpret_cast<const bool*>(src);
  return res;
}

bool ezVisualScriptAssignNumberBool(const void* src, void* dst)
{
  const bool res = *reinterpret_cast<bool*>(dst) != *reinterpret_cast<const double*>(src) > 0.0;
  *reinterpret_cast<bool*>(dst) = *reinterpret_cast<const double*>(src) > 0.0;
  return res;
}

bool ezVisualScriptAssignVec3Vec3(const void* src, void* dst)
{
  const bool res = *reinterpret_cast<ezVec3*>(dst) != *reinterpret_cast<const ezVec3*>(src);
  *reinterpret_cast<ezVec3*>(dst) = *reinterpret_cast<const ezVec3*>(src);
  return res;
}

bool ezVisualScriptAssignNumberVec3(const void* src, void* dst)
{
  const bool res = *reinterpret_cast<ezVec3*>(dst) != ezVec3(static_cast<float>(*reinterpret_cast<const double*>(src)));
  *reinterpret_cast<ezVec3*>(dst) = ezVec3(static_cast<float>(*reinterpret_cast<const double*>(src)));
  return res;
}

bool ezVisualScriptAssignGameObject(const void* src, void* dst)
{
  const bool res = *reinterpret_cast<ezGameObjectHandle*>(dst) != *reinterpret_cast<const ezGameObjectHandle*>(src);
  *reinterpret_cast<ezGameObjectHandle*>(dst) = *reinterpret_cast<const ezGameObjectHandle*>(src);
  return res;
}

bool ezVisualScriptAssignComponent(const void* src, void* dst)
{
  const bool res = *reinterpret_cast<ezComponentHandle*>(dst) != *reinterpret_cast<const ezComponentHandle*>(src);
  *reinterpret_cast<ezComponentHandle*>(dst) = *reinterpret_cast<const ezComponentHandle*>(src);
  return res;
}

ezVisualScriptInstance::ezVisualScriptInstance()
{
  SetupPinDataTypeConversions();
}

void ezVisualScriptInstance::SetupPinDataTypeConversions()
{
  static bool bDone = false;
  if (bDone)
    return;

  bDone = true;

  RegisterDataPinAssignFunction(ezVisualScriptDataPinType::Number, ezVisualScriptDataPinType::Number, ezVisualScriptAssignNumberNumber);
  RegisterDataPinAssignFunction(ezVisualScriptDataPinType::Boolean, ezVisualScriptDataPinType::Boolean, ezVisualScriptAssignBoolBool);
  RegisterDataPinAssignFunction(ezVisualScriptDataPinType::Vec3, ezVisualScriptDataPinType::Vec3, ezVisualScriptAssignVec3Vec3);
  RegisterDataPinAssignFunction(ezVisualScriptDataPinType::Number, ezVisualScriptDataPinType::Vec3, ezVisualScriptAssignNumberVec3);
  RegisterDataPinAssignFunction(ezVisualScriptDataPinType::GameObjectHandle, ezVisualScriptDataPinType::GameObjectHandle, ezVisualScriptAssignGameObject);
  RegisterDataPinAssignFunction(ezVisualScriptDataPinType::ComponentHandle, ezVisualScriptDataPinType::ComponentHandle, ezVisualScriptAssignComponent);
  RegisterDataPinAssignFunction(ezVisualScriptDataPinType::Number, ezVisualScriptDataPinType::Boolean, ezVisualScriptAssignNumberBool);
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

  m_pWorld = nullptr;
  m_Nodes.Clear();
  m_ExecutionConnections.Clear();
  m_DataConnections.Clear();
  m_LocalVariables.Clear();
  m_hScriptResource.Invalidate();
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
    auto* pNode = m_Nodes[uiDependency];

    // recurse to the most dependent nodes first
    ExecuteDependentNodes(uiDependency);

    pNode->Execute(this, 0);
    pNode->m_bInputValuesChanged = false;
  }
}

void ezVisualScriptInstance::Configure(const ezVisualScriptResourceHandle& hScript, ezGameObject* pOwner)
{
  Clear();

  ezResourceLock<ezVisualScriptResource> pScript(hScript, ezResourceAcquireMode::NoFallback);
  const auto& resource = pScript->GetDescriptor();
  m_pMessageHandlers = &resource.m_MessageHandlers;

  m_hScriptResource = hScript;

  if (pOwner)
  {
    m_hOwner = pOwner->GetHandle();
    m_pWorld = pOwner->GetWorld();
  }

  m_Nodes.Reserve(resource.m_Nodes.GetCount());

  for (ezUInt32 n = 0; n < resource.m_Nodes.GetCount(); ++n)
  {
    const auto& node = resource.m_Nodes[n];

    if (node.m_pType->IsDerivedFrom<ezMessage>())
    {
      CreateMessageNode(n, resource);
    }
    else if (node.m_pType->IsDerivedFrom<ezVisualScriptNode>())
    {
      CreateVisualScriptNode(n, resource);
    }
    else
    {
      ezLog::Error("Invalid node type '{0}' in visual script", node.m_sTypeName);
      Clear();
      return;
    }
  }

  m_ExecutionConnections.Reserve(resource.m_ExecutionPaths.GetCount());

  for (const auto& con : resource.m_ExecutionPaths)
  {
    ConnectExecutionPins(con.m_uiSourceNode, con.m_uiOutputPin, con.m_uiTargetNode, con.m_uiInputPin);
  }

  for (const auto& con : resource.m_DataPaths)
  {
    ConnectDataPins(con.m_uiSourceNode, con.m_uiOutputPin, (ezVisualScriptDataPinType::Enum) con.m_uiOutputPinType, con.m_uiTargetNode, con.m_uiInputPin, (ezVisualScriptDataPinType::Enum) con.m_uiInputPinType);
  }

  ComputeNodeDependencies();
}


void ezVisualScriptInstance::CreateVisualScriptNode(ezUInt32 uiNodeIdx, const ezVisualScriptResourceDescriptor& resource)
{
  const auto& node = resource.m_Nodes[uiNodeIdx];

  ezVisualScriptNode* pNode = static_cast<ezVisualScriptNode*>(node.m_pType->GetAllocator()->Allocate());
  pNode->m_uiNodeID = uiNodeIdx;

  // assign all property values
  for (ezUInt32 i = 0; i < node.m_uiNumProperties; ++i)
  {
    const ezUInt32 uiProp = node.m_uiFirstProperty + i;
    const auto& prop = resource.m_Properties[uiProp];

    ezAbstractProperty* pAbstract = pNode->GetDynamicRTTI()->FindPropertyByName(prop.m_sName);
    if (pAbstract->GetCategory() != ezPropertyCategory::Member)
      continue;

    ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pAbstract);
    ezReflectionUtils::SetMemberPropertyValue(pMember, pNode, prop.m_Value);
  }

  m_Nodes.PushBack(pNode);
}

void ezVisualScriptInstance::CreateMessageNode(ezUInt32 uiNodeIdx, const ezVisualScriptResourceDescriptor& resource)
{
  const auto& node = resource.m_Nodes[uiNodeIdx];

  ezVisualScriptNode_MessageSender* pNode = static_cast<ezVisualScriptNode_MessageSender*>(ezGetStaticRTTI<ezVisualScriptNode_MessageSender>()->GetAllocator()->Allocate());
  pNode->m_uiNodeID = uiNodeIdx;

  pNode->m_pMessageToSend = static_cast<ezMessage*>(node.m_pType->GetAllocator()->Allocate());

  // assign all property values
  for (ezUInt32 i = 0; i < node.m_uiNumProperties; ++i)
  {
    const ezUInt32 uiProp = node.m_uiFirstProperty + i;
    const auto& prop = resource.m_Properties[uiProp];

    ezAbstractProperty* pAbstract = pNode->m_pMessageToSend->GetDynamicRTTI()->FindPropertyByName(prop.m_sName);
    if (pAbstract == nullptr)
    {
      if (prop.m_sName == "Delay" && prop.m_Value.CanConvertTo<ezTime>())
      {
        pNode->m_Delay = prop.m_Value.ConvertTo<ezTime>();
      }
      if (prop.m_sName == "Recursive" && prop.m_Value.CanConvertTo<bool>())
      {
        pNode->m_bRecursive = prop.m_Value.ConvertTo<bool>();
      }

      continue;
    }

    if (pAbstract->GetCategory() != ezPropertyCategory::Member)
      continue;

    ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pAbstract);
    ezReflectionUtils::SetMemberPropertyValue(pMember, pNode->m_pMessageToSend, prop.m_Value);
  }

  m_Nodes.PushBack(pNode);
}

void ezVisualScriptInstance::ExecuteScript(ezVisualScriptInstanceActivity* pActivity /*= nullptr*/)
{
  m_pActivity = pActivity;

  if (m_pActivity != nullptr)
  {
    m_pActivity->Clear();
  }

  for (ezUInt32 i = 0; i < m_Nodes.GetCount(); ++i)
  {
    auto* pNode = m_Nodes[i];

    if (pNode->m_bStepNode)
    {
      ExecuteDependentNodes(i);

      // node stepping is always executed, even if the node only 'wants' to be executed on input change
      pNode->m_bStepNode = false;
      pNode->Execute(this, 0);
      pNode->m_bInputValuesChanged = false;
    }
  }
}

void ezVisualScriptInstance::HandleMessage(ezMessage& msg)
{
  ezUInt32 uiFirstHandler = m_pMessageHandlers->LowerBound(msg.GetId());

  while (uiFirstHandler < m_pMessageHandlers->GetCount())
  {
    const auto& data = (*m_pMessageHandlers)[uiFirstHandler];
    if (data.key != msg.GetId())
      return;

    const ezUInt32 uiNodeId = data.value;
    m_Nodes[uiNodeId]->GetDynamicRTTI()->DispatchMessage(m_Nodes[uiNodeId], msg);

    ++uiFirstHandler;
  }
}

void ezVisualScriptInstance::ConnectExecutionPins(ezUInt16 uiSourceNode, ezUInt8 uiOutputSlot, ezUInt16 uiTargetNode, ezUInt8 uiTargetPin)
{
  auto& con = m_ExecutionConnections[((ezUInt32)uiSourceNode << 16) | (ezUInt32)uiOutputSlot];
  con.m_uiTargetNode = uiTargetNode;
  con.m_uiTargetPin = uiTargetPin;
}

void ezVisualScriptInstance::ConnectDataPins(ezUInt16 uiSourceNode, ezUInt8 uiSourcePin, ezVisualScriptDataPinType::Enum sourcePinType, ezUInt16 uiTargetNode, ezUInt8 uiTargetPin, ezVisualScriptDataPinType::Enum targetPinType)
{
  DataPinConnection& con = m_DataConnections[((ezUInt32)uiSourceNode << 16) | (ezUInt32)uiSourcePin].ExpandAndGetRef();
  con.m_uiTargetNode = uiTargetNode;
  con.m_uiTargetPin = uiTargetPin;
  con.m_pTargetData = m_Nodes[uiTargetNode]->GetInputPinDataPointer(uiTargetPin);
  con.m_AssignFunc = FindDataPinAssignFunction(sourcePinType, targetPinType);
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
      if (TargetNodeAndPin.m_AssignFunc(pValue, TargetNodeAndPin.m_pTargetData))
      {
        m_Nodes[TargetNodeAndPin.m_uiTargetNode]->m_bInputValuesChanged = true;
      }
    }
  }

  if (m_pActivity != nullptr)
  {
    m_pActivity->m_ActiveDataConnections.PushBack(uiConnectionID);
  }
}

void ezVisualScriptInstance::ExecuteConnectedNodes(const ezVisualScriptNode* pNode, ezUInt16 uiNthTarget)
{
  const ezUInt32 uiConnectionID = ((ezUInt32)pNode->m_uiNodeID << 16) | (ezUInt32)uiNthTarget;

  ExecPinConnection TargetNode;
  if (!m_ExecutionConnections.TryGetValue(uiConnectionID, TargetNode))
    return;

  auto* pTargetNode = m_Nodes[TargetNode.m_uiTargetNode];

  ExecuteDependentNodes(TargetNode.m_uiTargetNode);

  pTargetNode->Execute(this, TargetNode.m_uiTargetPin);
  pTargetNode->m_bInputValuesChanged = false;

  if (m_pActivity != nullptr)
  {
    m_pActivity->m_ActiveExecutionConnections.PushBack(uiConnectionID);
  }
}

void ezVisualScriptInstance::RegisterDataPinAssignFunction(ezVisualScriptDataPinType::Enum sourceType, ezVisualScriptDataPinType::Enum dstType, ezVisualScriptDataPinAssignFunc func)
{
  AssignFuncKey key;
  key.m_SourceType = sourceType;
  key.m_DstType = dstType;

  s_DataPinAssignFunctions[key] = func;
}

ezVisualScriptDataPinAssignFunc ezVisualScriptInstance::FindDataPinAssignFunction(ezVisualScriptDataPinType::Enum sourceType, ezVisualScriptDataPinType::Enum dstType)
{
  AssignFuncKey key;
  key.m_SourceType = sourceType;
  key.m_DstType = dstType;

  return  s_DataPinAssignFunctions.GetValueOrDefault(key, nullptr);
}

bool ezVisualScriptInstance::HandlesEventMessage(const ezEventMessage& msg) const
{
  return m_pMessageHandlers->LowerBound(msg.GetId()) != ezInvalidIndex;
}
