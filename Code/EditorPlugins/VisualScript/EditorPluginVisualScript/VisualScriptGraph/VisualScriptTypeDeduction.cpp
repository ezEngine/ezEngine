#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraph.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptTypeDeduction.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>

// static
ezVisualScriptDataType::Enum ezVisualScriptTypeDeduction::DeductFromNodeDataType(const ezVisualScriptPin& pin)
{
  auto pObject = pin.GetParent();
  auto pManager = static_cast<const ezVisualScriptNodeManager*>(pObject->GetDocumentObjectManager());

  return pManager->GetDeductedType(pObject);
}

// static
ezVisualScriptDataType::Enum ezVisualScriptTypeDeduction::DeductFromTypeProperty(const ezVisualScriptPin& pin)
{
  if (auto pType = GetReflectedType(pin.GetParent()))
  {
    return ezVisualScriptDataType::FromRtti(pType);
  }

  return ezVisualScriptDataType::Invalid;
}

// static
ezVisualScriptDataType::Enum ezVisualScriptTypeDeduction::DeductFromExpressionInput(const ezVisualScriptPin& pin)
{
  return DeductFromExpressionVariable(pin, "Inputs");
}

// static
ezVisualScriptDataType::Enum ezVisualScriptTypeDeduction::DeductFromExpressionOutput(const ezVisualScriptPin& pin)
{
  return DeductFromExpressionVariable(pin, "Outputs");
}

// static
ezVisualScriptDataType::Enum ezVisualScriptTypeDeduction::DeductFromAllInputPins(const ezDocumentObject* pObject, const ezVisualScriptPin* pDisconnectedPin)
{
  auto pManager = static_cast<const ezVisualScriptNodeManager*>(pObject->GetDocumentObjectManager());

  ezVisualScriptDataType::Enum deductedType = ezVisualScriptDataType::Invalid;

  ezHybridArray<const ezVisualScriptPin*, 16> pins;
  pManager->GetInputDataPins(pObject, pins);
  for (auto pPin : pins)
  {
    if (pPin->GetScriptDataType() != ezVisualScriptDataType::Any)
      continue;

    // the pin is about to be disconnected so we ignore it here
    if (pPin == pDisconnectedPin)
      continue;

    ezVisualScriptDataType::Enum pinDataType = ezVisualScriptDataType::Invalid;
    auto connections = pManager->GetConnections(*pPin);
    if (connections.IsEmpty() == false)
    {
      pinDataType = static_cast<const ezVisualScriptPin&>(connections[0]->GetSourcePin()).GetResolvedScriptDataType();
    }
    else
    {
      ezVariant var = pObject->GetTypeAccessor().GetValue(pPin->GetName());
      pinDataType = ezVisualScriptDataType::FromVariantType(var.GetType());
    }

    deductedType = ezMath::Max(deductedType, pinDataType);
  }

  return deductedType;
}

// static
ezVisualScriptDataType::Enum ezVisualScriptTypeDeduction::DeductFromVariableNameProperty(const ezDocumentObject* pObject, const ezVisualScriptPin* pDisconnectedPin)
{
  auto nameVar = pObject->GetTypeAccessor().GetValue("Name");
  if (nameVar.IsA<ezString>())
  {
    auto pManager = static_cast<const ezVisualScriptNodeManager*>(pObject->GetDocumentObjectManager());
    return pManager->GetVariableType(ezTempHashedString(nameVar.Get<ezString>()));
  }

  return ezVisualScriptDataType::Invalid;
}

// static
ezVisualScriptDataType::Enum ezVisualScriptTypeDeduction::DeductFromScriptDataTypeProperty(const ezDocumentObject* pObject, const ezVisualScriptPin* pDisconnectedPin)
{
  auto typeVar = pObject->GetTypeAccessor().GetValue("Type");
  if (typeVar.IsA<ezInt64>())
  {
    return static_cast<ezVisualScriptDataType::Enum>(typeVar.Get<ezInt64>());
  }

  return ezVisualScriptDataType::Invalid;
}

// static
ezVisualScriptDataType::Enum ezVisualScriptTypeDeduction::DeductFromPropertyProperty(const ezDocumentObject* pObject, const ezVisualScriptPin* pDisconnectedPin)
{
  if (auto pProperty = GetReflectedProperty(pObject))
  {
    return ezVisualScriptDataType::FromRtti(pProperty->GetSpecificType());
  }

  return ezVisualScriptDataType::Invalid;
}

// static
ezVisualScriptDataType::Enum ezVisualScriptTypeDeduction::DeductDummy(const ezDocumentObject* pObject, const ezVisualScriptPin* pDisconnectedPin)
{
  // nothing to do here
  return ezVisualScriptDataType::Float;
}

// static
const ezRTTI* ezVisualScriptTypeDeduction::GetReflectedType(const ezDocumentObject* pObject)
{
  auto typeVar = pObject->GetTypeAccessor().GetValue("Type");
  if (typeVar.IsA<ezString>() == false)
    return nullptr;

  const ezString& sTypeName = typeVar.Get<ezString>();
  if (sTypeName.IsEmpty())
    return nullptr;

  const ezRTTI* pType = ezRTTI::FindTypeByName(sTypeName);
  if (pType == nullptr && sTypeName.StartsWith("ez") == false)
  {
    ezStringBuilder sFullTypeName;
    sFullTypeName.Set("ez", typeVar.Get<ezString>());
    pType = ezRTTI::FindTypeByName(sFullTypeName);
  }

  if (pType == nullptr)
  {
    ezLog::Error("'{}' is not a valid type", typeVar.Get<ezString>());
    return nullptr;
  }

  return pType;
}

// static
const ezAbstractProperty* ezVisualScriptTypeDeduction::GetReflectedProperty(const ezDocumentObject* pObject)
{
  auto pType = GetReflectedType(pObject);
  if (pType == nullptr)
    return nullptr;

  auto propertyVar = pObject->GetTypeAccessor().GetValue("Property");
  if (propertyVar.IsA<ezString>() == false)
    return nullptr;

  const ezString& sPropertyName = propertyVar.Get<ezString>();
  if (sPropertyName.IsEmpty())
    return nullptr;

  const ezAbstractProperty* pProperty = pType->FindPropertyByName(propertyVar.Get<ezString>());

  if (pProperty == nullptr)
  {
    ezLog::Error("'{}' is not a valid property of '{}'", propertyVar.Get<ezString>(), pType->GetTypeName());
    return nullptr;
  }

  return pProperty;
}

// static
ezVisualScriptDataType::Enum ezVisualScriptTypeDeduction::DeductFromExpressionVariable(const ezVisualScriptPin& pin, ezStringView sPropertyName)
{
  auto pObject = pin.GetParent();

  ezVariant varList = pObject->GetTypeAccessor().GetValue(sPropertyName);
  if (varList.IsA<ezVariantArray>() == false)
    return ezVisualScriptDataType::Invalid;

  ezVariant var = varList[pin.GetDataPinIndex()];
  if (var.IsA<ezUuid>() == false)
    return ezVisualScriptDataType::Invalid;

  const ezDocumentObject* pVarObject = pObject->GetDocumentObjectManager()->GetObject(var.Get<ezUuid>());
  if (pVarObject == nullptr)
    return ezVisualScriptDataType::Invalid;

  ezVariant typeVar = pVarObject->GetTypeAccessor().GetValue("Type");
  if (typeVar.IsA<ezInt64>() == false)
    return ezVisualScriptDataType::Invalid;

  auto expressionDataType = static_cast<ezVisualScriptExpressionDataType::Enum>(typeVar.Get<ezInt64>());
  return ezVisualScriptExpressionDataType::GetVisualScriptDataType(expressionDataType);
}
