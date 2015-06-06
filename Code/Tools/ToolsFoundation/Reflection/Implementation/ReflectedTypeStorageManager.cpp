#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Document/Document.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/Log.h>

ezMap<const ezRTTI*, ezReflectedTypeStorageManager::ReflectedTypeStorageMapping*> ezReflectedTypeStorageManager::m_ReflectedTypeToStorageMapping;

EZ_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, ReflectedTypeStorageManager)

BEGIN_SUBSYSTEM_DEPENDENCIES
"Core",
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

void ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::AddProperties(const ezRTTI* pType)
{
  // Mark all properties as invalid. Thus, when a property is dropped we know it is no longer valid.
  // All others will be set to their old or new value by the AddPropertiesRecursive function. 
  for (auto it = m_PathToStorageInfoTable.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_Type = ezVariant::Type::Invalid;
  }
  AddPropertiesRecursive(pType, "");
}

void ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::AddPropertiesRecursive(const ezRTTI* pType, const char* szPath)
{
  // Parse parent class
  const ezRTTI* pParent = pType->GetParentType();
  if (pParent != nullptr)
    AddPropertiesRecursive(pParent, szPath);

  ezStringBuilder pathBuilder;
  ezString path;

  // Parse properties
  const ezUInt32 uiPropertyCount = pType->GetProperties().GetCount();
  for (ezUInt32 i = 0; i < uiPropertyCount; ++i)
  {
    const ezAbstractProperty* pProperty = pType->GetProperties()[i];

    // Build property path
    pathBuilder = szPath;
    pathBuilder.AppendPath(pProperty->GetPropertyName());

    path = pathBuilder;

    if (pProperty->GetFlags().IsSet(ezPropertyFlags::StandardType) && pProperty->GetCategory() == ezPropertyCategory::Member)
    {
      const ezAbstractMemberProperty* pMember = static_cast<const ezAbstractMemberProperty*>(pProperty);

      // POD types are added to the dictionary
      StorageInfo* storageInfo = nullptr;
      if (m_PathToStorageInfoTable.TryGetValue(path, storageInfo))
      {
        // Value already present, update type and instances
        storageInfo->m_Type = pMember->GetPropertyType()->GetVariantType();
        UpdateInstances(storageInfo->m_uiIndex, pProperty);
      }
      else
      {
        const ezUInt16 uiIndex = (ezUInt16)m_PathToStorageInfoTable.GetCount();

        // Add value, new entries are appended
        m_PathToStorageInfoTable.Insert(path, StorageInfo(uiIndex, pMember->GetPropertyType()->GetVariantType()));
        AddPropertyToInstances(uiIndex, pProperty);
      }
    }
    else if (pProperty->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
    {
      // Enum and bitflags types are added to the dictionary
      StorageInfo* storageInfo = nullptr;
      if (m_PathToStorageInfoTable.TryGetValue(path, storageInfo))
      {
        // Value already present, update type and instances
        storageInfo->m_Type = ezVariant::Type::Int64;
        UpdateInstances(storageInfo->m_uiIndex, pProperty);
      }
      else
      {
        ezUInt16 uiIndex = (ezUInt16)m_PathToStorageInfoTable.GetCount();
        // Add value, new entries are appended as int64
        m_PathToStorageInfoTable.Insert(path, StorageInfo(uiIndex, ezVariant::Type::Int64));
        AddPropertyToInstances(uiIndex, pProperty);
      }
    }
    else if (pProperty->GetCategory() == ezPropertyCategory::Member)
    {
      const ezAbstractMemberProperty* pMember = static_cast<const ezAbstractMemberProperty*>(pProperty);

      // Not POD type, recurse further
      AddPropertiesRecursive(pMember->GetPropertyType(), path);
    }
  }
}

void ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::UpdateInstances(ezUInt32 uiIndex, const ezAbstractProperty* pProperty)
{
  for (auto it = m_Instances.GetIterator(); it.IsValid(); ++it)
  {
    ezDynamicArray<ezVariant>& data = it.Key()->m_Data;
    EZ_ASSERT_DEV(uiIndex < data.GetCount(), "ezReflectedTypeStorageAccessor found with fewer properties that is should have!");
    ezVariant& value = data[uiIndex];
    // Did the type change from what it was previously?

    if (pProperty->GetCategory() != ezPropertyCategory::Member)
      continue;

    const ezAbstractMemberProperty* pMember = static_cast<const ezAbstractMemberProperty*>(pProperty);

    if (value.GetType() == pMember->GetPropertyType()->GetVariantType())
    {
      // The types are equal so nothing needs to be done. The current value will stay valid.
      // This should be the most common case.
      continue;
    }

    if (value.GetType() == ezVariant::Type::Invalid)
    {
      // The old data point was never set, this can happen if an instance was created after a ezRTTI
      // was updated and a property was removed. In that case the index of the old property would still exist
      // but its value would never be set. Now if the property gets added again its value will be invalid on
      // this particular instance.
      value = ezToolsReflectionUtils::GetDefaultVariantFromType(pMember->GetPropertyType()->GetVariantType());

      continue;
    }

    if (value.GetType() != pMember->GetPropertyType()->GetVariantType())
    {
      // The type has changed but we have a valid value stored. Assume that the type of a property was changed
      // and try to convert the value.
      ezResult res(EZ_FAILURE);
      value = value.ConvertTo(pMember->GetPropertyType()->GetVariantType(), &res);
      if (res == EZ_FAILURE)
      {
        value = ezToolsReflectionUtils::GetDefaultVariantFromType(pMember->GetPropertyType()->GetVariantType());
      }
    }
  }
}

void ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::AddPropertyToInstances(ezUInt32 uiIndex, const ezAbstractProperty* pProperty)
{
  if (pProperty->GetCategory() != ezPropertyCategory::Member)
    return;

  const ezAbstractMemberProperty* pMember = static_cast<const ezAbstractMemberProperty*>(pProperty);

  for (auto it = m_Instances.GetIterator(); it.IsValid(); ++it)
  {
    ezDynamicArray<ezVariant>& data = it.Key()->m_Data;
    EZ_ASSERT_DEV(data.GetCount() == uiIndex, "ezReflectedTypeStorageAccessor found with a property count that does not match its storage mapping!");
    data.PushBack(ezToolsReflectionUtils::GetDefaultVariantFromType(pMember->GetPropertyType()->GetVariantType()));
  }
}


////////////////////////////////////////////////////////////////////////
// ezReflectedTypeStorageManager private functions
////////////////////////////////////////////////////////////////////////

void ezReflectedTypeStorageManager::Startup()
{
  ezPhantomRttiManager::m_TypeAddedEvent.AddEventHandler(TypeAddedEvent);
  ezPhantomRttiManager::m_TypeChangedEvent.AddEventHandler(TypeChangedEvent);
  ezPhantomRttiManager::m_TypeRemovedEvent.AddEventHandler(TypeRemovedEvent);

  ezToolsReflectionUtils::RegisterType(ezGetStaticRTTI<ezEnumBase>());
  ezToolsReflectionUtils::RegisterType(ezGetStaticRTTI<ezBitflagsBase>());
  ezToolsReflectionUtils::RegisterType(ezGetStaticRTTI<ezDocumentInfo>());
}

void ezReflectedTypeStorageManager::Shutdown()
{
  ezPhantomRttiManager::m_TypeAddedEvent.RemoveEventHandler(TypeAddedEvent);
  ezPhantomRttiManager::m_TypeChangedEvent.RemoveEventHandler(TypeChangedEvent);
  ezPhantomRttiManager::m_TypeRemovedEvent.RemoveEventHandler(TypeRemovedEvent);

  for (auto it = m_ReflectedTypeToStorageMapping.GetIterator(); it.IsValid(); ++it)
  {
    ReflectedTypeStorageMapping* pMapping = it.Value();

    for (auto inst : pMapping->m_Instances)
    {
      const char* sz = inst->GetType()->GetTypeName();
      ezLog::Error("Type '%s' survived shutdown!", sz);
    }

    EZ_ASSERT_DEV(pMapping->m_Instances.IsEmpty(), "A type was removed which still has instances using the type!");
    EZ_DEFAULT_DELETE(pMapping);
  }
  m_ReflectedTypeToStorageMapping.Clear();
}

const ezReflectedTypeStorageManager::ReflectedTypeStorageMapping* ezReflectedTypeStorageManager::AddStorageAccessor(ezReflectedTypeStorageAccessor* pInstance)
{
  ReflectedTypeStorageMapping* pMapping = m_ReflectedTypeToStorageMapping[pInstance->GetType()];
  pMapping->m_Instances.Insert(pInstance);
  return pMapping;
}

void ezReflectedTypeStorageManager::RemoveStorageAccessor(ezReflectedTypeStorageAccessor* pInstance)
{
  ReflectedTypeStorageMapping* pMapping = m_ReflectedTypeToStorageMapping[pInstance->GetType()];
  pMapping->m_Instances.Remove(pInstance);
}

void ezReflectedTypeStorageManager::TypeAddedEvent(const ezPhantomTypeChange& data)
{
  const ezRTTI* pType = data.m_pChangedType;
  EZ_ASSERT_DEV(pType != nullptr, "A type was added but it has an invalid handle!");

  ReflectedTypeStorageMapping* pMapping = EZ_DEFAULT_NEW(ReflectedTypeStorageMapping);
  EZ_ASSERT_DEV(!m_ReflectedTypeToStorageMapping.Find(data.m_pChangedType).IsValid(), "The type '%s' was added twice!", pType->GetTypeName());
  pMapping->AddProperties(pType);

  m_ReflectedTypeToStorageMapping.Insert(data.m_pChangedType, pMapping);
}

void ezReflectedTypeStorageManager::TypeChangedEvent(const ezPhantomTypeChange& data)
{
  const ezRTTI* pNewType = data.m_pChangedType;
  EZ_ASSERT_DEV(pNewType != nullptr, "A type was updated but its handle is invalid!");

  ReflectedTypeStorageMapping* pMapping = m_ReflectedTypeToStorageMapping[data.m_pChangedType];
  EZ_ASSERT_DEV(pMapping != nullptr, "A type was updated but no mapping exists for it!");

  if (pNewType->GetParentType() != nullptr && ezStringUtils::IsEqual(pNewType->GetParentType()->GetTypeName(), "ezEnumBase"))
  {
    EZ_ASSERT_DEV(false, "Updating enums not implemented yet!");
  }
  else if (pNewType->GetParentType() != nullptr && ezStringUtils::IsEqual(pNewType->GetParentType()->GetTypeName(), "ezBitflagsBase"))
  {
    EZ_ASSERT_DEV(false, "Updating bitflags not implemented yet!");
  }

  pMapping->AddProperties(pNewType);

  ezSet<ezRTTI*> dependencies;
  // Update all types that either derive from the changed type or have the type as a member.
  for (auto it = m_ReflectedTypeToStorageMapping.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Key() == data.m_pChangedType)
      continue;

    const ezRTTI* pType = it.Key();

    EZ_ASSERT_DEV(pType != nullptr, "Invalid type pointer");

    // Check whether the changed type comes up in the transitive dependency list.
    // If that is the case we need to update this type as well.
    ezSet<const ezRTTI*> DepTypes;
    ezReflectionUtils::GatherDependentTypes(pType, DepTypes);

    if (DepTypes.Contains(data.m_pChangedType))
      it.Value()->AddProperties(pType);
  }
}

void ezReflectedTypeStorageManager::TypeRemovedEvent(const ezPhantomTypeChange& data)
{
  ReflectedTypeStorageMapping* pMapping = m_ReflectedTypeToStorageMapping[data.m_pChangedType];
  EZ_ASSERT_DEV(pMapping != nullptr, "A type was removed but no mapping ever exited for it!");
  EZ_ASSERT_DEV(pMapping->m_Instances.IsEmpty(), "A type was removed which still has instances using the type!");
  m_ReflectedTypeToStorageMapping.Remove(data.m_pChangedType);
  EZ_DEFAULT_DELETE(pMapping);
}
