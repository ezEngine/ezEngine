#include <PCH.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Document/Document.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/Log.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

ezMap<const ezRTTI*, ezReflectedTypeStorageManager::ReflectedTypeStorageMapping*> ezReflectedTypeStorageManager::m_ReflectedTypeToStorageMapping;

EZ_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, ReflectedTypeStorageManager)

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

void ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::AddProperties(const ezRTTI* pType)
{
  // Mark all properties as invalid. Thus, when a property is dropped we know it is no longer valid.
  // All others will be set to their old or new value by the AddPropertiesRecursive function.
  for (auto it = m_PathToStorageInfoTable.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_Type = ezVariant::Type::Invalid;
  }

  ezSet<const ezDocumentObject*> requiresPatchingEmbeddedClass;
  AddPropertiesRecursive(pType, requiresPatchingEmbeddedClass);

  for (const ezDocumentObject* pObject : requiresPatchingEmbeddedClass)
  {
    pObject->GetDocumentObjectManager()->PatchEmbeddedClassObjects(pObject);
  }
}

void ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::AddPropertiesRecursive(const ezRTTI* pType, ezSet<const ezDocumentObject*>& requiresPatchingEmbeddedClass)
{
  // Parse parent class
  const ezRTTI* pParent = pType->GetParentType();
  if (pParent != nullptr)
    AddPropertiesRecursive(pParent, requiresPatchingEmbeddedClass);

  // Parse properties
  const ezUInt32 uiPropertyCount = pType->GetProperties().GetCount();
  for (ezUInt32 i = 0; i < uiPropertyCount; ++i)
  {
    const ezAbstractProperty* pProperty = pType->GetProperties()[i];

    ezString path = pProperty->GetPropertyName();

    StorageInfo* storageInfo = nullptr;
    if (m_PathToStorageInfoTable.TryGetValue(path, storageInfo))
    {
      // Value already present, update type and instances
      storageInfo->m_Type = GetStorageType(pProperty);
      storageInfo->m_DefaultValue = ezToolsReflectionUtils::GetStorageDefault(pProperty);
      UpdateInstances(storageInfo->m_uiIndex, pProperty, requiresPatchingEmbeddedClass);
    }
    else
    {
      const ezUInt16 uiIndex = (ezUInt16)m_PathToStorageInfoTable.GetCount();

      // Add value, new entries are appended
      m_PathToStorageInfoTable.Insert(path, StorageInfo(uiIndex, GetStorageType(pProperty), ezToolsReflectionUtils::GetStorageDefault(pProperty)));
      AddPropertyToInstances(uiIndex, pProperty, requiresPatchingEmbeddedClass);
    }
  }
}

void ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::UpdateInstances(ezUInt32 uiIndex, const ezAbstractProperty* pProperty, ezSet<const ezDocumentObject*>& requiresPatchingEmbeddedClass)
{
  for (auto it = m_Instances.GetIterator(); it.IsValid(); ++it)
  {
    ezDynamicArray<ezVariant>& data = it.Key()->m_Data;
    EZ_ASSERT_DEV(uiIndex < data.GetCount(), "ezReflectedTypeStorageAccessor found with fewer properties that is should have!");
    ezVariant& value = data[uiIndex];

        const auto SpecVarType = GetStorageType(pProperty);

    switch (pProperty->GetCategory())
    {
    case ezPropertyCategory::Member:
      {
        if (pProperty->GetFlags().IsSet(ezPropertyFlags::Class) && !pProperty->GetFlags().IsSet(ezPropertyFlags::Pointer))
        {
          // Did the type change from what it was previously?
          if (value.GetType() == SpecVarType)
          {
            if (!value.Get<ezUuid>().IsValid())
            {
              requiresPatchingEmbeddedClass.Insert(it.Key()->GetOwner());
            }
          }
          else
          {
            value = ezToolsReflectionUtils::GetStorageDefault(pProperty);
            requiresPatchingEmbeddedClass.Insert(it.Key()->GetOwner());
          }
          continue;
        }
        else
        {
          // Did the type change from what it was previously?
          if (value.GetType() == SpecVarType)
          {
            // The types are equal so nothing needs to be done. The current value will stay valid.
            // This should be the most common case.
            continue;
          }
          else
          {
            // The type is new or has changed but we have a valid value stored. Assume that the type of a property was changed
            // and try to convert the value.
            if (value.CanConvertTo(SpecVarType))
            {
              value = value.ConvertTo(SpecVarType);
            }
            else
            {
              value = ezToolsReflectionUtils::GetStorageDefault(pProperty);
            }
            continue;
          }
        }
      }
      break;
    case ezPropertyCategory::Array:
    case ezPropertyCategory::Set:
      {
        if (value.GetType() != ezVariantType::VariantArray)
        {
          value = ezVariantArray();
          continue;
        }
        ezVariantArray values = value.Get<ezVariantArray>();
        if (values.IsEmpty())
          continue;

        // Same conversion logic as for ezPropertyCategory::Member, but for each element instead.
        for (ezVariant& var : values)
        {
          if (var.GetType() == SpecVarType)
          {
            continue;
          }
          else
          {
            ezResult res(EZ_FAILURE);
            var = var.ConvertTo(SpecVarType, &res);
            if (res == EZ_FAILURE)
            {
              var = ezToolsReflectionUtils::GetDefaultValue(pProperty);
            }
          }
        }
        value = values;
      }
      break;
    case ezPropertyCategory::Map:
      {
        if (value.GetType() != ezVariantType::VariantDictionary)
        {
          value = ezVariantDictionary();
          continue;
        }
        ezVariantDictionary values = value.Get<ezVariantDictionary>();
        if (values.IsEmpty())
          continue;

        // Same conversion logic as for ezPropertyCategory::Member, but for each element instead.
        for (auto it = values.GetIterator(); it.IsValid(); ++it)
        {
          if (it.Value().GetType() == SpecVarType)
          {
            continue;
          }
          else
          {
            ezResult res(EZ_FAILURE);
            it.Value() = it.Value().ConvertTo(SpecVarType, &res);
            if (res == EZ_FAILURE)
            {
              it.Value() = ezToolsReflectionUtils::GetDefaultValue(pProperty);
            }
          }
        }
        value = values;
      }
      break;
    default:
      break;
    }
  }
}

void ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::AddPropertyToInstances(ezUInt32 uiIndex, const ezAbstractProperty* pProperty, ezSet<const ezDocumentObject*>& requiresPatchingEmbeddedClass)
{
  if (pProperty->GetCategory() != ezPropertyCategory::Member)
    return;

  for (auto it = m_Instances.GetIterator(); it.IsValid(); ++it)
  {
    ezDynamicArray<ezVariant>& data = it.Key()->m_Data;
    EZ_ASSERT_DEV(data.GetCount() == uiIndex, "ezReflectedTypeStorageAccessor found with a property count that does not match its storage mapping!");
    data.PushBack(ezToolsReflectionUtils::GetStorageDefault(pProperty));
    if (pProperty->GetFlags().IsSet(ezPropertyFlags::Class) && !pProperty->GetFlags().IsSet(ezPropertyFlags::Pointer))
    {
      requiresPatchingEmbeddedClass.Insert(it.Key()->GetOwner());
    }
  }
}


ezVariantType::Enum ezReflectedTypeStorageManager::ReflectedTypeStorageMapping::GetStorageType(const ezAbstractProperty* pProperty)
{
  ezVariantType::Enum type = ezVariantType::Uuid;
  switch (pProperty->GetCategory())
  {
  case ezPropertyCategory::Member:
    {
      if (pProperty->GetFlags().IsSet(ezPropertyFlags::StandardType))
        type = pProperty->GetSpecificType()->GetVariantType();
      else if (pProperty->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
        type = ezVariantType::Int64;
    }
    break;
  case ezPropertyCategory::Array:
  case ezPropertyCategory::Set:
    {
      type = ezVariantType::VariantArray;
    }
    break;
  case ezPropertyCategory::Map:
    {
      type = ezVariantType::VariantDictionary;
    }
    break;
  default:
    break;
  }

  return type;
}

////////////////////////////////////////////////////////////////////////
// ezReflectedTypeStorageManager private functions
////////////////////////////////////////////////////////////////////////

void ezReflectedTypeStorageManager::Startup()
{
  ezPhantomRttiManager::s_Events.AddEventHandler(TypeEventHandler);
}

void ezReflectedTypeStorageManager::Shutdown()
{
  ezPhantomRttiManager::s_Events.RemoveEventHandler(TypeEventHandler);

  for (auto it = m_ReflectedTypeToStorageMapping.GetIterator(); it.IsValid(); ++it)
  {
    ReflectedTypeStorageMapping* pMapping = it.Value();

    for (auto inst : pMapping->m_Instances)
    {
      const char* sz = inst->GetType()->GetTypeName();
      ezLog::Error("Type '{0}' survived shutdown!", sz);
    }

    EZ_ASSERT_DEV(pMapping->m_Instances.IsEmpty(), "A type was removed which still has instances using the type!");
    EZ_DEFAULT_DELETE(pMapping);
  }
  m_ReflectedTypeToStorageMapping.Clear();
}

const ezReflectedTypeStorageManager::ReflectedTypeStorageMapping* ezReflectedTypeStorageManager::AddStorageAccessor(ezReflectedTypeStorageAccessor* pInstance)
{
  ReflectedTypeStorageMapping* pMapping = GetTypeStorageMapping(pInstance->GetType());
  pMapping->m_Instances.Insert(pInstance);
  return pMapping;
}

void ezReflectedTypeStorageManager::RemoveStorageAccessor(ezReflectedTypeStorageAccessor* pInstance)
{
  ReflectedTypeStorageMapping* pMapping = GetTypeStorageMapping(pInstance->GetType());
  pMapping->m_Instances.Remove(pInstance);
}

ezReflectedTypeStorageManager::ReflectedTypeStorageMapping* ezReflectedTypeStorageManager::GetTypeStorageMapping(const ezRTTI* pType)
{
  EZ_ASSERT_DEV(pType != nullptr, "Nullptr is not a valid type!");
  auto it = m_ReflectedTypeToStorageMapping.Find(pType);
  if (it.IsValid())
    return it.Value();

  ReflectedTypeStorageMapping* pMapping = EZ_DEFAULT_NEW(ReflectedTypeStorageMapping);
  pMapping->AddProperties(pType);
  m_ReflectedTypeToStorageMapping[pType] = pMapping;
  return pMapping;
}

void ezReflectedTypeStorageManager::TypeEventHandler(const ezPhantomRttiManagerEvent& e)
{
  switch (e.m_Type)
  {
  case ezPhantomRttiManagerEvent::Type::TypeAdded:
    {
      const ezRTTI* pType = e.m_pChangedType;
      EZ_ASSERT_DEV(pType != nullptr, "A type was added but it has an invalid handle!");

      EZ_ASSERT_DEV(!m_ReflectedTypeToStorageMapping.Find(e.m_pChangedType).IsValid(), "The type '{0}' was added twice!", pType->GetTypeName());
      GetTypeStorageMapping(e.m_pChangedType);
    }
    break;
  case ezPhantomRttiManagerEvent::Type::TypeChanged:
    {
      const ezRTTI* pNewType = e.m_pChangedType;
      EZ_ASSERT_DEV(pNewType != nullptr, "A type was updated but its handle is invalid!");

      ReflectedTypeStorageMapping* pMapping = m_ReflectedTypeToStorageMapping[e.m_pChangedType];
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
        if (it.Key() == e.m_pChangedType)
          continue;

        const ezRTTI* pType = it.Key();
        if (pType->IsDerivedFrom(e.m_pChangedType))
        {
          it.Value()->AddProperties(pType);
        }
      }
    }
    break;
  case ezPhantomRttiManagerEvent::Type::TypeRemoved:
    {
      ReflectedTypeStorageMapping* pMapping = m_ReflectedTypeToStorageMapping[e.m_pChangedType];
      EZ_ASSERT_DEV(pMapping != nullptr, "A type was removed but no mapping ever exited for it!");
      EZ_ASSERT_DEV(pMapping->m_Instances.IsEmpty(), "A type was removed which still has instances using the type!");
      m_ReflectedTypeToStorageMapping.Remove(e.m_pChangedType);
      EZ_DEFAULT_DELETE(pMapping);
    }
    break;
  }
}
