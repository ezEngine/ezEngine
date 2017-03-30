#include <PCH.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>
#include <Foundation/Strings/HashedString.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Core/World/Declarations.h>

ezMap<ezVisualScriptInstance::AssignFuncKey, ezVisualScriptDataPinAssignFunc> ezVisualScriptInstance::s_DataPinAssignFunctions;

void ezVisualScriptAssignNumberNumber(const void* src, void* dst)
{
  *reinterpret_cast<double*>(dst) = *reinterpret_cast<const double*>(src);
}

void ezVisualScriptAssignBoolBool(const void* src, void* dst)
{
  *reinterpret_cast<bool*>(dst) = *reinterpret_cast<const bool*>(src);
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
  m_Nodes.Reserve(resource.m_Nodes.GetCount());

  ezUInt16 uiNodeId = 0;
  for (const auto& node : resource.m_Nodes)
  {
    ezVisualScriptNode* pNode = static_cast<ezVisualScriptNode*>(node.m_pType->GetAllocator()->Allocate());
    pNode->m_uiNodeID = uiNodeId++;

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

  m_ExecutionConnections.Reserve(resource.m_ExecutionPaths.GetCount());

  for (const auto& con : resource.m_ExecutionPaths)
  {
    ConnectExecutionPins(con.m_uiSourceNode, con.m_uiOutputPin, con.m_uiTargetNode, con.m_uiInputPin);
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
      m_Nodes[i]->Execute(this, 0);
    }
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

void ezVisualScriptInstance::ConnectDataPins(ezUInt16 uiSourceNode, ezUInt8 uiSourcePin, ezUInt16 uiTargetNode, ezUInt8 uiTargetPin)
{
  DataPinConnection& con = m_DataConnections[((ezUInt32)uiSourceNode << 16) | (ezUInt32)uiSourcePin].ExpandAndGetRef();
  con.m_uiTargetNode = uiTargetNode;
  con.m_uiTargetPin = uiTargetPin;
  con.m_AssignFunc = nullptr;
  con.m_pTargetData = nullptr;

  ezVisualScriptDataPinType::Enum sourceType = ezVisualScriptDataPinType::None;
  ezVisualScriptDataPinType::Enum targetType = ezVisualScriptDataPinType::None;

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
          targetType = pAttr->m_DataType;
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
          sourceType = pAttr->m_DataType;
          break;
        }
      }
    }
  }

  con.m_AssignFunc = FindDataPinAssignFunction(sourceType, targetType);
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

  ExecPinConnection TargetNode;
  if (!m_ExecutionConnections.TryGetValue(uiConnectionID, TargetNode))
    return;

  ExecuteDependentNodes(TargetNode.m_uiTargetNode);
  m_Nodes[TargetNode.m_uiTargetNode]->Execute(this, TargetNode.m_uiTargetPin);
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

