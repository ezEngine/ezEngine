#include <PCH.h>

#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>


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
    const auto storageInfo = it.Value();
    m_Data[storageInfo.m_uiIndex] = storageInfo.m_DefaultValue;
  }
}

ezReflectedTypeStorageAccessor::~ezReflectedTypeStorageAccessor()
{
  ezReflectedTypeStorageManager::RemoveStorageAccessor(this);
}

const ezVariant ezReflectedTypeStorageAccessor::GetValue(const char* szProperty, ezVariant index, ezStatus* pRes) const
{
  const ezAbstractProperty* pProp = GetType()->FindPropertyByName(szProperty);
  if (pProp == nullptr)
  {
    if (pRes)
      *pRes = ezStatus(ezFmt("Property '{0}' not found in type '{1}'", szProperty, GetType()->GetTypeName()));
    return ezVariant();
  }

  if (pRes)
    *pRes = ezStatus(EZ_SUCCESS);
  const ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(szProperty, storageInfo))
  {
    if (storageInfo->m_Type == ezVariant::Type::Invalid)
    {
      if (pRes)
        *pRes = ezStatus(ezFmt("No storage type defined for property '{0}'", szProperty));
      return ezVariant();
    }

    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Member:
        return m_Data[storageInfo->m_uiIndex];
      case ezPropertyCategory::Array:
      case ezPropertyCategory::Set:
      {
        const ezVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<ezVariantArray>();
        if (index.CanConvertTo<ezUInt32>())
        {
          ezUInt32 uiIndex = index.ConvertTo<ezUInt32>();
          if (uiIndex < values.GetCount())
          {
            return values[uiIndex];
          }
        }
        if (pRes)
          *pRes = ezStatus(ezFmt("Index '{0}' for property '{1}' is invalid or out of bounds.", index, szProperty));
      }
      break;
      case ezPropertyCategory::Map:
      {
        const ezVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<ezVariantDictionary>();
        if (index.IsA<ezString>())
        {
          const ezString& sIndex = index.Get<ezString>();
          if (const ezVariant* pValue = values.GetValue(sIndex))
          {
            return *pValue;
          }
        }
        if (pRes)
          *pRes = ezStatus(ezFmt("Index '{0}' for property '{1}' is invalid or out of bounds.", index, szProperty));
      }
      break;
      default:
        break;
    }
  }
  return ezVariant();
}

bool ezReflectedTypeStorageAccessor::SetValue(const char* szProperty, const ezVariant& value, ezVariant index)
{
  const ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(szProperty, storageInfo))
  {
    if (storageInfo->m_Type == ezVariant::Type::Invalid)
      return false;

    const ezAbstractProperty* pProp = GetType()->FindPropertyByName(szProperty);
    if (pProp == nullptr)
      return false;
    EZ_ASSERT_DEV(pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>() || value.IsValid(), "");

    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Member:
      {
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
        const ezVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<ezVariantArray>();
        if (index.CanConvertTo<ezUInt32>())
        {
          ezUInt32 uiIndex = index.ConvertTo<ezUInt32>();
          if (uiIndex < values.GetCount())
          {
            ezVariantArray changedValues = values;
            if (pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
            {
              changedValues[uiIndex] = value;
              m_Data[storageInfo->m_uiIndex] = changedValues;
              return true;
            }
            else
            {
              const auto SpecVarType =
                  pProp->GetFlags().IsSet(ezPropertyFlags::StandardType) ? pProp->GetSpecificType()->GetVariantType() : ezVariantType::Uuid;
              if (pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
              {
                m_Data[storageInfo->m_uiIndex] = value;
                return true;
              }
              else if (value.CanConvertTo(SpecVarType))
              {
                // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
                // that may have a different type now as someone reloaded the type information and replaced a type.
                changedValues[uiIndex] = value.ConvertTo(SpecVarType);
                m_Data[storageInfo->m_uiIndex] = changedValues;
                return true;
              }
            }
          }
        }
      }
      break;
      case ezPropertyCategory::Map:
      {
        const ezVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<ezVariantDictionary>();
        if (index.IsA<ezString>() && values.Contains(index.Get<ezString>()))
        {
          const ezString& sIndex = index.Get<ezString>();
          ezVariantDictionary changedValues = values;
          if (pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
          {
            changedValues[sIndex] = value;
            m_Data[storageInfo->m_uiIndex] = changedValues;
            return true;
          }
          else
          {
            const auto SpecVarType =
                pProp->GetFlags().IsSet(ezPropertyFlags::StandardType) ? pProp->GetSpecificType()->GetVariantType() : ezVariantType::Uuid;
            if (value.CanConvertTo(SpecVarType))
            {
              // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
              // that may have a different type now as someone reloaded the type information and replaced a type.
              changedValues[sIndex] = value.ConvertTo(SpecVarType);
              m_Data[storageInfo->m_uiIndex] = changedValues;
              return true;
            }
          }
        }
      }
      break;
      default:
        break;
    }
  }
  return false;
}

ezInt32 ezReflectedTypeStorageAccessor::GetCount(const char* szProperty) const
{
  const ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(szProperty, storageInfo))
  {
    if (storageInfo->m_Type == ezVariant::Type::Invalid)
      return false;

    const ezAbstractProperty* pProp = GetType()->FindPropertyByName(szProperty);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Array:
      case ezPropertyCategory::Set:
      {
        const ezVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<ezVariantArray>();
        return values.GetCount();
      }
      case ezPropertyCategory::Map:
      {
        const ezVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<ezVariantDictionary>();
        return values.GetCount();
      }
      default:
        break;
    }
  }
  return -1;
}

bool ezReflectedTypeStorageAccessor::GetKeys(const char* szProperty, ezHybridArray<ezVariant, 16>& out_keys) const
{
  out_keys.Clear();

  const ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(szProperty, storageInfo))
  {
    if (storageInfo->m_Type == ezVariant::Type::Invalid)
      return false;

    const ezAbstractProperty* pProp = GetType()->FindPropertyByName(szProperty);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Array:
      case ezPropertyCategory::Set:
      {
        const ezVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<ezVariantArray>();
        out_keys.Reserve(values.GetCount());
        for (ezUInt32 i = 0; i < values.GetCount(); ++i)
        {
          out_keys.PushBack(i);
        }
        return true;
      }
      break;
      case ezPropertyCategory::Map:
      {
        const ezVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<ezVariantDictionary>();
        out_keys.Reserve(values.GetCount());
        for (auto it = values.GetIterator(); it.IsValid(); ++it)
        {
          out_keys.PushBack(it.Key());
        }
        return true;
      }
      break;

      default:
        break;
    }
  }
  return false;
}
bool ezReflectedTypeStorageAccessor::InsertValue(const char* szProperty, ezVariant index, const ezVariant& value)
{
  const ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(szProperty, storageInfo))
  {
    if (storageInfo->m_Type == ezVariant::Type::Invalid)
      return false;

    const ezAbstractProperty* pProp = GetType()->FindPropertyByName(szProperty);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Array:
      case ezPropertyCategory::Set:
      {
        const ezVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<ezVariantArray>();
        if (index.CanConvertTo<ezUInt32>())
        {
          ezUInt32 uiIndex = index.ConvertTo<ezUInt32>();
          if (uiIndex <= values.GetCount())
          {
            ezVariantArray changedValues = values;
            const auto SpecVarType =
                pProp->GetFlags().IsSet(ezPropertyFlags::StandardType) ? pProp->GetSpecificType()->GetVariantType() : ezVariantType::Uuid;
            if (pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
            {
              changedValues.Insert(value, uiIndex);
              m_Data[storageInfo->m_uiIndex] = changedValues;
              return true;
            }
            else if (value.CanConvertTo(SpecVarType))
            {
              // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
              // that may have a different type now as someone reloaded the type information and replaced a type.
              changedValues.Insert(value.ConvertTo(SpecVarType), uiIndex);
              m_Data[storageInfo->m_uiIndex] = changedValues;
              return true;
            }
          }
        }
      }
      break;
      case ezPropertyCategory::Map:
      {
        const ezVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<ezVariantDictionary>();
        if (index.IsA<ezString>() && !values.Contains(index.Get<ezString>()))
        {
          const ezString& sIndex = index.Get<ezString>();
          ezVariantDictionary changedValues = values;
          const auto SpecVarType =
              pProp->GetFlags().IsSet(ezPropertyFlags::StandardType) ? pProp->GetSpecificType()->GetVariantType() : ezVariantType::Uuid;
          if (pProp->GetSpecificType() == ezGetStaticRTTI<ezVariant>())
          {
            changedValues.Insert(sIndex, value);
            m_Data[storageInfo->m_uiIndex] = changedValues;
            return true;
          }
          else if (value.CanConvertTo(SpecVarType))
          {
            // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
            // that may have a different type now as someone reloaded the type information and replaced a type.
            changedValues.Insert(sIndex, value.ConvertTo(SpecVarType));
            m_Data[storageInfo->m_uiIndex] = changedValues;
            return true;
          }
        }
      }
      break;
      default:
        break;
    }
  }
  return false;
}

bool ezReflectedTypeStorageAccessor::RemoveValue(const char* szProperty, ezVariant index)
{
  const ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(szProperty, storageInfo))
  {
    if (storageInfo->m_Type == ezVariant::Type::Invalid)
      return false;

    const ezAbstractProperty* pProp = GetType()->FindPropertyByName(szProperty);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Array:
      case ezPropertyCategory::Set:
      {
        const ezVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<ezVariantArray>();
        if (index.CanConvertTo<ezUInt32>())
        {
          ezUInt32 uiIndex = index.ConvertTo<ezUInt32>();
          if (uiIndex < values.GetCount())
          {
            ezVariantArray changedValues = values;
            changedValues.RemoveAtAndCopy(uiIndex);
            m_Data[storageInfo->m_uiIndex] = changedValues;
            return true;
          }
        }
      }
      break;
      case ezPropertyCategory::Map:
      {
        const ezVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<ezVariantDictionary>();
        if (index.IsA<ezString>() && values.Contains(index.Get<ezString>()))
        {
          const ezString& sIndex = index.Get<ezString>();
          ezVariantDictionary changedValues = values;
          changedValues.Remove(sIndex);
          m_Data[storageInfo->m_uiIndex] = changedValues;
          return true;
        }
      }
      break;
      default:
        break;
    }
  }
  return false;
}

bool ezReflectedTypeStorageAccessor::MoveValue(const char* szProperty, ezVariant oldIndex, ezVariant newIndex)
{
  const ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(szProperty, storageInfo))
  {
    if (storageInfo->m_Type == ezVariant::Type::Invalid)
      return false;

    const ezAbstractProperty* pProp = GetType()->FindPropertyByName(szProperty);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Array:
      case ezPropertyCategory::Set:
      {
        const ezVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<ezVariantArray>();
        if (oldIndex.CanConvertTo<ezUInt32>() && newIndex.CanConvertTo<ezUInt32>())
        {
          ezUInt32 uiOldIndex = oldIndex.ConvertTo<ezUInt32>();
          ezUInt32 uiNewIndex = newIndex.ConvertTo<ezUInt32>();
          if (uiOldIndex < values.GetCount() && uiNewIndex <= values.GetCount())
          {
            ezVariantArray changedValues = values;
            ezVariant value = changedValues[uiOldIndex];
            changedValues.RemoveAtAndCopy(uiOldIndex);
            if (uiNewIndex > uiOldIndex)
            {
              uiNewIndex -= 1;
            }
            changedValues.Insert(value, uiNewIndex);

            m_Data[storageInfo->m_uiIndex] = changedValues;
            return true;
          }
        }
      }
      break;
      case ezPropertyCategory::Map:
      {
        const ezVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<ezVariantDictionary>();
        if (oldIndex.IsA<ezString>() && values.Contains(oldIndex.Get<ezString>()) && newIndex.IsA<ezString>())
        {
          const ezString& sIndex = oldIndex.Get<ezString>();
          ezVariantDictionary changedValues = values;
          changedValues.Insert(newIndex.Get<ezString>(), changedValues[sIndex]);
          changedValues.Remove(sIndex);
          m_Data[storageInfo->m_uiIndex] = changedValues;
          return true;
        }
      }
      default:
        break;
    }
  }
  return false;
}

ezVariant ezReflectedTypeStorageAccessor::GetPropertyChildIndex(const char* szProperty, const ezVariant& value) const
{
  const ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(szProperty, storageInfo))
  {
    if (storageInfo->m_Type == ezVariant::Type::Invalid)
      return ezVariant();

    const ezAbstractProperty* pProp = GetType()->FindPropertyByName(szProperty);
    if (pProp == nullptr)
      return ezVariant();

    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Array:
      case ezPropertyCategory::Set:
      {
        const auto SpecVarType =
            pProp->GetFlags().IsSet(ezPropertyFlags::StandardType) ? pProp->GetSpecificType()->GetVariantType() : ezVariantType::Uuid;
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
        const auto SpecVarType =
            pProp->GetFlags().IsSet(ezPropertyFlags::StandardType) ? pProp->GetSpecificType()->GetVariantType() : ezVariantType::Uuid;
        if (value.CanConvertTo(SpecVarType))
        {
          const ezVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<ezVariantDictionary>();
          for (auto it = values.GetIterator(); it.IsValid(); ++it)
          {
            if (it.Value() == value)
              return it.Key();
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
