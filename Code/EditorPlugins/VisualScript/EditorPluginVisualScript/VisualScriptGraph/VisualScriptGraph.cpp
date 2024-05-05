#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraph.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <Foundation/Utilities/DGMLWriter.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptPin, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptPin::ezVisualScriptPin(Type type, ezStringView sName, const ezVisualScriptNodeRegistry::PinDesc& pinDesc, const ezDocumentObject* pObject, ezUInt32 uiDataPinIndex, ezUInt32 uiElementIndex)
  : ezPin(type, sName, pinDesc.GetColor(), pObject)
  , m_pDesc(&pinDesc)
  , m_uiDataPinIndex(uiDataPinIndex)
  , m_uiElementIndex(uiElementIndex)
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

ezVisualScriptPin::~ezVisualScriptPin()
{
  auto pManager = static_cast<ezVisualScriptNodeManager*>(const_cast<ezDocumentObjectManager*>(GetParent()->GetDocumentObjectManager()));
  pManager->RemoveDeductedPinType(*this);
}

ezVisualScriptDataType::Enum ezVisualScriptPin::GetResolvedScriptDataType() const
{
  auto scriptDataType = GetScriptDataType();
  if (scriptDataType == ezVisualScriptDataType::AnyPointer || scriptDataType == ezVisualScriptDataType::Any)
  {
    auto pManager = static_cast<const ezVisualScriptNodeManager*>(GetParent()->GetDocumentObjectManager());
    return pManager->GetDeductedType(*this);
  }

  return scriptDataType;
}

ezStringView ezVisualScriptPin::GetDataTypeName() const
{
  ezVisualScriptDataType::Enum resolvedDataType = GetResolvedScriptDataType();
  if (resolvedDataType == ezVisualScriptDataType::Invalid)
  {
    return ezVisualScriptDataType::GetName(GetScriptDataType());
  }

  if ((resolvedDataType == ezVisualScriptDataType::TypedPointer ||
        resolvedDataType == ezVisualScriptDataType::EnumValue || resolvedDataType == ezVisualScriptDataType::BitflagValue) &&
      GetDataType() != nullptr)
  {
    return GetDataType()->GetTypeName();
  }

  return ezVisualScriptDataType::GetName(resolvedDataType);
}

bool ezVisualScriptPin::CanConvertTo(const ezVisualScriptPin& targetPin, bool bUseResolvedDataTypes /*= true*/) const
{
  ezVisualScriptDataType::Enum sourceScriptDataType = bUseResolvedDataTypes ? GetResolvedScriptDataType() : GetScriptDataType();
  ezVisualScriptDataType::Enum targetScriptDataType = bUseResolvedDataTypes ? targetPin.GetResolvedScriptDataType() : targetPin.GetScriptDataType();

  const ezRTTI* pSourceDataType = GetDataType();
  const ezRTTI* pTargetDataType = targetPin.GetDataType();

  if (ezVisualScriptDataType::IsPointer(sourceScriptDataType) &&
      targetScriptDataType == ezVisualScriptDataType::AnyPointer)
    return true;

  if (sourceScriptDataType == ezVisualScriptDataType::TypedPointer && pSourceDataType != nullptr &&
      targetScriptDataType == ezVisualScriptDataType::TypedPointer && pTargetDataType != nullptr)
    return pSourceDataType->IsDerivedFrom(pTargetDataType);

  if (sourceScriptDataType == ezVisualScriptDataType::EnumValue && pSourceDataType != nullptr &&
      targetScriptDataType == ezVisualScriptDataType::EnumValue && pTargetDataType != nullptr)
    return pSourceDataType == pTargetDataType;

  if (sourceScriptDataType == ezVisualScriptDataType::BitflagValue && pSourceDataType != nullptr &&
      targetScriptDataType == ezVisualScriptDataType::BitflagValue && pTargetDataType != nullptr)
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

ezResult ezVisualScriptNodeManager::GetVariableDefaultValue(ezTempHashedString sName, ezVariant& out_value) const
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

      out_value = pVariableObject->GetTypeAccessor().GetValue("DefaultValue");
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
  ezEnum<ezVisualScriptDataType> dataType = ezVisualScriptDataType::Invalid;
  m_PinToDeductedType.TryGetValue(&pin, dataType);
  return dataType;
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

bool ezVisualScriptNodeManager::IsLoop(const ezDocumentObject* pObject) const
{
  auto pNodeDesc = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
  if (pNodeDesc != nullptr && ezVisualScriptNodeDescription::Type::IsLoop(pNodeDesc->m_Type))
  {
    return true;
  }

  return false;
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
    ezTempHashedString sPropNameHashed = ezTempHashedString(pProp->GetPropertyName());
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

  if (pinSource.IsDataPin() && pinSource.CanConvertTo(pinTarget, false) == false)
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

      auto pPin = EZ_DEFAULT_NEW(ezVisualScriptPin, type, dynamicPinNames[i], pinDesc, pObject, uiDataPinIndex, i);
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

void ezVisualScriptNodeManager::GetNodeCreationTemplates(ezDynamicArray<ezNodeCreationTemplate>& out_templates) const
{
  auto pRegistry = ezVisualScriptNodeRegistry::GetSingleton();
  auto propertyValues = pRegistry->GetPropertyValues();
  ezHashedString sBaseClass = GetScriptBaseClass();

  for (auto& nodeTemplate : pRegistry->GetNodeCreationTemplates())
  {
    const ezRTTI* pNodeType = nodeTemplate.m_pType;

    if (IsFilteredByBaseClass(pNodeType, *pRegistry->GetNodeDescForType(pNodeType), sBaseClass))
      continue;

    if (!pNodeType->GetTypeFlags().IsSet(ezTypeFlags::Abstract))
    {
      auto& temp = out_templates.ExpandAndGetRef();
      temp.m_pType = pNodeType;
      temp.m_sTypeName = nodeTemplate.m_sTypeName;
      temp.m_sCategory = nodeTemplate.m_sCategory;
      temp.m_PropertyValues = propertyValues.GetSubArray(nodeTemplate.m_uiPropertyValuesStart, nodeTemplate.m_uiPropertyValuesCount);
    }
  }

  // Getter and setter templates for variables
  if (GetRootObject()->GetChildren().IsEmpty() == false)
  {
    static ezHashedString sVariables = ezMakeHashedString("Variables");
    static ezHashedString sName = ezMakeHashedString("Name");

    m_PropertyValues.Clear();
    m_VariableNodeTypeNames.Clear();

    ezStringBuilder sNodeTypeName;

    auto& typeAccessor = GetRootObject()->GetChildren()[0]->GetTypeAccessor();
    const ezUInt32 uiNumVariables = typeAccessor.GetCount(sVariables.GetView());
    for (ezUInt32 i = 0; i < uiNumVariables; ++i)
    {
      ezVariant variableUuid = typeAccessor.GetValue(sVariables.GetView(), i);
      if (variableUuid.IsA<ezUuid>() == false)
        continue;

      auto pVariableObject = GetObject(variableUuid.Get<ezUuid>());
      if (pVariableObject == nullptr)
        continue;

      ezVariant nameVar = pVariableObject->GetTypeAccessor().GetValue(sName.GetView());
      if (nameVar.IsA<ezHashedString>() == false)
        continue;

      ezHashedString sVariableName = nameVar.Get<ezHashedString>();

      ezUInt32 uiStart = m_PropertyValues.GetCount();
      m_PropertyValues.PushBack({sName, nameVar});

      // Setter
      {
        sNodeTypeName.Set("Set", sVariableName);
        m_VariableNodeTypeNames.PushBack(sNodeTypeName);

        auto& temp = out_templates.ExpandAndGetRef();
        temp.m_pType = pRegistry->GetVariableSetterType();
        temp.m_sTypeName = m_VariableNodeTypeNames.PeekBack();
        temp.m_sCategory = sVariables;
        temp.m_PropertyValues = m_PropertyValues.GetArrayPtr().GetSubArray(uiStart, 1);
      }

      // Getter
      {
        sNodeTypeName.Set("Get", sVariableName);
        m_VariableNodeTypeNames.PushBack(sNodeTypeName);

        auto& temp = out_templates.ExpandAndGetRef();
        temp.m_pType = pRegistry->GetVariableGetterType();
        temp.m_sTypeName = m_VariableNodeTypeNames.PeekBack();
        temp.m_sCategory = sVariables;
        temp.m_PropertyValues = m_PropertyValues.GetArrayPtr().GetSubArray(uiStart, 1);
      }
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
      DeductNodeTypeAndAllPinTypes(targetPin.GetParent());
      UpdateCoroutine(targetPin.GetParent(), connection);
    }
    break;

    case ezDocumentNodeManagerEvent::Type::BeforePinsDisonnected:
    {
      auto& connection = GetConnection(e.m_pObject);
      auto& targetPin = connection.GetTargetPin();
      DeductNodeTypeAndAllPinTypes(targetPin.GetParent(), &targetPin);
      UpdateCoroutine(targetPin.GetParent(), connection, false);
    }
    break;

    case ezDocumentNodeManagerEvent::Type::AfterNodeAdded:
    {
      DeductNodeTypeAndAllPinTypes(e.m_pObject);
    }
    break;

    case ezDocumentNodeManagerEvent::Type::BeforeNodeRemoved:
    {
      m_ObjectToDeductedType.Remove(e.m_pObject);
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
    DeductNodeTypeAndAllPinTypes(e.m_pObject);
  }
  else if (e.m_sProperty == "Name" || e.m_sProperty == "DefaultValue") // a variable's name or default value has changed, re-run type deduction
  {
    for (auto pObject : GetRootObject()->GetChildren())
    {
      if (IsNode(pObject) == false)
        continue;

      DeductNodeTypeAndAllPinTypes(pObject);
    }
  }
}

void ezVisualScriptNodeManager::RemoveDeductedPinType(const ezVisualScriptPin& pin)
{
  m_PinToDeductedType.Remove(&pin);
}

void ezVisualScriptNodeManager::DeductNodeTypeAndAllPinTypes(const ezDocumentObject* pObject, const ezPin* pDisconnectedPin /*= nullptr*/)
{
  auto pNodeDesc = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
  if (pNodeDesc == nullptr || pNodeDesc->NeedsTypeDeduction() == false)
    return;

  if (pDisconnectedPin != nullptr && static_cast<const ezVisualScriptPin*>(pDisconnectedPin)->NeedsTypeDeduction() == false)
    return;

  bool bNodeTypeChanged = false;
  {
    ezEnum<ezVisualScriptDataType> newDeductedType = pNodeDesc->m_DeductTypeFunc(pObject, static_cast<const ezVisualScriptPin*>(pDisconnectedPin));
    ezEnum<ezVisualScriptDataType> oldDeductedType = ezVisualScriptDataType::Invalid;
    m_ObjectToDeductedType.Insert(pObject, newDeductedType, &oldDeductedType);

    bNodeTypeChanged = (newDeductedType != oldDeductedType);
  }

  bool bAnyInputPinChanged = false;
  ezHybridArray<const ezVisualScriptPin*, 16> pins;
  GetInputDataPins(pObject, pins);
  for (auto pPin : pins)
  {
    if (auto pFunc = pPin->GetDeductTypeFunc())
    {
      ezEnum<ezVisualScriptDataType> newDeductedType = pFunc(*pPin);
      ezEnum<ezVisualScriptDataType> oldDeductedType = ezVisualScriptDataType::Invalid;
      m_PinToDeductedType.Insert(pPin, newDeductedType, &oldDeductedType);

      bAnyInputPinChanged |= (newDeductedType != oldDeductedType);
    }
  }

  bool bAnyOutputPinChanged = false;
  GetOutputDataPins(pObject, pins);
  for (auto pPin : pins)
  {
    if (auto pFunc = pPin->GetDeductTypeFunc())
    {
      ezEnum<ezVisualScriptDataType> newDeductedType = pFunc(*pPin);
      ezEnum<ezVisualScriptDataType> oldDeductedType = ezVisualScriptDataType::Invalid;
      m_PinToDeductedType.Insert(pPin, newDeductedType, &oldDeductedType);

      bAnyOutputPinChanged |= (newDeductedType != oldDeductedType);
    }
  }

  if (bNodeTypeChanged || bAnyInputPinChanged || bAnyOutputPinChanged)
  {
    m_NodeChangedEvent.Broadcast(pObject);
  }

  // propagate to connected nodes
  if (bAnyOutputPinChanged)
  {
    for (auto pPin : pins)
    {
      if (pPin->NeedsTypeDeduction() == false)
        continue;

      auto connections = GetConnections(*pPin);
      for (auto& connection : connections)
      {
        DeductNodeTypeAndAllPinTypes(connection->GetTargetPin().GetParent());
      }
    }
  }
}

void ezVisualScriptNodeManager::UpdateCoroutine(const ezDocumentObject* pTargetNode, const ezConnection& changedConnection, bool bIsAboutToDisconnect)
{
  auto vsPin = static_cast<const ezVisualScriptPin&>(changedConnection.GetTargetPin());
  if (vsPin.IsExecutionPin() == false)
    return;

  ezHybridArray<const ezDocumentObject*, 16> entryNodes;
  GetEntryNodes(pTargetNode, entryNodes);

  for (auto pEntryNode : entryNodes)
  {
    const bool bWasCoroutine = m_CoroutineObjects.Contains(pEntryNode);
    const bool bIsCoroutine = IsConnectedToCoroutine(pEntryNode, changedConnection, bIsAboutToDisconnect);

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

bool ezVisualScriptNodeManager::IsConnectedToCoroutine(const ezDocumentObject* pEntryNode, const ezConnection& changedConnection, bool bIsAboutToDisconnect) const
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
        if (bIsAboutToDisconnect && pConnection == &changedConnection)
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
