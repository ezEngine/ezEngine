#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/AttributeDefaultStateProvider.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

static ezSharedPtr<ezDefaultStateProvider> g_pAttributeDefaultStateProvider = EZ_DEFAULT_NEW(ezAttributeDefaultStateProvider);
ezSharedPtr<ezDefaultStateProvider> ezAttributeDefaultStateProvider::CreateProvider(ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp)
{
  // One global instance handles all. No need to create a new instance per request as no state need to be tracked.
  return g_pAttributeDefaultStateProvider;
}

ezInt32 ezAttributeDefaultStateProvider::GetRootDepth() const
{
  return -1;
}

ezColorGammaUB ezAttributeDefaultStateProvider::GetBackgroundColor() const
{
  // Set alpha to 0 -> color will be ignored.
  return ezColorGammaUB(0, 0, 0, 0);
}

ezVariant ezAttributeDefaultStateProvider::GetDefaultValue(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index)
{
  if (!pProp->GetFlags().IsSet(ezPropertyFlags::Pointer) && pProp->GetFlags().IsSet(ezPropertyFlags::Class) && pProp->GetCategory() == ezPropertyCategory::Member && !ezReflectionUtils::IsValueType(pProp))
  {
    // An embedded class that is not a value type can never change its value.
    ezVariant value;
    pAccessor->GetValue(pObject, pProp, value).LogFailure();
    return value;
  }
  return ezReflectionUtils::GetDefaultValue(pProp, index);
}

ezStatus ezAttributeDefaultStateProvider::CreateRevertContainerDiff(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDeque<ezAbstractGraphDiffOperation>& out_diff)
{
  auto RemoveObject = [&](const ezUuid& object)
  {
    auto& op = out_diff.ExpandAndGetRef();
    op.m_Node = object;
    op.m_Operation = ezAbstractGraphDiffOperation::Op::NodeRemoved;
    op.m_uiTypeVersion = 0;
    op.m_sProperty = pAccessor->GetObject(object)->GetType()->GetTypeName();
  };

  auto SetProperty = [&](const ezVariant& newValue)
  {
    auto& op = out_diff.ExpandAndGetRef();
    op.m_Node = pObject->GetGuid();
    op.m_Operation = ezAbstractGraphDiffOperation::Op::PropertyChanged;
    op.m_uiTypeVersion = 0;
    op.m_sProperty = pProp->GetPropertyName();
    op.m_Value = newValue;
  };

  ezVariant currentValue;
  pAccessor->GetValue(pObject, pProp, currentValue).LogFailure();
  switch (pProp->GetCategory())
  {
    case ezPropertyCategory::Member:
    {
      const auto& objectGuid = currentValue.Get<ezUuid>();
      if (objectGuid.IsValid())
      {
        RemoveObject(objectGuid);
        SetProperty(ezUuid());
      }
    }
    break;
    case ezPropertyCategory::Array:
    case ezPropertyCategory::Set:
    {
      const auto& currentArray = currentValue.Get<ezVariantArray>();
      for (ezInt32 i = (ezInt32)currentArray.GetCount() - 1; i >= 0; i--)
      {
        const auto& objectGuid = currentArray[i].Get<ezUuid>();
        if (objectGuid.IsValid())
        {
          RemoveObject(objectGuid);
        }
      }
      SetProperty(ezVariantArray());
    }
    break;
    case ezPropertyCategory::Map:
    {
      const auto& currentArray = currentValue.Get<ezVariantDictionary>();
      for (auto val : currentArray)
      {
        const auto& objectGuid = val.Value().Get<ezUuid>();
        if (objectGuid.IsValid())
        {
          RemoveObject(objectGuid);
        }
      }
      SetProperty(ezVariantDictionary());
    }
    break;
    default:
      EZ_REPORT_FAILURE("Unreachable code");
      break;
  }
  return ezStatus(EZ_SUCCESS);
}
