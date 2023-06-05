#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraph.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <Foundation/Utilities/DGMLWriter.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptPin, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptPin::ezVisualScriptPin(Type type, ezStringView sName, const ezVisualScriptNodeRegistry::PinDesc& pinDesc, const ezDocumentObject* pObject, ezUInt32 uiPinIndex)
  : ezPin(type, sName, pinDesc.GetColor(), pObject)
  , m_pDataType(pinDesc.m_pDataType)
  , m_uiPinIndex(uiPinIndex)
  , m_ScriptDataType(pinDesc.m_ScriptDataType)
  , m_bRequired(pinDesc.m_bRequired)
  , m_bHasDynamicPinProperty(pinDesc.m_sDynamicPinProperty.IsEmpty() == false)
{
  m_Shape = pinDesc.IsExecutionPin() ? Shape::Arrow : Shape::Circle;
}

ezStringView ezVisualScriptPin::GetDataTypeName(ezVisualScriptDataType::Enum deductedType) const
{
  if (m_ScriptDataType == ezVisualScriptDataType::TypedPointer)
  {
    return m_pDataType->GetTypeName();
  }

  ezVisualScriptDataType::Enum finalDataType = m_ScriptDataType;
  if (finalDataType == ezVisualScriptDataType::Any && deductedType != ezVisualScriptDataType::Invalid)
    finalDataType = deductedType;

  return ezVisualScriptDataType::GetName(finalDataType);
}

bool ezVisualScriptPin::CanConvertTo(const ezVisualScriptPin& targetPin, ezVisualScriptDataType::Enum deductedSourceDataType /*= ezVisualScriptDataType::Invalid*/, ezVisualScriptDataType::Enum deductedTargetDataType /*= ezVisualScriptDataType::Invalid*/) const
{
  ezVisualScriptDataType::Enum sourceScriptDataType = m_ScriptDataType;
  const ezRTTI* pSourceDataType = m_pDataType;
  if (sourceScriptDataType == ezVisualScriptDataType::Any && deductedSourceDataType != ezVisualScriptDataType::Invalid)
  {
    sourceScriptDataType = deductedSourceDataType;
    pSourceDataType = ezVisualScriptDataType::GetRtti(sourceScriptDataType);
  }

  ezVisualScriptDataType::Enum targetScriptDataType = targetPin.GetScriptDataType();
  const ezRTTI* pTargetDataType = targetPin.GetDataType();
  EZ_ASSERT_DEV(targetScriptDataType != ezVisualScriptDataType::Invalid, "Invalid script data type '{}'", targetPin.GetDataTypeName(deductedTargetDataType));
  if (targetScriptDataType == ezVisualScriptDataType::Any && deductedTargetDataType != ezVisualScriptDataType::Invalid)
  {
    targetScriptDataType = deductedTargetDataType;
    pTargetDataType = ezVisualScriptDataType::GetRtti(targetScriptDataType);
  }

  if (sourceScriptDataType == ezVisualScriptDataType::TypedPointer && pSourceDataType != nullptr &&
      targetScriptDataType == ezVisualScriptDataType::TypedPointer && pTargetDataType != nullptr)
    return pSourceDataType->IsDerivedFrom(pTargetDataType);

  if (sourceScriptDataType == ezVisualScriptDataType::Any ||
      targetScriptDataType == ezVisualScriptDataType::Any)
    return true;

  return ezVisualScriptDataType::CanConvertTo(sourceScriptDataType, targetScriptDataType);
}

//////////////////////////////////////////////////////////////////////////

ezVisualScriptNodeManager::ezVisualScriptNodeManager()
{
  m_NodeEvents.AddEventHandler(ezMakeDelegate(&ezVisualScriptNodeManager::NodeEventsHandler, this));
  m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezVisualScriptNodeManager::PropertyEventsHandler, this));
}

ezVisualScriptNodeManager::~ezVisualScriptNodeManager() = default;

ezHashedString ezVisualScriptNodeManager::GetScriptBaseClass() const
{
  ezHashedString sBaseClass;
  if (GetRootObject()->GetChildren().IsEmpty() == false)
  {
    ezVariant baseClass = GetRootObject()->GetChildren()[0]->GetTypeAccessor().GetValue("BaseClass");
    if (baseClass.IsA<ezString>())
    {
      sBaseClass.Assign(baseClass.Get<ezString>());
    }
  }
  return sBaseClass;
}

bool ezVisualScriptNodeManager::IsFilteredByBaseClass(const ezRTTI* pNodeType, const ezVisualScriptNodeRegistry::NodeDesc& nodeDesc, const ezHashedString& sBaseClass, bool bLogWarning /*= false*/) const
{
  if (nodeDesc.m_sFilterByBaseClass.IsEmpty() == false && nodeDesc.m_sFilterByBaseClass != sBaseClass)
  {
    if (bLogWarning)
    {
      ezStringView sTypeName = pNodeType->GetTypeName();
      sTypeName.TrimWordStart(ezVisualScriptNodeRegistry::s_szTypeNamePrefix);

      ezLog::Warning("The base class function '{}' is not a function of the currently selected base class '{}' and will be skipped", sTypeName, sBaseClass);
    }

    return true;
  }

  return false;
}

void ezVisualScriptNodeManager::GetInputExecutionPins(const ezDocumentObject* pObject, ezDynamicArray<const ezVisualScriptPin*>& out_pins) const
{
  out_pins.Clear();

  auto pins = GetInputPins(pObject);
  for (auto& pPin : pins)
  {
    auto& vsPin = ezStaticCast<const ezVisualScriptPin&>(*pPin);
    if (vsPin.IsExecutionPin())
    {
      out_pins.PushBack(&vsPin);
    }
  }
}

void ezVisualScriptNodeManager::GetOutputExecutionPins(const ezDocumentObject* pObject, ezDynamicArray<const ezVisualScriptPin*>& out_pins) const
{
  out_pins.Clear();

  auto pins = GetOutputPins(pObject);
  for (auto& pPin : pins)
  {
    auto& vsPin = ezStaticCast<const ezVisualScriptPin&>(*pPin);
    if (vsPin.IsExecutionPin())
    {
      out_pins.PushBack(&vsPin);
    }
  }
}

void ezVisualScriptNodeManager::GetInputDataPins(const ezDocumentObject* pObject, ezDynamicArray<const ezVisualScriptPin*>& out_pins) const
{
  out_pins.Clear();

  auto pins = GetInputPins(pObject);
  for (auto& pPin : pins)
  {
    auto& vsPin = ezStaticCast<const ezVisualScriptPin&>(*pPin);
    if (vsPin.IsDataPin())
    {
      out_pins.PushBack(&vsPin);
    }
  }
}

void ezVisualScriptNodeManager::GetOutputDataPins(const ezDocumentObject* pObject, ezDynamicArray<const ezVisualScriptPin*>& out_pins) const
{
  out_pins.Clear();

  auto pins = GetOutputPins(pObject);
  for (auto& pPin : pins)
  {
    auto& vsPin = ezStaticCast<const ezVisualScriptPin&>(*pPin);
    if (vsPin.IsDataPin())
    {
      out_pins.PushBack(&vsPin);
    }
  }
}

// static
ezStringView ezVisualScriptNodeManager::GetNiceTypeName(const ezDocumentObject* pObject)
{
  ezStringView sTypeName = pObject->GetType()->GetTypeName();

  while (sTypeName.TrimWordStart(ezVisualScriptNodeRegistry::s_szTypeNamePrefix) ||
         sTypeName.TrimWordStart("Builtin_"))
  {
  }

  if (const char* szAngleBracket = sTypeName.FindSubString("<"))
  {
    sTypeName = ezStringView(sTypeName.GetStartPointer(), szAngleBracket);
  }

  return sTypeName;
}

void ezVisualScriptNodeManager::DeductType(const ezDocumentObject* pObject, const ezPin* pChangedPin, bool bConnected)
{
  if (pChangedPin != nullptr)
  {
    auto pVsPin = ezStaticCast<const ezVisualScriptPin*>(pChangedPin);
    if (pVsPin->GetScriptDataType() != ezVisualScriptDataType::Any)
      return;
  }

  ezVisualScriptDataType::Enum deductedType = ezVisualScriptDataType::Invalid;

  ezHybridArray<const ezVisualScriptPin*, 16> pins;
  GetInputDataPins(pObject, pins);
  for (auto pPin : pins)
  {
    if (pPin->GetScriptDataType() != ezVisualScriptDataType::Any)
      continue;

    // the pin is about to be disconnected so we ignore it here
    if (bConnected == false && pPin == pChangedPin)
      continue;

    deductedType = ezMath::Max(deductedType, GetDeductedType(*pPin));
  }

  if (deductedType == ezVisualScriptDataType::Invalid)
  {
    auto typeVar = pObject->GetTypeAccessor().GetValue("Type");
    if (typeVar.IsValid())
    {
      deductedType = static_cast<ezVisualScriptDataType::Enum>(typeVar.Get<ezInt64>());
    }
  }

  ezEnum<ezVisualScriptDataType> oldDeductedType = ezVisualScriptDataType::Invalid;
  m_ObjectToDeductedType.Insert(pObject, deductedType, &oldDeductedType);

  if (deductedType != oldDeductedType)
  {
    m_DeductedTypeChangedEvent.Broadcast(pObject);
  }

  GetOutputDataPins(pObject, pins);
  for (auto pPin : pins)
  {
    if (pPin->GetScriptDataType() != ezVisualScriptDataType::Any)
      continue;

    auto connections = GetConnections(*pPin);
    if (connections.IsEmpty() == false)
    {
      for (auto& connection : connections)
      {
        auto& targetPin = connection->GetTargetPin();
        DeductType(targetPin.GetParent(), &targetPin);
      }
    }
  }
}

ezVisualScriptDataType::Enum ezVisualScriptNodeManager::GetDeductedType(const ezVisualScriptPin& pin) const
{
  if (pin.GetType() == ezPin::Type::Input)
  {
    auto connections = GetConnections(pin);
    if (connections.IsEmpty() == false)
    {
      return GetDeductedType(static_cast<const ezVisualScriptPin&>(connections[0]->GetSourcePin()));
    }
    else
    {
      ezVariant var = pin.GetParent()->GetTypeAccessor().GetValue(pin.GetName());
      return ezVisualScriptDataType::FromVariantType(var.GetType());
    }

    return ezVisualScriptDataType::Invalid;
  }

  if (pin.GetScriptDataType() == ezVisualScriptDataType::Any)
  {
    return GetDeductedType(pin.GetParent());
  }

  return pin.GetScriptDataType();
}

ezVisualScriptDataType::Enum ezVisualScriptNodeManager::GetDeductedType(const ezDocumentObject* pObject) const
{
  ezEnum<ezVisualScriptDataType> dataType = ezVisualScriptDataType::Invalid;
  m_ObjectToDeductedType.TryGetValue(pObject, dataType);
  return dataType;
}

bool ezVisualScriptNodeManager::InternalIsNode(const ezDocumentObject* pObject) const
{
  return pObject->GetType()->IsDerivedFrom(ezVisualScriptNodeRegistry::GetSingleton()->GetNodeBaseType());
}

bool ezVisualScriptNodeManager::InternalIsDynamicPinProperty(const ezDocumentObject* pObject, const ezAbstractProperty* pProp) const
{
  auto pNodeDesc = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());

  if (pNodeDesc != nullptr && pNodeDesc->m_bHasDynamicPins)
  {
    ezTempHashedString sPropNameHashed = ezStringView(pProp->GetPropertyName());
    for (auto& pinDesc : pNodeDesc->m_InputPins)
    {
      if (pinDesc.m_sDynamicPinProperty == sPropNameHashed)
      {
        return true;
      }
    }

    for (auto& pinDesc : pNodeDesc->m_OutputPins)
    {
      if (pinDesc.m_sDynamicPinProperty == sPropNameHashed)
      {
        return true;
      }
    }
  }

  return false;
}

ezStatus ezVisualScriptNodeManager::InternalCanConnect(const ezPin& source, const ezPin& target, CanConnectResult& out_result) const
{
  const ezVisualScriptPin& pinSource = ezStaticCast<const ezVisualScriptPin&>(source);
  const ezVisualScriptPin& pinTarget = ezStaticCast<const ezVisualScriptPin&>(target);

  if (pinSource.IsExecutionPin() != pinTarget.IsExecutionPin())
  {
    out_result = CanConnectResult::ConnectNever;
    return ezStatus("Cannot connect data pins with execution pins.");
  }

  if (pinSource.IsDataPin() && pinSource.CanConvertTo(pinTarget) == false)
  {
    out_result = CanConnectResult::ConnectNever;
    return ezStatus(ezFmt("The pin data types are incompatible."));
  }

  if (WouldConnectionCreateCircle(source, target))
  {
    out_result = CanConnectResult::ConnectNever;
    return ezStatus("Connecting these pins would create a circle in the graph.");
  }

  // only one connection is allowed on DATA input pins, execution input pins may have multiple incoming connections
  if (pinTarget.IsDataPin() && HasConnections(pinTarget))
  {
    out_result = CanConnectResult::ConnectNto1;
    return ezStatus(EZ_FAILURE);
  }

  // only one outgoing connection is allowed on EXECUTION pins, data pins may have multiple outgoing connections
  if (pinSource.IsExecutionPin() && HasConnections(pinSource))
  {
    out_result = CanConnectResult::Connect1toN;
    return ezStatus(EZ_FAILURE);
  }

  out_result = CanConnectResult::ConnectNtoN;
  return ezStatus(EZ_SUCCESS);
}

void ezVisualScriptNodeManager::InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& ref_node)
{
  auto pNodeDesc = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());

  if (pNodeDesc == nullptr)
    return;

  ezHybridArray<ezString, 16> dynamicPinNames;
  auto CreatePins = [&](const ezVisualScriptNodeRegistry::PinDesc& pinDesc, ezPin::Type type, ezDynamicArray<ezUniquePtr<ezPin>>& out_pins) {
    if (pinDesc.m_sDynamicPinProperty.IsEmpty() == false)
    {
      GetDynamicPinNames(pObject, pinDesc.m_sDynamicPinProperty, pinDesc.m_sName, dynamicPinNames);
    }
    else
    {
      dynamicPinNames.Clear();
      dynamicPinNames.PushBack(pinDesc.m_sName.GetView());
    }

    for (ezUInt32 i = 0; i < dynamicPinNames.GetCount(); ++i)
    {
      auto pPin = EZ_DEFAULT_NEW(ezVisualScriptPin, type, dynamicPinNames[i], pinDesc, pObject, out_pins.GetCount());
      out_pins.PushBack(pPin);
    }
  };

  for (const auto& pinDesc : pNodeDesc->m_InputPins)
  {
    CreatePins(pinDesc, ezPin::Type::Input, ref_node.m_Inputs);
  }

  for (const auto& pinDesc : pNodeDesc->m_OutputPins)
  {
    CreatePins(pinDesc, ezPin::Type::Output, ref_node.m_Outputs);
  }
}

void ezVisualScriptNodeManager::GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const
{
  ezHashedString sBaseClass = GetScriptBaseClass();

  for (auto it : ezVisualScriptNodeRegistry::GetSingleton()->GetAllNodeTypes())
  {
    if (IsFilteredByBaseClass(it.Key(), it.Value(), sBaseClass))
      continue;

    if (!it.Key()->GetTypeFlags().IsSet(ezTypeFlags::Abstract))
    {
      Types.PushBack(it.Key());
    }
  }
}

void ezVisualScriptNodeManager::NodeEventsHandler(const ezDocumentNodeManagerEvent& e)
{
  switch (e.m_EventType)
  {
    case ezDocumentNodeManagerEvent::Type::AfterPinsConnected:
    {
      auto& connection = GetConnection(e.m_pObject);
      auto& targetPin = connection.GetTargetPin();
      DeductType(targetPin.GetParent(), &targetPin, true);
    }
    break;

    case ezDocumentNodeManagerEvent::Type::BeforePinsDisonnected:
    {
      auto& connection = GetConnection(e.m_pObject);
      auto& targetPin = connection.GetTargetPin();
      DeductType(targetPin.GetParent(), &targetPin, false);
    }
    break;

    case ezDocumentNodeManagerEvent::Type::AfterNodeAdded:
    {
      auto pNodeDesc = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(e.m_pObject->GetType());
      if (pNodeDesc->m_bNeedsDataTypeDeduction)
      {
        DeductType(e.m_pObject);
      }
    }
    break;

    default:
      break;
  }
}

void ezVisualScriptNodeManager::PropertyEventsHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (IsNode(e.m_pObject) == false)
    return;

  auto pNodeDesc = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(e.m_pObject->GetType());

  if (pNodeDesc->m_bNeedsDataTypeDeduction && e.m_EventType == ezDocumentObjectPropertyEvent::Type::PropertySet)
  {
    DeductType(e.m_pObject);
  }
}
