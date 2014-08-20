#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>


////////////////////////////////////////////////////////////////////////
// ezReflectedTypeStorageAccessor public functions
////////////////////////////////////////////////////////////////////////

ezReflectedTypeStorageAccessor::ezReflectedTypeStorageAccessor(ezReflectedTypeHandle hReflectedType) : ezIReflectedTypeAccessor(hReflectedType)
{
  
  const ezReflectedType* pType = hReflectedType.GetType();
  EZ_ASSERT(pType != nullptr, "Trying to construct an ezReflectedTypeStorageAccessor for an invalid type!");
  m_pMapping = ezReflectedTypeStorageManager::AddStorageAccessor(this);
  EZ_ASSERT(m_pMapping != nullptr, "The type for this ezReflectedTypeStorageAccessor is unknown to the ezReflectedTypeStorageManager!");

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
  EZ_ASSERT(path.GetCount() > 0, "Can't set a property with an empty path!");
  ezStringBuilder pathBuilder(path[0]);
  const ezUInt32 pathLength = path.GetCount();
  for (ezUInt32 i = 1; i < pathLength; ++i)
  {
    pathBuilder.Append("/", path[i]);
  }

  ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(ezString(pathBuilder.GetData()), storageInfo))
  {
    if (storageInfo->m_Type == ezVariant::Type::Invalid)
      return ezVariant();

    return m_Data[storageInfo->m_uiIndex];
  }
  return ezVariant();
}

bool ezReflectedTypeStorageAccessor::SetValue(const ezPropertyPath& path, const ezVariant& value)
{
  EZ_ASSERT(path.GetCount() > 0, "Can't set a property with an empty path!");
  ezStringBuilder pathBuilder(path[0]);
  const ezUInt32 pathLength = path.GetCount();
  for (ezUInt32 i = 1; i < pathLength; ++i)
  {
    pathBuilder.Append("/", path[i]);
  }

  ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(ezString(pathBuilder.GetData()), storageInfo))
  {
    // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
    // that may have a different type now as someone reloaded the type information and replaced a type.
    if (value.CanConvertTo(storageInfo->m_Type))
    {
      m_Data[storageInfo->m_uiIndex] = value.ConvertTo(storageInfo->m_Type);
      return true;
    }
  }
  return false;
}
