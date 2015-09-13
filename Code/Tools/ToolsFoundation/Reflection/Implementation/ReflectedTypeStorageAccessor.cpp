#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>


////////////////////////////////////////////////////////////////////////
// ezReflectedTypeStorageAccessor public functions
////////////////////////////////////////////////////////////////////////

ezReflectedTypeStorageAccessor::ezReflectedTypeStorageAccessor(const ezRTTI* pRtti, ezDocumentObjectBase* pOwner) : ezIReflectedTypeAccessor(pRtti, pOwner)
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

const ezVariant ezReflectedTypeStorageAccessor::GetValue(const ezPropertyPath& path, ezVariant index) const
{
  ezStringBuilder sPathString = path.GetPathString();

  ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sPathString, storageInfo))
  {
    if (storageInfo->m_Type == ezVariant::Type::Invalid)
      return ezVariant();

    const ezAbstractProperty* pProp = ezToolsReflectionUtils::GetPropertyByPath(GetType(), path);
    if (pProp == nullptr)
      return ezVariant();

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
      }
      break;
    }
  }
  return ezVariant();
}

bool ezReflectedTypeStorageAccessor::SetValue(const ezPropertyPath& path, const ezVariant& value, ezVariant index)
{
  ezStringBuilder sPathString = path.GetPathString();

  ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sPathString, storageInfo))
  {
    if (storageInfo->m_Type == ezVariant::Type::Invalid)
      return false;

    const ezAbstractProperty* pProp = ezToolsReflectionUtils::GetPropertyByPath(GetType(), path);
    if (pProp == nullptr)
      return false;

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
            const auto SpecVarType = pProp->GetFlags().IsSet(ezPropertyFlags::StandardType) ? pProp->GetSpecificType()->GetVariantType() : ezVariantType::Uuid;

            if (value.CanConvertTo(SpecVarType))
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
      break;
    }
  }
  return false;
}

ezInt32 ezReflectedTypeStorageAccessor::GetCount(const ezPropertyPath& path) const
{
  ezStringBuilder sPathString = path.GetPathString();
  ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sPathString, storageInfo))
  {
    if (storageInfo->m_Type == ezVariant::Type::Invalid)
      return false;

    const ezAbstractProperty* pProp = ezToolsReflectionUtils::GetPropertyByPath(GetType(), path);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
    case ezPropertyCategory::Array:
    case ezPropertyCategory::Set:
      const ezVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<ezVariantArray>();
      return values.GetCount();
    }
  }
  return -1;
}

bool ezReflectedTypeStorageAccessor::GetKeys(const ezPropertyPath& path, ezHybridArray<ezVariant, 16>& out_keys) const
{
  out_keys.Clear();
  ezStringBuilder sPathString = path.GetPathString();

  ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sPathString, storageInfo))
  {
    if (storageInfo->m_Type == ezVariant::Type::Invalid)
      return false;

    const ezAbstractProperty* pProp = ezToolsReflectionUtils::GetPropertyByPath(GetType(), path);
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
    }
  }
  return false;
}
bool ezReflectedTypeStorageAccessor::InsertValue(const ezPropertyPath& path, ezVariant index, const ezVariant& value)
{
  ezStringBuilder sPathString = path.GetPathString();

  ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sPathString, storageInfo))
  {
    if (storageInfo->m_Type == ezVariant::Type::Invalid)
      return false;

    const ezAbstractProperty* pProp = ezToolsReflectionUtils::GetPropertyByPath(GetType(), path);
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
            const auto SpecVarType = pProp->GetFlags().IsSet(ezPropertyFlags::StandardType) ? pProp->GetSpecificType()->GetVariantType() : ezVariantType::Uuid;

            if (value.CanConvertTo(SpecVarType))
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
    }
  }
  return false;
}

bool ezReflectedTypeStorageAccessor::RemoveValue(const ezPropertyPath& path, ezVariant index)
{
  ezStringBuilder sPathString = path.GetPathString();

  ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sPathString, storageInfo))
  {
    if (storageInfo->m_Type == ezVariant::Type::Invalid)
      return false;

    const ezAbstractProperty* pProp = ezToolsReflectionUtils::GetPropertyByPath(GetType(), path);
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
            changedValues.RemoveAt(uiIndex);
            m_Data[storageInfo->m_uiIndex] = changedValues;
            return true;
          }
        }
      }
      break;
    }
  }
  return false;
}

bool ezReflectedTypeStorageAccessor::MoveValue(const ezPropertyPath& path, ezVariant oldIndex, ezVariant newIndex)
{
  ezStringBuilder sPathString = path.GetPathString();

  ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sPathString, storageInfo))
  {
    if (storageInfo->m_Type == ezVariant::Type::Invalid)
      return false;

    const ezAbstractProperty* pProp = ezToolsReflectionUtils::GetPropertyByPath(GetType(), path);
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
            changedValues.RemoveAt(uiOldIndex);
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
    }
  }
  return false;
}

ezVariant ezReflectedTypeStorageAccessor::GetPropertyChildIndex(const ezPropertyPath& path, const ezVariant& value) const
{
  ezStringBuilder sPathString = path.GetPathString();

  ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sPathString, storageInfo))
  {
    if (storageInfo->m_Type == ezVariant::Type::Invalid)
      return ezVariant();

    const ezAbstractProperty* pProp = ezToolsReflectionUtils::GetPropertyByPath(GetType(), path);
    if (pProp == nullptr)
      return ezVariant();

    switch (pProp->GetCategory())
    {
    case ezPropertyCategory::Array:
    case ezPropertyCategory::Set:
      {
        const auto SpecVarType = pProp->GetFlags().IsSet(ezPropertyFlags::StandardType) ? pProp->GetSpecificType()->GetVariantType() : ezVariantType::Uuid;
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
    }
  }
  return ezVariant();
}


