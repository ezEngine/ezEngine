#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

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
  if (pDescriptor->m_PinType == ezVisualScriptPinDescriptor::PinType::Data)
  {
    m_Shape = Shape::Rect;
  }
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

  for (const auto& pinDesc : pDesc->m_InputPins)
  {
    auto pPin = EZ_DEFAULT_NEW(ezVisualScriptPin, ezPin::Type::Input, &pinDesc, pObject);
    node.m_Inputs.PushBack(pPin);
  }

  for (const auto& pinDesc : pDesc->m_OutputPins)
  {
    auto pPin = EZ_DEFAULT_NEW(ezVisualScriptPin, ezPin::Type::Output, &pinDesc, pObject);
    node.m_Outputs.PushBack(pPin);
  }
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

const ezRTTI* ezVisualScriptNodeManager::GetConnectionType() const
{
  return ezGetStaticRTTI<ezVisualScriptConnection>();
}

ezStatus ezVisualScriptNodeManager::InternalCanConnect(const ezPin& source, const ezPin& target, CanConnectResult& out_Result) const
{
  const ezVisualScriptPin& pinSource = ezStaticCast<const ezVisualScriptPin&>(source);
  const ezVisualScriptPin& pinTarget = ezStaticCast<const ezVisualScriptPin&>(target);

  if (pinSource.GetDescriptor()->m_PinType != pinTarget.GetDescriptor()->m_PinType)
  {
    out_Result = CanConnectResult::ConnectNever;
    return ezStatus("Cannot connect data pins with execution pins.");
  }

  if (pinSource.GetDescriptor()->m_PinType == ezVisualScriptPinDescriptor::PinType::Data &&
      pinSource.GetDescriptor()->m_DataType != pinTarget.GetDescriptor()->m_DataType)
  {
    ezVisualScriptInstance::SetupPinDataTypeConversions();

    if (ezVisualScriptInstance::FindDataPinAssignFunction(pinSource.GetDescriptor()->m_DataType, pinTarget.GetDescriptor()->m_DataType) ==
        nullptr)
    {
      out_Result = CanConnectResult::ConnectNever;
      return ezStatus(ezFmt("The pin data types are incompatible."));
    }
  }

  if (WouldConnectionCreateCircle(source, target))
  {
    out_Result = CanConnectResult::ConnectNever;
    return ezStatus("Connecting these pins would create a circle in the graph.");
  }

  // only one connection is allowed on DATA input pins, execution input pins may have multiple incoming connections
  if (pinTarget.GetDescriptor()->m_PinType == ezVisualScriptPinDescriptor::PinType::Data && HasConnections(pinTarget))
  {
    out_Result = CanConnectResult::ConnectNto1;
    return ezStatus(EZ_FAILURE);
  }

  // only one outgoing connection is allowed on EXECUTION pins, data pins may have multiple outgoing connections
  if (pinSource.GetDescriptor()->m_PinType == ezVisualScriptPinDescriptor::PinType::Execution && HasConnections(pinSource))
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
