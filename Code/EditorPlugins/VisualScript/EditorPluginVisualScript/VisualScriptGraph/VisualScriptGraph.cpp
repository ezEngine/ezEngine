#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraph.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <Foundation/Utilities/DGMLWriter.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptPin, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptPin::ezVisualScriptPin(Type type, ezStringView sName, const ezVisualScriptNodeRegistry::PinDesc& pinDesc, const ezDocumentObject* pObject, ezUInt32 uiDataPinIndex)
  : ezPin(type, sName, pinDesc.GetColor(), pObject)
  , m_pDataType(pinDesc.m_pDataType)
  , m_uiDataPinIndex(uiDataPinIndex)
  , m_ScriptDataType(pinDesc.m_ScriptDataType)
  , m_bRequired(pinDesc.m_bRequired)
  , m_bHasDynamicPinProperty(pinDesc.m_sDynamicPinProperty.IsEmpty() == false)
  , m_bSplitExecution(pinDesc.m_bSplitExecution)
{
  if (pinDesc.IsExecutionPin())
  {
    m_Shape = Shape::Arrow;
  }
  else
  {
    m_Shape = (pinDesc.m_ScriptDataType == ezVisualScriptDataType::Array || pinDesc.m_ScriptDataType == ezVisualScriptDataType::Map) ? Shape::Rect : Shape::Circle;
  }
}

ezStringView ezVisualScriptPin::GetDataTypeName(ezVisualScriptDataType::Enum deductedType) const
{
  if (m_ScriptDataType == ezVisualScriptDataType::TypedPointer || m_ScriptDataType == ezVisualScriptDataType::EnumValue)
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

  if (sourceScriptDataType == ezVisualScriptDataType::EnumValue && pSourceDataType != nullptr &&
      targetScriptDataType == ezVisualScriptDataType::EnumValue && pTargetDataType != nullptr)
    return pSourceDataType == pTargetDataType;

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


ezVisualScriptDataType::Enum ezVisualScriptNodeManager::GetVariableType(ezTempHashedString sName) const
{
  ezVariant defaultValue;
  GetVariableDefaultValue(sName, defaultValue).IgnoreResult();
  return ezVisualScriptDataType::FromVariantType(defaultValue.GetType());
}

ezResult ezVisualScriptNodeManager::GetVariableDefaultValue(ezTempHashedString sName, ezVariant& out_Value) const
{
  if (GetRootObject()->GetChildren().IsEmpty() == false)
  {
    auto& typeAccessor = GetRootObject()->GetChildren()[0]->GetTypeAccessor();
    ezUInt32 uiNumVariables = typeAccessor.GetCount("Variables");
    for (ezUInt32 i = 0; i < uiNumVariables; ++i)
    {
      ezVariant variableUuid = typeAccessor.GetValue("Variables", i);
      if (variableUuid.IsA<ezUuid>() == false)
        continue;

      auto pVariableObject = GetObject(variableUuid.Get<ezUuid>());
      if (pVariableObject == nullptr)
        continue;

      ezVariant nameVar = pVariableObject->GetTypeAccessor().GetValue("Name");
      if (nameVar.IsA<ezHashedString>() == false || nameVar.Get<ezHashedString>() != sName)
        continue;

      out_Value = pVariableObject->GetTypeAccessor().GetValue("DefaultValue");
      return EZ_SUCCESS;
    }
  }

  return EZ_FAILURE;
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

void ezVisualScriptNodeManager::GetEntryNodes(const ezDocumentObject* pObject, ezDynamicArray<const ezDocumentObject*>& out_entryNodes) const
{
  ezHybridArray<const ezDocumentObject*, 64> nodeStack;
  nodeStack.PushBack(pObject);

  ezHashSet<const ezDocumentObject*> visitedNodes;
  ezHybridArray<const ezVisualScriptPin*, 16> pins;

  while (nodeStack.IsEmpty() == false)
  {
    const ezDocumentObject* pCurrentNode = nodeStack.PeekBack();
    nodeStack.PopBack();

    auto pNodeDesc = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pCurrentNode->GetType());
    if (ezVisualScriptNodeDescription::Type::IsEntry(pNodeDesc->m_Type))
    {
      out_entryNodes.PushBack(pCurrentNode);
      continue;
    }

    GetInputExecutionPins(pCurrentNode, pins);
    for (auto pPin : pins)
    {
      auto connections = GetConnections(*pPin);
      for (auto pConnection : connections)
      {
        const ezDocumentObject* pSourceNode = pConnection->GetSourcePin().GetParent();
        if (visitedNodes.Insert(pSourceNode))
          continue;

        nodeStack.PushBack(pSourceNode);
      }
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

ezStringView ezVisualScriptNodeManager::GetNiceFunctionName(const ezDocumentObject* pObject)
{
  ezStringView sFunctionName = pObject->GetType()->GetTypeName();

  if (const char* szSeparator = sFunctionName.FindLastSubString("::"))
  {
    sFunctionName = ezStringView(szSeparator + 2, sFunctionName.GetEndPointer());
  }

  return sFunctionName;
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

bool ezVisualScriptNodeManager::IsCoroutine(const ezDocumentObject* pObject) const
{
  auto pNodeDesc = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
  if (pNodeDesc != nullptr && ezVisualScriptNodeDescription::Type::MakesOuterCoroutine(pNodeDesc->m_Type))
  {
    return true;
  }

  return m_CoroutineObjects.Contains(pObject);
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
  auto CreatePins = [&](const ezVisualScriptNodeRegistry::PinDesc& pinDesc, ezPin::Type type, ezDynamicArray<ezUniquePtr<ezPin>>& out_pins, ezUInt32& inout_dataPinIndex)
  {
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
      ezUInt32 uiDataPinIndex = ezInvalidIndex;
      if (pinDesc.IsDataPin())
      {
        uiDataPinIndex = inout_dataPinIndex;
        ++inout_dataPinIndex;
      }

      auto pPin = EZ_DEFAULT_NEW(ezVisualScriptPin, type, dynamicPinNames[i], pinDesc, pObject, uiDataPinIndex);
      out_pins.PushBack(pPin);
    }
  };

  ezUInt32 uiDataPinIndex = 0;
  for (const auto& pinDesc : pNodeDesc->m_InputPins)
  {
    CreatePins(pinDesc, ezPin::Type::Input, ref_node.m_Inputs, uiDataPinIndex);
  }

  uiDataPinIndex = 0;
  for (const auto& pinDesc : pNodeDesc->m_OutputPins)
  {
    CreatePins(pinDesc, ezPin::Type::Output, ref_node.m_Outputs, uiDataPinIndex);
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
      UpdateCoroutine(targetPin.GetParent(), connection, true);
    }
    break;

    case ezDocumentNodeManagerEvent::Type::BeforePinsDisonnected:
    {
      auto& connection = GetConnection(e.m_pObject);
      auto& targetPin = connection.GetTargetPin();
      DeductType(targetPin.GetParent(), &targetPin, false);
      UpdateCoroutine(targetPin.GetParent(), connection, false);
    }
    break;

    case ezDocumentNodeManagerEvent::Type::AfterNodeAdded:
    {
      DeductType(e.m_pObject);
    }
    break;

    default:
      break;
  }
}

void ezVisualScriptNodeManager::PropertyEventsHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (IsNode(e.m_pObject))
  {
    DeductType(e.m_pObject);
  }
  else if (e.m_sProperty == "Name" || e.m_sProperty == "DefaultValue") // a variable's name or default value has changed, re-run type deduction
  {
    for (auto pObject : GetRootObject()->GetChildren())
    {
      if (IsNode(pObject) == false)
        continue;

      DeductType(pObject);
    }
  }
}

void ezVisualScriptNodeManager::DeductType(const ezDocumentObject* pObject, const ezPin* pChangedPin, bool bConnected)
{
  auto pNodeDesc = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
  if (pNodeDesc == nullptr || pNodeDesc->NeedsTypeDeduction() == false)
    return;

  if (pChangedPin != nullptr)
  {
    auto pVsPin = ezStaticCast<const ezVisualScriptPin*>(pChangedPin);
    if (pVsPin->GetScriptDataType() != ezVisualScriptDataType::Any)
      return;
  }

  ezVisualScriptDataType::Enum deductedType = ezVisualScriptDataType::Invalid;
  ezHybridArray<const ezVisualScriptPin*, 16> pins;

  if (pNodeDesc->m_TypeDeductionMode == ezVisualScriptNodeRegistry::NodeDesc::TypeDeductionMode::FromInputPins)
  {
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
  }
  else if (pNodeDesc->m_TypeDeductionMode == ezVisualScriptNodeRegistry::NodeDesc::TypeDeductionMode::FromTypeProperty)
  {
    auto typeVar = pObject->GetTypeAccessor().GetValue("Type");
    if (typeVar.IsA<ezInt64>())
    {
      deductedType = static_cast<ezVisualScriptDataType::Enum>(typeVar.Get<ezInt64>());
    }
  }
  else if (pNodeDesc->m_TypeDeductionMode == ezVisualScriptNodeRegistry::NodeDesc::TypeDeductionMode::FromNameProperty)
  {
    auto nameVar = pObject->GetTypeAccessor().GetValue("Name");
    if (nameVar.IsA<ezString>())
    {
      deductedType = GetVariableType(ezTempHashedString(nameVar.Get<ezString>()));
    }
  }

  ezEnum<ezVisualScriptDataType> oldDeductedType = ezVisualScriptDataType::Invalid;
  m_ObjectToDeductedType.Insert(pObject, deductedType, &oldDeductedType);

  if (deductedType != oldDeductedType)
  {
    m_NodeChangedEvent.Broadcast(pObject);
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

void ezVisualScriptNodeManager::UpdateCoroutine(const ezDocumentObject* pTargetNode, const ezConnection& changedConnection, bool bConnected)
{
  auto vsPin = static_cast<const ezVisualScriptPin&>(changedConnection.GetTargetPin());
  if (vsPin.IsExecutionPin() == false)
    return;

  ezHybridArray<const ezDocumentObject*, 16> entryNodes;
  GetEntryNodes(pTargetNode, entryNodes);

  for (auto pEntryNode : entryNodes)
  {
    const bool bWasCoroutine = m_CoroutineObjects.Contains(pEntryNode);
    const bool bIsCoroutine = IsConnectedToCoroutine(pEntryNode, changedConnection, bConnected);

    if (bWasCoroutine != bIsCoroutine)
    {
      if (bIsCoroutine)
      {
        m_CoroutineObjects.Insert(pEntryNode);
      }
      else
      {
        m_CoroutineObjects.Remove(pEntryNode);
      }

      m_NodeChangedEvent.Broadcast(pEntryNode);
    }
  }
}

bool ezVisualScriptNodeManager::IsConnectedToCoroutine(const ezDocumentObject* pEntryNode, const ezConnection& changedConnection, bool bConnected) const
{
  ezHybridArray<const ezDocumentObject*, 64> nodeStack;
  nodeStack.PushBack(pEntryNode);

  ezHashSet<const ezDocumentObject*> visitedNodes;
  ezHybridArray<const ezVisualScriptPin*, 16> pins;

  while (nodeStack.IsEmpty() == false)
  {
    const ezDocumentObject* pCurrentNode = nodeStack.PeekBack();
    nodeStack.PopBack();

    auto pNodeDesc = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pCurrentNode->GetType());
    if (ezVisualScriptNodeDescription::Type::MakesOuterCoroutine(pNodeDesc->m_Type))
    {
      return true;
    }

    GetOutputExecutionPins(pCurrentNode, pins);
    for (auto pPin : pins)
    {
      if (pPin->SplitExecution())
        continue;

      auto connections = GetConnections(*pPin);
      for (auto pConnection : connections)
      {
        // the connection is about to be disconnected so we ignore it here
        if (bConnected == false && pConnection == &changedConnection)
          continue;

        const ezDocumentObject* pTargetNode = pConnection->GetTargetPin().GetParent();
        if (visitedNodes.Insert(pTargetNode))
          continue;

        nodeStack.PushBack(pTargetNode);
      }
    }
  }

  return false;
}
