#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Configuration/Startup.h>

ezEvent<ezReflectedTypeChange&> ezReflectedTypeManager::m_TypeAddedEvent;
ezEvent<ezReflectedTypeChange&> ezReflectedTypeManager::m_TypeChangedEvent;
ezEvent<ezReflectedTypeChange&> ezReflectedTypeManager::m_TypeRemovedEvent;

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

ezReflectedTypeHandle ezReflectedTypeManager::RegisterType(ezReflectedTypeDescriptor& desc)
{
  EZ_ASSERT(m_NameToHandle.KeyExists(desc.m_sTypeName.GetData()) == false, "ezReflectedTypeManager::RegisterType: Updating of types not implemented yet!");
  
  ezReflectedType* pType;
  if (ezStringUtils::IsNullOrEmpty(desc.m_sParentTypeName.GetData()))
  {
    pType = EZ_DEFAULT_NEW(ezReflectedType)(desc.m_sTypeName.GetData(), desc.m_sPluginName.GetData(), ezReflectedTypeHandle());
  }
  else
  {
    EZ_ASSERT(m_NameToHandle.KeyExists(desc.m_sParentTypeName.GetData()) == true, "ezReflectedTypeManager::RegisterType: Can't register a type to which the parent type is not known yet!");
    pType = EZ_DEFAULT_NEW(ezReflectedType)(desc.m_sTypeName.GetData(), desc.m_sPluginName.GetData(), m_NameToHandle[desc.m_sParentTypeName.GetData()]);
  }

  // Convert properties
  const ezUInt32 iCount = desc.m_Properties.GetCount();
  pType->m_Properties.Reserve(iCount);

  for (ezUInt32 i = 0; i < iCount; i++)
  {
    ezReflectedPropertyDescriptor& propDesc = desc.m_Properties[i];
    if (propDesc.m_Type != ezVariant::Type::Invalid)
    {
      pType->m_Properties.PushBack(ezReflectedProperty(propDesc.m_sName.GetData(), propDesc.m_Type, propDesc.m_Flags));
    }
    else
    {
      EZ_ASSERT(m_NameToHandle.KeyExists(propDesc.m_sType.GetData()) == true, "ezReflectedTypeManager::RegisterType: Can't register a type to which a property's type is not known yet!");
      pType->m_Properties.PushBack(ezReflectedProperty(propDesc.m_sName.GetData(), m_NameToHandle[propDesc.m_sType.GetData()], propDesc.m_Flags));
    }
  }
  pType->RegisterProperties();

  // Register finished Type
  ezReflectedTypeHandle hType = m_Types.Insert(pType);
  m_NameToHandle.Insert(pType->GetTypeName().GetString().GetData(), hType);
  pType->m_hType = hType;

  {
    ezReflectedTypeChange msg;
    msg.m_hType = hType;
    msg.pOldType = nullptr;
    msg.pNewType = pType;
    m_TypeAddedEvent.Broadcast(msg);
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

  EZ_ASSERT(m_Types.IsEmpty() && m_NameToHandle.IsEmpty(), "ezReflectedTypeManager::Shutdown: Removal of types failed!");
}
