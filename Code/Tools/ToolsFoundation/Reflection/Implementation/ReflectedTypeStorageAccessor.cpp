#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>


////////////////////////////////////////////////////////////////////////
// ezReflectedTypeStorageAccessor public functions
////////////////////////////////////////////////////////////////////////

ezReflectedTypeStorageAccessor::ezReflectedTypeStorageAccessor(ezReflectedTypeHandle hReflectedType) : ezIReflectedTypeAccessor(hReflectedType)
{
  
  const ezReflectedType* pType = hReflectedType.GetType();
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
    m_Data[storageInfo.m_uiIndex] = ezToolsReflectionUtils::GetDefaultVariantFromType(storageInfo.m_Type);
  }
}

ezReflectedTypeStorageAccessor::~ezReflectedTypeStorageAccessor()
{
  ezReflectedTypeStorageManager::RemoveStorageAccessor(this);
}

const ezVariant ezReflectedTypeStorageAccessor::GetValue(const ezPropertyPath& path) const
{
  ezString sPathString = ezToolsReflectionUtils::GetStringFromPropertyPath(path);

  ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sPathString, storageInfo))
  {
    if (storageInfo->m_Type == ezVariant::Type::Invalid)
      return ezVariant();

    return m_Data[storageInfo->m_uiIndex];
  }
  return ezVariant();
}

bool ezReflectedTypeStorageAccessor::SetValue(const ezPropertyPath& path, const ezVariant& value)
{
  ezString sPathString = ezToolsReflectionUtils::GetStringFromPropertyPath(path);

  ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sPathString, storageInfo))
  {
    if (value.IsA<ezString>())
    {
      const ezReflectedProperty* pProp = GetReflectedTypeHandle().GetType()->GetPropertyByPath(path);
      if (pProp == nullptr)
        return false;

      ezInt64 iValue;
      ezToolsReflectionUtils::StringToEnumeration(pProp->m_hTypeHandle.GetType(), value.Get<ezString>(), iValue);
      m_Data[storageInfo->m_uiIndex] = ezVariant(iValue).ConvertTo(storageInfo->m_Type);
    }
    else if (value.CanConvertTo(storageInfo->m_Type))
    {
      // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
      // that may have a different type now as someone reloaded the type information and replaced a type.
      m_Data[storageInfo->m_uiIndex] = value.ConvertTo(storageInfo->m_Type);
      return true;
    }
    
    
  }
  return false;
}
