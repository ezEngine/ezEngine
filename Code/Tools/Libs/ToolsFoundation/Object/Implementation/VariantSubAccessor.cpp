#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/VariantSubAccessor.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVariantSubAccessor, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVariantSubAccessor::ezVariantSubAccessor(ezObjectAccessorBase* pSource, const ezAbstractProperty* pProp)
  : ezObjectProxyAccessor(pSource)
  , m_pProp(pProp)
{
}

void ezVariantSubAccessor::SetSubItems(const ezMap<const ezDocumentObject*, ezVariant>& subItemMap)
{
  m_SubItemMap = subItemMap;
}

ezInt32 ezVariantSubAccessor::GetDepth() const
{
  if (auto variantSubAccessor = ezDynamicCast<ezVariantSubAccessor*>(GetSourceAccessor()))
  {
    return variantSubAccessor->GetDepth() + 1;
  }
  return 1;
}

ezResult ezVariantSubAccessor::GetPath(const ezDocumentObject* pObject, ezDynamicArray<ezVariant>& out_path) const
{
  out_path.Clear();
  if (auto variantSubAccessor = ezDynamicCast<ezVariantSubAccessor*>(GetSourceAccessor()))
  {
    EZ_SUCCEED_OR_RETURN(variantSubAccessor->GetPath(pObject, out_path));
  }
  ezVariant subItem;
  if (!m_SubItemMap.TryGetValue(pObject, subItem))
    return EZ_FAILURE;

  out_path.PushBack(subItem);
  return EZ_SUCCESS;
}

ezStatus ezVariantSubAccessor::GetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant& out_value, ezVariant index)
{
  EZ_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, out_value));

  ezStatus result(EZ_SUCCESS);
  out_value = ezVariantStorageAccessor(pProp->GetPropertyName(), out_value).GetValue(index, &result);
  return result;
}

ezStatus ezVariantSubAccessor::SetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index)
{
  return SetSubValue(pObject, pProp, [&](ezVariant& subValue) -> ezStatus
    { return ezVariantStorageAccessor(pProp->GetPropertyName(), subValue).SetValue(newValue, index); });
}

ezStatus ezVariantSubAccessor::InsertValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index)
{
  return SetSubValue(pObject, pProp, [&](ezVariant& subValue) -> ezStatus
    { return ezVariantStorageAccessor(pProp->GetPropertyName(), subValue).InsertValue(index, newValue); });
}

ezStatus ezVariantSubAccessor::RemoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index)
{
  return SetSubValue(pObject, pProp, [&](ezVariant& subValue) -> ezStatus
    { return ezVariantStorageAccessor(pProp->GetPropertyName(), subValue).RemoveValue(index); });
}

ezStatus ezVariantSubAccessor::MoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& oldIndex, const ezVariant& newIndex)
{
  return SetSubValue(pObject, pProp, [&](ezVariant& subValue) -> ezStatus
    { return ezVariantStorageAccessor(pProp->GetPropertyName(), subValue).MoveValue(oldIndex, newIndex); });
}

ezStatus ezVariantSubAccessor::GetCount(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezInt32& out_iCount)
{
  ezVariant subValue;
  EZ_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, subValue));
  out_iCount = ezVariantStorageAccessor(pProp->GetPropertyName(), subValue).GetCount();
  return EZ_SUCCESS;
}

ezStatus ezVariantSubAccessor::GetKeys(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDynamicArray<ezVariant>& out_keys)
{
  ezVariant subValue;
  EZ_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, subValue));
  return ezVariantStorageAccessor(pProp->GetPropertyName(), subValue).GetKeys(out_keys);
}

ezStatus ezVariantSubAccessor::GetValues(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDynamicArray<ezVariant>& out_values)
{
  ezVariant subValue;
  EZ_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, subValue));
  ezHybridArray<ezVariant, 16> keys;
  ezVariantStorageAccessor accessor(pProp->GetPropertyName(), subValue);
  EZ_SUCCEED_OR_RETURN(accessor.GetKeys(keys));
  out_values.Clear();
  out_values.Reserve(keys.GetCount());
  for (const ezVariant& key : keys)
  {
    out_values.PushBack(accessor.GetValue(key));
  }
  return EZ_SUCCESS;
}

ezStatus ezVariantSubAccessor::GetSubValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant& out_value)
{
  ezStatus result = ezObjectProxyAccessor::GetValue(pObject, pProp, out_value);
  if (result.Failed())
    return result;

  ezVariant subItem;
  if (!m_SubItemMap.TryGetValue(pObject, subItem))
    return ezStatus(ezFmt("Sub-item '{0}' not found in variant property '{1}'", subItem, pProp->GetPropertyName()));

  out_value = ezVariantStorageAccessor(pProp->GetPropertyName(), out_value).GetValue(subItem, &result);
  return result;
}

ezStatus ezVariantSubAccessor::SetSubValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezDelegate<ezStatus(ezVariant&)>& func)
{
  EZ_ASSERT_DEBUG(m_pProp == pProp, "ezVariantSubAccessor should only be used to access a single variant property");
  ezVariant subItem;
  if (!m_SubItemMap.TryGetValue(pObject, subItem))
    return ezStatus(ezFmt("Sub-item '{0}' not found in variant property '{1}'", subItem, pProp->GetPropertyName()));

  ezVariant currentValue;
  EZ_SUCCEED_OR_RETURN(ezObjectProxyAccessor::GetValue(pObject, pProp, currentValue, subItem));
  EZ_SUCCEED_OR_RETURN(func(currentValue));
  return ezObjectProxyAccessor::SetValue(pObject, pProp, currentValue, subItem);
}
