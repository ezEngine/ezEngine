#include <PCH.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>
#include <Foundation/Strings/HashedString.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Core/World/Declarations.h>
#include <Core/World/GameObject.h>
#include <Foundation/Communication/Message.h>

ezMap<ezVisualScriptInstance::AssignFuncKey, ezVisualScriptDataPinAssignFunc> ezVisualScriptInstance::s_DataPinAssignFunctions;

void ezVisualScriptAssignNumberNumber(const void* src, void* dst)
{
  *reinterpret_cast<double*>(dst) = *reinterpret_cast<const double*>(src);
}

void ezVisualScriptAssignBoolBool(const void* src, void* dst)
{
  *reinterpret_cast<bool*>(dst) = *reinterpret_cast<const bool*>(src);
}

void ezVisualScriptAssignNumberBool(const void* src, void* dst)
{
  *reinterpret_cast<bool*>(dst) = *reinterpret_cast<const double*>(src) > 0.0;
}

void ezVisualScriptAssignVec3Vec3(const void* src, void* dst)
{
  *reinterpret_cast<ezVec3*>(dst) = *reinterpret_cast<const ezVec3*>(src);
}

void ezVisualScriptAssignNumberVec3(const void* src, void* dst)
{
  *reinterpret_cast<ezVec3*>(dst) = ezVec3(static_cast<float>(*reinterpret_cast<const double*>(src)));
}

void ezVisualScriptAssignGameObject(const void* src, void* dst)
{
  *reinterpret_cast<ezGameObjectHandle*>(dst) = *reinterpret_cast<const ezGameObjectHandle*>(src);
}

void ezVisualScriptAssignComponent(const void* src, void* dst)
{
  *reinterpret_cast<ezComponentHandle*>(dst) = *reinterpret_cast<const ezComponentHandle*>(src);
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
  m_pOwner = nullptr;
  m_Nodes.Clear();
  m_ExecutionConnections.Clear();
  m_DataConnections.Clear();
  m_LocalVariables.Clear();
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

    m_Nodes[uiDependency]->Execute(this, 0);
  }
}

void ezVisualScriptInstance::Configure(const ezVisualScriptResourceDescriptor& resource, ezGameObject* pOwner)
{
  Clear();

  m_pOwner = pOwner;

  if (m_pOwner)
  {
    m_pWorld = m_pOwner->GetWorld();
  }

  m_Nodes.Reserve(resource.m_Nodes.GetCount());

  ezUInt16 uiNodeId = 0;
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
    if (!pAbstract->GetCategory() == ezPropertyCategory::Member)
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

      continue;
    }

    if (!pAbstract->GetCategory() == ezPropertyCategory::Member)
      continue;

    ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pAbstract);
    ezReflectionUtils::SetMemberPropertyValue(pMember, pNode->m_pMessageToSend, prop.m_Value);
  }

  m_Nodes.PushBack(pNode);
}

void ezVisualScriptInstance::ExecuteScript(ezVisualScriptInstanceActivity* pActivity /*= nullptr*/)
{
  for (ezUInt32 i = 0; i < m_Nodes.GetCount(); ++i)
  {
    if (m_Nodes[i]->m_bStepNode)
    {
      ExecuteDependentNodes(i);

      m_Nodes[i]->m_bStepNode = false;
      m_Nodes[i]->Execute(this, 0);
    }
  }

  if (m_pActivity = pActivity)
  {
    m_pActivity->Clear();
  }
}

void ezVisualScriptInstance::HandleMessage(ezMessage& msg)
{
  /// \todo Precompute which nodes actually have message handlers!

  for (ezUInt32 i = 0; i < m_Nodes.GetCount(); ++i)
  {
    m_Nodes[i]->GetDynamicRTTI()->DispatchMessage(m_Nodes[i], msg);
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
      TargetNodeAndPin.m_AssignFunc(pValue, TargetNodeAndPin.m_pTargetData);
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

  ExecuteDependentNodes(TargetNode.m_uiTargetNode);
  m_Nodes[TargetNode.m_uiTargetNode]->Execute(this, TargetNode.m_uiTargetPin);

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

  auto it = s_DataPinAssignFunctions.Find(key);

  if (it.IsValid())
    return it.Value();

  return nullptr;
}

