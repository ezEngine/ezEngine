#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Configuration/Startup.h>

ezEvent<const ezReflectedTypeChange&> ezReflectedTypeManager::m_TypeAddedEvent;
ezEvent<const ezReflectedTypeChange&> ezReflectedTypeManager::m_TypeChangedEvent;
ezEvent<const ezReflectedTypeChange&> ezReflectedTypeManager::m_TypeRemovedEvent;

ReflectedTypeTable ezReflectedTypeManager::m_Types;
ezHashTable<const char*, ezReflectedTypeHandle> ezReflectedTypeManager::m_NameToHandle;

EZ_BEGIN_SUBSYSTEM_DECLARATION(Core, ReflectedTypeManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    ezReflectedTypeManager::Startup();
  }

  ON_CORE_SHUTDOWN
  {
    ezReflectedTypeManager::Shutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION

////////////////////////////////////////////////////////////////////////
// ezReflectedTypeManager public functions
////////////////////////////////////////////////////////////////////////

ezReflectedTypeHandle ezReflectedTypeManager::RegisterType(const ezReflectedTypeDescriptor& desc)
{
  ezReflectedType* pType;
  if (ezStringUtils::IsNullOrEmpty(desc.m_sParentTypeName.GetData()))
  {
    pType = EZ_DEFAULT_NEW(ezReflectedType)(desc.m_sTypeName.GetData(), desc.m_sPluginName.GetData(), ezReflectedTypeHandle());
  }
  else
  {
    ezReflectedTypeHandle hParent = GetTypeHandleByName(desc.m_sParentTypeName.GetData());
    EZ_ASSERT_DEV(!hParent.IsInvalidated(), "ezReflectedTypeManager::RegisterType: Can't register a type to which the parent type is not known yet!");
    pType = EZ_DEFAULT_NEW(ezReflectedType)(desc.m_sTypeName.GetData(), desc.m_sPluginName.GetData(), hParent);
    pType->m_Dependencies.Insert(hParent);
  }

  pType->m_sDefaultInitialization = desc.m_sDefaultInitialization;

  ezUInt32 iPropertyCount = 0;
  ezUInt32 iConstantCount = 0;
  for (const ezReflectedPropertyDescriptor& propDesc : desc.m_Properties)
  {
    if (propDesc.m_Flags.IsSet(PropertyFlags::IsConstant))
      iConstantCount++;
    else
      iPropertyCount++;
  }

  // Convert properties
  const ezUInt32 iCount = desc.m_Properties.GetCount();
  pType->m_Properties.Reserve(iPropertyCount);

  for (ezUInt32 i = 0; i < iCount; i++)
  {
    const ezReflectedPropertyDescriptor& propDesc = desc.m_Properties[i];
    if (propDesc.m_Flags.IsSet(PropertyFlags::IsConstant))
      continue;

    if (propDesc.m_Type != ezVariant::Type::Invalid)
    {
      pType->m_Properties.PushBack(ezReflectedProperty(propDesc.m_sName.GetData(), propDesc.m_Type, propDesc.m_Flags));
    }
    else
    {
      ezReflectedTypeHandle hProp = GetTypeHandleByName(propDesc.m_sType.GetData());
      EZ_ASSERT_DEV(!hProp.IsInvalidated(), "ezReflectedTypeManager::RegisterType: Can't register a type to which a property's type is not known yet!");
      pType->m_Properties.PushBack(ezReflectedProperty(propDesc.m_sName.GetData(), hProp, propDesc.m_Flags));
      pType->m_Dependencies.Insert(hProp);
    }
  }
  pType->RegisterProperties();
  
  // Convert constants
  pType->m_Constants.Reserve(iConstantCount);
  for (ezUInt32 i = 0; i < iCount; i++)
  {
    const ezReflectedPropertyDescriptor& propDesc = desc.m_Properties[i];
    if (!propDesc.m_Flags.IsSet(PropertyFlags::IsConstant))
      continue;

    if (propDesc.m_Type != ezVariant::Type::Invalid)
    {
      pType->m_Constants.PushBack(ezReflectedConstant(propDesc.m_sName.GetData(), propDesc.m_ConstantValue));
    }
    else
    {
      EZ_ASSERT_DEV(false, "Non-pod constants are not supported yet.");
    }
  }
  pType->RegisterConstants();

  // Register finished Type
  ezReflectedTypeHandle hType = GetTypeHandleByName(desc.m_sTypeName.GetData());
  if (!hType.IsInvalidated())
  {
    // Type already present, swap type definition and broadcast changed event.
    pType->m_hType = hType;
    ezReflectedType* pOldType = m_Types[hType];
    m_Types[hType] = pType;

    {
      ezReflectedTypeChange msg;
      msg.m_hType = hType;
      msg.pOldType = pOldType;
      msg.pNewType = pType;
      m_TypeChangedEvent.Broadcast(msg);
    }

    EZ_DEFAULT_DELETE(pOldType);
  }
  else
  {
    // Type is new, add and broadcast added event.
    hType = m_Types.Insert(pType);
    m_NameToHandle.Insert(pType->GetTypeName().GetString().GetData(), hType);
    pType->m_hType = hType;

    {
      ezReflectedTypeChange msg;
      msg.m_hType = hType;
      msg.pOldType = nullptr;
      msg.pNewType = pType;
      m_TypeAddedEvent.Broadcast(msg);
    }
  }



  return hType;
}

bool ezReflectedTypeManager::UnregisterType(ezReflectedTypeHandle hType)
{
  ezReflectedType* pType = GetTypeInternal(hType);
  if (pType == nullptr)
    return false;

  {
    ezReflectedTypeChange msg;
    msg.m_hType = hType;
    msg.pOldType = pType;
    msg.pNewType = nullptr;
    m_TypeRemovedEvent.Broadcast(msg);
  }

  m_NameToHandle.Remove(pType->GetTypeName().GetString().GetData());
  m_Types.Remove(hType);

  EZ_DEFAULT_DELETE(pType);
  return true;
}

ezReflectedTypeHandle ezReflectedTypeManager::GetTypeHandleByName(const char* szName)
{
  ezReflectedTypeHandle hType;
  if (m_NameToHandle.TryGetValue(szName, hType))
    return hType;

  hType.Invalidate();
  return hType;
}

const ezReflectedType* ezReflectedTypeManager::GetType(ezReflectedTypeHandle hType)
{
  return GetTypeInternal(hType);
}


////////////////////////////////////////////////////////////////////////
// ezReflectedTypeManager private functions
////////////////////////////////////////////////////////////////////////

ezReflectedType* ezReflectedTypeManager::GetTypeInternal(ezReflectedTypeHandle hType)
{
  ezReflectedType* pType = nullptr;
  if (m_Types.TryGetValue(hType, pType))
    return pType;

  return nullptr;
}

void ezReflectedTypeManager::Startup()
{
  // Add all foundation types.
}

void ezReflectedTypeManager::Shutdown()
{
  while (!m_Types.IsEmpty())
  {
    ReflectedTypeTable::Iterator it = m_Types.GetIterator();
    UnregisterType(it.Id());
  }

  EZ_ASSERT_DEV(m_Types.IsEmpty() && m_NameToHandle.IsEmpty(), "ezReflectedTypeManager::Shutdown: Removal of types failed!");
}
