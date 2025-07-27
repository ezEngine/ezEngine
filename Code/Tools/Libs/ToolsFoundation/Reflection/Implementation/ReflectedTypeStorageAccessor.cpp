#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Types/Status.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>

////////////////////////////////////////////////////////////////////////
// ezReflectedTypeStorageAccessor public functions
////////////////////////////////////////////////////////////////////////

ezReflectedTypeStorageAccessor::ezReflectedTypeStorageAccessor(const ezRTTI* pRtti, ezDocumentObject* pOwner)
  : ezIReflectedTypeAccessor(pRtti, pOwner)
{
  const ezRTTI* pType = pRtti;
  EZ_ASSERT_DEV(pType != nullptr, "Trying to construct an ezReflectedTypeStorageAccessor for an invalid type!");
  m_pMapping = ezReflectedTypeStorageManager::AddStorageAccessor(this);
  EZ_ASSERT_DEV(m_pMapping != nullptr, "The type for this ezReflectedTypeStorageAccessor is unknown to the ezReflectedTypeStorageManager!");

  auto& indexTable = m_pMapping->m_PathToStorageInfoTable;
  const ezUInt32 uiProperties = indexTable.GetCount();
  // To prevent re-allocs due to new properties being added we reserve 20% more space.
  m_Data.Reserve(uiProperties + uiProperties / 20);
  m_Data.SetCount(uiProperties);

  // Fill data storage with default values for the given types.
  for (auto it = indexTable.GetIterator(); it.IsValid(); ++it)
  {
    const auto& storageInfo = it.Value();
    m_Data[storageInfo.m_uiIndex] = storageInfo.m_DefaultValue;
  }
}

ezReflectedTypeStorageAccessor::~ezReflectedTypeStorageAccessor()
{
  ezReflectedTypeStorageManager::RemoveStorageAccessor(this);
}

const ezVariant ezReflectedTypeStorageAccessor::GetValue(ezStringView sProperty, ezVariant index, ezStatus* pRes) const
{
  const ezAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
  if (pProp == nullptr)
  {
    if (pRes)
      *pRes = ezStatus(ezFmt("Property '{0}' not found in type '{1}'", sProperty, GetType()->GetTypeName()));
    return ezVariant();
  }

  if (pRes)
    *pRes = ezStatus(EZ_SUCCESS);
  const ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Member:
        if (index.IsValid())
        {
          if (pRes)
          {
            *pRes = ezStatus(ezFmt("Property '{0}' is a member property but an index of '{1}' is given", sProperty, index));
          }
          return ezVariant();
        }
        return m_Data[storageInfo->m_uiIndex];
      case ezPropertyCategory::Array:
      case ezPropertyCategory::Set:
      case ezPropertyCategory::Map:
      {
        return ezVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).GetValue(index, pRes);
      }
      break;
      default:
        break;
    }
  }
  return ezVariant();
}

bool ezReflectedTypeStorageAccessor::SetValue(ezStringView sProperty, const ezVariant& value, ezVariant index)
{
  const ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    const ezAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;
    EZ_ASSERT_DEV(pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>() || value.IsValid(), "");

    if (storageInfo->m_Type == ezVariantType::TypedObject && storageInfo->m_DefaultValue.GetReflectedType() != value.GetReflectedType())
    {
      // Typed objects must match exactly.
      return false;
    }

    const bool isValueType = ezReflectionUtils::IsValueType(pProp);
    const ezVariantType::Enum SpecVarType = pProp->GetFlags().IsSet(ezPropertyFlags::Pointer) || (pProp->GetFlags().IsSet(ezPropertyFlags::Class) && !isValueType) ? ezVariantType::Uuid : pProp->GetSpecificType()->GetVariantType();

    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Member:
      {
        if (index.IsValid())
          return false;

        if (value.IsA<ezString>() && pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
        {
          ezInt64 iValue;
          ezReflectionUtils::StringToEnumeration(pProp->GetSpecificType(), value.Get<ezString>(), iValue);
          m_Data[storageInfo->m_uiIndex] = ezVariant(iValue).ConvertTo(storageInfo->m_Type);
          return true;
        }
        else if (pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
        {
          m_Data[storageInfo->m_uiIndex] = value;
          return true;
        }
        else if (value.CanConvertTo(storageInfo->m_Type))
        {
          // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
          // that may have a different type now as someone reloaded the type information and replaced a type.
          m_Data[storageInfo->m_uiIndex] = value.ConvertTo(storageInfo->m_Type);
          return true;
        }
      }
      break;
      case ezPropertyCategory::Array:
      case ezPropertyCategory::Set:
      {
        if (index.IsNumber())
        {
          if (pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
            return ezVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).SetValue(value, index).Succeeded();
          else if (value.CanConvertTo(SpecVarType))
            // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
            // that may have a different type now as someone reloaded the type information and replaced a type.
            return ezVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).SetValue(value.ConvertTo(SpecVarType), index).Succeeded();
        }
      }
      break;
      case ezPropertyCategory::Map:
      {
        if (index.IsA<ezString>())
        {
          if (pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
            return ezVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).SetValue(value, index).Succeeded();
          else if (value.CanConvertTo(SpecVarType))
            // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
            // that may have a different type now as someone reloaded the type information and replaced a type.
            return ezVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).SetValue(value.ConvertTo(SpecVarType), index).Succeeded();
        }
      }
      break;
      default:
        break;
    }
  }
  return false;
}

ezInt32 ezReflectedTypeStorageAccessor::GetCount(ezStringView sProperty) const
{
  const ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    if (storageInfo->m_Type == ezVariant::Type::Invalid)
      return false;

    const ezAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return -1;

    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Array:
      case ezPropertyCategory::Set:
      case ezPropertyCategory::Map:
        return ezVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).GetCount();
      default:
        break;
    }
  }
  return -1;
}

bool ezReflectedTypeStorageAccessor::GetKeys(ezStringView sProperty, ezDynamicArray<ezVariant>& out_keys) const
{
  out_keys.Clear();

  const ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    if (storageInfo->m_Type == ezVariant::Type::Invalid)
      return false;

    const ezAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Array:
      case ezPropertyCategory::Set:
      case ezPropertyCategory::Map:
      {
        return ezVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).GetKeys(out_keys).Succeeded();
      }
      break;
      default:
        break;
    }
  }
  return false;
}
bool ezReflectedTypeStorageAccessor::InsertValue(ezStringView sProperty, ezVariant index, const ezVariant& value)
{
  const ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    if (storageInfo->m_Type == ezVariant::Type::Invalid)
      return false;

    const ezAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;

    if (storageInfo->m_Type == ezVariantType::TypedObject && storageInfo->m_DefaultValue.GetReflectedType() != value.GetReflectedType())
    {
      // Typed objects must match exactly.
      return false;
    }

    const bool isValueType = ezReflectionUtils::IsValueType(pProp);
    const ezVariantType::Enum SpecVarType = pProp->GetFlags().IsSet(ezPropertyFlags::Pointer) || (pProp->GetFlags().IsSet(ezPropertyFlags::Class) && !isValueType) ? ezVariantType::Uuid : pProp->GetSpecificType()->GetVariantType();

    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Array:
      case ezPropertyCategory::Set:
      {
        if (index.IsNumber())
        {
          if (pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
            return ezVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).InsertValue(index, value).Succeeded();
          else if (value.CanConvertTo(SpecVarType))
            // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
            // that may have a different type now as someone reloaded the type information and replaced a type.
            return ezVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).InsertValue(index, value.ConvertTo(SpecVarType)).Succeeded();
        }
      }
      break;
      case ezPropertyCategory::Map:
      {
        if (index.IsA<ezString>())
        {
          if (pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
            return ezVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).InsertValue(index, value).Succeeded();
          else if (value.CanConvertTo(SpecVarType))
            // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
            // that may have a different type now as someone reloaded the type information and replaced a type.
            return ezVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).InsertValue(index, value.ConvertTo(SpecVarType)).Succeeded();
        }
      }
      break;
      default:
        break;
    }
  }
  return false;
}

bool ezReflectedTypeStorageAccessor::RemoveValue(ezStringView sProperty, ezVariant index)
{
  const ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    if (storageInfo->m_Type == ezVariant::Type::Invalid)
      return false;

    const ezAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Array:
      case ezPropertyCategory::Set:
      case ezPropertyCategory::Map:
      {
        return ezVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).RemoveValue(index).Succeeded();
      }
      break;
      default:
        break;
    }
  }
  return false;
}

bool ezReflectedTypeStorageAccessor::MoveValue(ezStringView sProperty, ezVariant oldIndex, ezVariant newIndex)
{
  const ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    if (storageInfo->m_Type == ezVariant::Type::Invalid)
      return false;

    const ezAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Array:
      case ezPropertyCategory::Set:
      case ezPropertyCategory::Map:
      {
        return ezVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).MoveValue(oldIndex, newIndex).Succeeded();
      }
      break;
      default:
        break;
    }
  }
  return false;
}

ezVariant ezReflectedTypeStorageAccessor::GetPropertyChildIndex(ezStringView sProperty, const ezVariant& value) const
{
  const ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    //    if (storageInfo->m_Type == ezVariant::Type::Invalid)
    //      return ezVariant();

    const ezAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return ezVariant();

    const bool isValueType = ezReflectionUtils::IsValueType(pProp);
    const ezVariantType::Enum SpecVarType = pProp->GetFlags().IsSet(ezPropertyFlags::Pointer) || (pProp->GetFlags().IsSet(ezPropertyFlags::Class) && !isValueType) ? ezVariantType::Uuid : pProp->GetSpecificType()->GetVariantType();

    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Array:
      case ezPropertyCategory::Set:
      {
        if (value.CanConvertTo(SpecVarType))
        {
          const ezVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<ezVariantArray>();
          for (ezUInt32 i = 0; i < values.GetCount(); i++)
          {
            if (values[i] == value)
              return ezVariant((ezUInt32)i);
          }
        }
      }
      break;
      case ezPropertyCategory::Map:
      {
        if (value.CanConvertTo(SpecVarType))
        {
          const ezVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<ezVariantDictionary>();
          for (auto it = values.GetIterator(); it.IsValid(); ++it)
          {
            if (it.Value() == value)
              return ezVariant(it.Key());
          }
        }
      }
      break;
      default:
        break;
    }
  }
  return ezVariant();
}
