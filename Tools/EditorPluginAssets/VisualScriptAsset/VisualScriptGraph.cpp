#include <PCH.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraph.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptTypeRegistry.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

//////////////////////////////////////////////////////////////////////////
// ezVisualScriptPin
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptPin, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezVisualScriptPin::ezVisualScriptPin(Type type, const ezVisualScriptPinDescriptor* pDescriptor, const ezDocumentObject* pObject)
  : ezPin(type, pDescriptor->m_sName, pDescriptor->m_Color, pObject)
{
  m_pDescriptor = pDescriptor;
}

const ezString& ezVisualScriptPin::GetTooltip() const
{
  return m_pDescriptor->m_sTooltip;
}


//////////////////////////////////////////////////////////////////////////
// ezVisualScriptConnection
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptConnection, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;


//////////////////////////////////////////////////////////////////////////
// ezVisualScriptNodeManager
//////////////////////////////////////////////////////////////////////////

bool ezVisualScriptNodeManager::InternalIsNode(const ezDocumentObject* pObject) const
{
  return pObject->GetType()->IsDerivedFrom(ezVisualScriptTypeRegistry::GetSingleton()->GetNodeBaseType());
}

void ezVisualScriptNodeManager::InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& node)
{
  const auto* pDesc = ezVisualScriptTypeRegistry::GetSingleton()->GetDescriptorForType(pObject->GetType());

  if (pDesc == nullptr)
    return;

  node.m_Inputs.Reserve(pDesc->m_InputPins.GetCount());
  node.m_Outputs.Reserve(pDesc->m_OutputPins.GetCount());

  for (const auto& pin : pDesc->m_InputPins)
  {
    ezVisualScriptPin* pPin = EZ_DEFAULT_NEW(ezVisualScriptPin, ezPin::Type::Input, &pin, pObject);
    node.m_Inputs.PushBack(pPin);
  }

  for (const auto& pin : pDesc->m_OutputPins)
  {
    ezVisualScriptPin* pPin = EZ_DEFAULT_NEW(ezVisualScriptPin, ezPin::Type::Output, &pin, pObject);
    node.m_Outputs.PushBack(pPin);
  }
}

void ezVisualScriptNodeManager::InternalDestroyPins(const ezDocumentObject* pObject, NodeInternal& node)
{
  for (ezPin* pPin : node.m_Inputs)
  {
    EZ_DEFAULT_DELETE(pPin);
  }
  node.m_Inputs.Clear();
  for (ezPin* pPin : node.m_Outputs)
  {
    EZ_DEFAULT_DELETE(pPin);
  }
  node.m_Outputs.Clear();
}


void ezVisualScriptNodeManager::GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const
{
  const ezRTTI* pNodeBaseType = ezVisualScriptTypeRegistry::GetSingleton()->GetNodeBaseType();

  for (auto it = ezRTTI::GetFirstInstance(); it != nullptr; it = it->GetNextInstance())
  {
    if (it->IsDerivedFrom(pNodeBaseType) && !it->GetTypeFlags().IsSet(ezTypeFlags::Abstract))
      Types.PushBack(it);
  }
}

ezStatus ezVisualScriptNodeManager::InternalCanConnect(const ezPin* pSource, const ezPin* pTarget, CanConnectResult& out_Result) const
{
  const ezVisualScriptPin* pPinSource = ezDynamicCast<const ezVisualScriptPin*>(pSource);
  const ezVisualScriptPin* pPinTarget = ezDynamicCast<const ezVisualScriptPin*>(pTarget);

  EZ_ASSERT_DEBUG(pPinSource != nullptr && pPinTarget != nullptr, "Das ist eigentlich unmoeglich!");

  if (pPinSource->GetDescriptor()->m_PinType != pPinTarget->GetDescriptor()->m_PinType)
  {
    out_Result = CanConnectResult::ConnectNever;
    return ezStatus("Cannot connect data pins with execution pins.");
  }

  if (pPinSource->GetDescriptor()->m_PinType == ezVisualScriptPinDescriptor::Data &&
      pPinSource->GetDescriptor()->m_DataType != pPinTarget->GetDescriptor()->m_DataType)
  {
    ezVisualScriptInstance::SetupPinDataTypeConversions();

    if (ezVisualScriptInstance::FindDataPinAssignFunction(pPinSource->GetDescriptor()->m_DataType, pPinTarget->GetDescriptor()->m_DataType) == nullptr)
    {
      out_Result = CanConnectResult::ConnectNever;
      return ezStatus(ezFmt("The pin data types are incompatible."));
    }
  }

  if (WouldConnectionCreateCircle(pSource, pTarget))
  {
    out_Result = CanConnectResult::ConnectNever;
    return ezStatus("Connecting these pins would create a circle in the graph.");
  }

  // only one connection is allowed on DATA input pins, execution input pins may have multiple incoming connections
  if (pPinTarget->GetDescriptor()->m_PinType == ezVisualScriptPinDescriptor::Data && !pPinTarget->GetConnections().IsEmpty())
  {
    out_Result = CanConnectResult::ConnectNto1;
    return ezStatus(EZ_FAILURE);
  }

  // only one outgoing connection is allowed on EXECUTION pins, data pins may have multiple outgoing connections
  if (pPinSource->GetDescriptor()->m_PinType == ezVisualScriptPinDescriptor::Execution && !pPinSource->GetConnections().IsEmpty())
  {
    out_Result = CanConnectResult::Connect1toN;
    return ezStatus(EZ_FAILURE);
  }

  out_Result = CanConnectResult::ConnectNtoN;
  return ezStatus(EZ_SUCCESS);
}

const char* ezVisualScriptNodeManager::GetTypeCategory(const ezRTTI* pRtti) const
{
  const ezVisualScriptNodeDescriptor* pDesc = ezVisualScriptTypeRegistry::GetSingleton()->GetDescriptorForType(pRtti);

  if (pDesc == nullptr)
    return nullptr;

  return pDesc->m_sCategory;
}


