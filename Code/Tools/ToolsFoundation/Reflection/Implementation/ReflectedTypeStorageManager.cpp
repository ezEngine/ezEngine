#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageManager.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <Foundation/Configuration/Startup.h>

ezMap<ezReflectedTypeHandle, ezReflectedTypeStorageManager::ReflectedTypeStorageMapping*> ezReflectedTypeStorageManager::m_ReflectedTypeToStorageMapping;

EZ_BEGIN_SUBSYSTEM_DECLARATION(Core, ReflectedTypeStorageManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    ezReflectedTypeStorageManager::Startup();
  }

  ON_CORE_SHUTDOWN
  {
    ezReflectedTypeStorageManager::Shutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION

////////////////////////////////////////////////////////////////////////
// ezReflectedTypeStorageManager::ReflectedTypeStorageMapping public functions
////////////////////////////////////////////////////////////////////////

void ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::AddProperties(const ezReflectedType* pType)
{
  // Mark all properties as invalid. Thus, when a property is dropped we know it is no longer valid.
  // All others will be set to their old or new value by the AddPropertiesRecursive function. 
  for (auto it = m_PathToStorageInfoTable.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_Type = ezVariant::Type::Invalid;
  }
  AddPropertiesRecursive(pType, "");
}

void ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::AddPropertiesRecursive(const ezReflectedType* pType, const char* szPath)
{
  // Parse parent class
  ezReflectedTypeHandle hParent = pType->GetParentTypeHandle();
  if (!hParent.IsInvalidated())
    AddPropertiesRecursive(hParent.GetType(), szPath);
  
  // Parse properties
  ezUInt32 uiPropertyCount = pType->GetPropertyCount();
  for (ezUInt32 i = 0; i < uiPropertyCount; ++i)
  {
    const ezReflectedProperty* pProperty = pType->GetPropertyByIndex(i);
    // Build property path
    ezStringBuilder pathBuilder(szPath);
    if (!pathBuilder.IsEmpty())
      pathBuilder.Append("/");
    pathBuilder.Append(pProperty->m_sPropertyName.GetString().GetData());
    ezString path = pathBuilder;

    if (pProperty->m_Flags.IsSet(PropertyFlags::IsPOD))
    {
      // POD types are added to the dictionary
      StorageInfo* storageInfo = nullptr;
      if (m_PathToStorageInfoTable.TryGetValue(path, storageInfo))
      {
        // Value already present, update type and instances
        storageInfo->m_Type = pProperty->m_Type;
        UpdateInstances(storageInfo->m_uiIndex, pProperty);
      }
      else
      {
        ezUInt16 uiIndex = (ezUInt16)m_PathToStorageInfoTable.GetCount();
        // Add value, new entries are appended
        m_PathToStorageInfoTable.Insert(path, StorageInfo(uiIndex, pProperty->m_Type));
        AddPropertyToInstances(uiIndex, pProperty);
      }
    }
    else
    {
      // Not POD type, recurse further
      if (pProperty->m_hTypeHandle.IsInvalidated())
      {
        EZ_ASSERT(false, "A non-POD property was found that cannot be recursed into!");
      }
      else
      {
        AddPropertiesRecursive(pProperty->m_hTypeHandle.GetType(), path.GetData());
      }
    }
    
  }
}

void ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::UpdateInstances(ezUInt32 uiIndex, const ezReflectedProperty* pProperty)
{
  for (auto it = m_Instances.GetIterator(); it.IsValid(); ++it)
  {
    ezDynamicArray<ezVariant>& data = it.Key()->m_Data;
    EZ_ASSERT(uiIndex < data.GetCount(), "ezReflectedTypeStorageAccessor found with fewer properties that is should have!");
    ezVariant& value = data[uiIndex];
    // Did the type change from what it was previously?
    if (value.GetType() == pProperty->m_Type)
    {
      // The types are equal so nothing needs to be done. The current value will stay valid.
      // This should be the most common case.
      continue;
    }
    else if (value.GetType() == ezVariant::Type::Invalid)
    {
      // The old data point was never set, this can happen if an instance was created after a ezReflectedType
      // was updated and a property was removed. In that case the index of the old property would still exist
      // but its value would never be set. Now if the property gets added again its value will be invalid on
      // this particular instance.
      value = ezToolsReflectionUtils::GetDefaultVariantFromType(pProperty->m_Type);
    }
    else if (value.GetType() != pProperty->m_Type)
    {
      // The type has changed but we have a valid value stored. Assume that the type of a property was changed
      // and try to convert the value.
      ezResult res(EZ_FAILURE);
      value = value.ConvertTo(pProperty->m_Type, &res);
      if (res == EZ_FAILURE)
      {
        value = ezToolsReflectionUtils::GetDefaultVariantFromType(pProperty->m_Type);
      }
    }
  }
}

void ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::AddPropertyToInstances(ezUInt32 uiIndex, const ezReflectedProperty* pProperty)
{
  for (auto it = m_Instances.GetIterator(); it.IsValid(); ++it)
  {
    ezDynamicArray<ezVariant>& data = it.Key()->m_Data;
    EZ_ASSERT(data.GetCount() == uiIndex, "ezReflectedTypeStorageAccessor found with a property count that does not match its storage mapping!");
    data.PushBack(ezToolsReflectionUtils::GetDefaultVariantFromType(pProperty->m_Type));
  }
}


////////////////////////////////////////////////////////////////////////
// ezReflectedTypeStorageManager private functions
////////////////////////////////////////////////////////////////////////

void ezReflectedTypeStorageManager::Startup()
{
  ezReflectedTypeManager::m_TypeAddedEvent.AddEventHandler(TypeAddedEvent);
  ezReflectedTypeManager::m_TypeChangedEvent.AddEventHandler(TypeChangedEvent);
  ezReflectedTypeManager::m_TypeRemovedEvent.AddEventHandler(TypeRemovedEvent);
}

void ezReflectedTypeStorageManager::Shutdown()
{
  ezReflectedTypeManager::m_TypeAddedEvent.RemoveEventHandler(TypeAddedEvent);
  ezReflectedTypeManager::m_TypeChangedEvent.RemoveEventHandler(TypeChangedEvent);
  ezReflectedTypeManager::m_TypeRemovedEvent.RemoveEventHandler(TypeRemovedEvent);
}

const ezReflectedTypeStorageManager::ReflectedTypeStorageMapping* ezReflectedTypeStorageManager::AddStorageAccessor(ezReflectedTypeStorageAccessor* pInstance)
{
  ReflectedTypeStorageMapping* pMapping = m_ReflectedTypeToStorageMapping[pInstance->GetReflectedTypeHandle().m_InternalId];
  pMapping->m_Instances.Insert(pInstance);
  return pMapping;
}

void ezReflectedTypeStorageManager::RemoveStorageAccessor(ezReflectedTypeStorageAccessor* pInstance)
{
  ReflectedTypeStorageMapping* pMapping = m_ReflectedTypeToStorageMapping[pInstance->GetReflectedTypeHandle()];
  pMapping->m_Instances.Remove(pInstance);
}

void ezReflectedTypeStorageManager::TypeAddedEvent(ezReflectedTypeChange& data)
{
  const ezReflectedType* pType = data.pNewType;
  EZ_ASSERT(pType != nullptr, "A type was added but it has an invalid handle!");
  ReflectedTypeStorageMapping* pMapping = EZ_DEFAULT_NEW(ReflectedTypeStorageMapping);
  EZ_ASSERT(!m_ReflectedTypeToStorageMapping.Find(data.m_hType).IsValid(), "The type '%s' was added twice!", pType->GetTypeName().GetString().GetData());
  pMapping->AddProperties(pType);

  m_ReflectedTypeToStorageMapping.Insert(data.m_hType, pMapping);
}

void ezReflectedTypeStorageManager::TypeChangedEvent(ezReflectedTypeChange& data)
{
  const ezReflectedType* pNewType = data.pNewType;
  EZ_ASSERT(pNewType != nullptr, "A type was updated but its handle is invalid!");
  ReflectedTypeStorageMapping* pMapping = m_ReflectedTypeToStorageMapping[data.m_hType];
  EZ_ASSERT(pMapping != nullptr, "A type was updated but no mapping exists for it!");
  pMapping->AddProperties(pNewType);

  ezSet<ezReflectedTypeHandle> dependencies;
  // Update all types that either derive from the changed type or have the type as a member.
  for(auto it = m_ReflectedTypeToStorageMapping.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Key() == data.m_hType)
      continue;

    const ezReflectedType* pType = it.Key().GetType();
    // Check whether the changed type comes up in the transitive dependency list.
    // If that is the case we need to update this type as well.
    pType->GetDependencies(dependencies, true);
    if (dependencies.Find(data.m_hType).IsValid())
      it.Value()->AddProperties(pType);
  }
}

void ezReflectedTypeStorageManager::TypeRemovedEvent(ezReflectedTypeChange& data)
{
  ReflectedTypeStorageMapping* pMapping = m_ReflectedTypeToStorageMapping[data.m_hType];
  EZ_ASSERT(pMapping != nullptr, "A type was removed but no mapping ever exited for it!");
  EZ_ASSERT(pMapping->m_Instances.IsEmpty(), "A type was removed which still has instances using the type!");
  m_ReflectedTypeToStorageMapping.Remove(data.m_hType);
  EZ_DEFAULT_DELETE(pMapping);
}
