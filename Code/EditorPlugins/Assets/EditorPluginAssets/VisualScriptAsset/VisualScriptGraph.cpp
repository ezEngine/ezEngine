#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraph.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptTypeRegistry.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

//////////////////////////////////////////////////////////////////////////
// ezVisualScriptPin
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptPin_Legacy, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezVisualScriptPin_Legacy::ezVisualScriptPin_Legacy(Type type, const ezVisualScriptPinDescriptor* pDescriptor, const ezDocumentObject* pObject)
  : ezPin(type, pDescriptor->m_sName, pDescriptor->m_Color, pObject)
{
  m_pDescriptor = pDescriptor;
  if (pDescriptor->m_PinType == ezVisualScriptPinDescriptor::PinType::Data)
  {
    m_Shape = Shape::Rect;
  }
}

const ezString& ezVisualScriptPin_Legacy::GetTooltip() const
{
  return m_pDescriptor->m_sTooltip;
}

//////////////////////////////////////////////////////////////////////////
// ezVisualScriptConnection
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptConnection_Legacy, 2, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////
// ezVisualScriptNodeManager
//////////////////////////////////////////////////////////////////////////

bool ezVisualScriptNodeManager_Legacy::InternalIsNode(const ezDocumentObject* pObject) const
{
  return pObject->GetType()->IsDerivedFrom(ezVisualScriptTypeRegistry::GetSingleton()->GetNodeBaseType());
}

void ezVisualScriptNodeManager_Legacy::InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& ref_node)
{
  const auto* pDesc = ezVisualScriptTypeRegistry::GetSingleton()->GetDescriptorForType(pObject->GetType());

  if (pDesc == nullptr)
    return;

  ref_node.m_Inputs.Reserve(pDesc->m_InputPins.GetCount());
  ref_node.m_Outputs.Reserve(pDesc->m_OutputPins.GetCount());

  for (const auto& pinDesc : pDesc->m_InputPins)
  {
    auto pPin = EZ_DEFAULT_NEW(ezVisualScriptPin_Legacy, ezPin::Type::Input, &pinDesc, pObject);
    ref_node.m_Inputs.PushBack(pPin);
  }

  for (const auto& pinDesc : pDesc->m_OutputPins)
  {
    auto pPin = EZ_DEFAULT_NEW(ezVisualScriptPin_Legacy, ezPin::Type::Output, &pinDesc, pObject);
    ref_node.m_Outputs.PushBack(pPin);
  }
}

void ezVisualScriptNodeManager_Legacy::GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& ref_types) const
{
  const ezRTTI* pNodeBaseType = ezVisualScriptTypeRegistry::GetSingleton()->GetNodeBaseType();

  ezRTTI::ForEachDerivedType(
    pNodeBaseType,
    [&](const ezRTTI* pRtti) { ref_types.PushBack(pRtti); },
    ezRTTI::ForEachOptions::ExcludeAbstract);
}

const ezRTTI* ezVisualScriptNodeManager_Legacy::GetConnectionType() const
{
  return ezGetStaticRTTI<ezVisualScriptConnection_Legacy>();
}

ezStatus ezVisualScriptNodeManager_Legacy::InternalCanConnect(const ezPin& source, const ezPin& target, CanConnectResult& out_result) const
{
  const ezVisualScriptPin_Legacy& pinSource = ezStaticCast<const ezVisualScriptPin_Legacy&>(source);
  const ezVisualScriptPin_Legacy& pinTarget = ezStaticCast<const ezVisualScriptPin_Legacy&>(target);

  if (pinSource.GetDescriptor()->m_PinType != pinTarget.GetDescriptor()->m_PinType)
  {
    out_result = CanConnectResult::ConnectNever;
    return ezStatus("Cannot connect data pins with execution pins.");
  }

  if (pinSource.GetDescriptor()->m_PinType == ezVisualScriptPinDescriptor::PinType::Data &&
      pinSource.GetDescriptor()->m_DataType != pinTarget.GetDescriptor()->m_DataType)
  {
    ezVisualScriptInstance::SetupPinDataTypeConversions();

    if (ezVisualScriptInstance::FindDataPinAssignFunction(pinSource.GetDescriptor()->m_DataType, pinTarget.GetDescriptor()->m_DataType) ==
        nullptr)
    {
      out_result = CanConnectResult::ConnectNever;
      return ezStatus(ezFmt("The pin data types are incompatible."));
    }
  }

  if (WouldConnectionCreateCircle(source, target))
  {
    out_result = CanConnectResult::ConnectNever;
    return ezStatus("Connecting these pins would create a circle in the graph.");
  }

  // only one connection is allowed on DATA input pins, execution input pins may have multiple incoming connections
  if (pinTarget.GetDescriptor()->m_PinType == ezVisualScriptPinDescriptor::PinType::Data && HasConnections(pinTarget))
  {
    out_result = CanConnectResult::ConnectNto1;
    return ezStatus(EZ_FAILURE);
  }

  // only one outgoing connection is allowed on EXECUTION pins, data pins may have multiple outgoing connections
  if (pinSource.GetDescriptor()->m_PinType == ezVisualScriptPinDescriptor::PinType::Execution && HasConnections(pinSource))
  {
    out_result = CanConnectResult::Connect1toN;
    return ezStatus(EZ_FAILURE);
  }

  out_result = CanConnectResult::ConnectNtoN;
  return ezStatus(EZ_SUCCESS);
}

const char* ezVisualScriptNodeManager_Legacy::GetTypeCategory(const ezRTTI* pRtti) const
{
  const ezVisualScriptNodeDescriptor* pDesc = ezVisualScriptTypeRegistry::GetSingleton()->GetDescriptorForType(pRtti);

  if (pDesc == nullptr)
    return nullptr;

  return pDesc->m_sCategory;
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezVisualScriptConnectionPatch_1_2 : public ezGraphPatch
{
public:
  ezVisualScriptConnectionPatch_1_2()
    : ezGraphPatch("ezVisualScriptConnection", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("ezVisualScriptConnection_Legacy");
  }
};

ezVisualScriptConnectionPatch_1_2 g_ezVisualScriptConnectionPatch_1_2;
