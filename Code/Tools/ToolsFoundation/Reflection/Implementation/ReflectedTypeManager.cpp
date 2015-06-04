#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Reflection/PhantomRtti.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Configuration/Startup.h>

ezEvent<const ezReflectedTypeChange&> ezReflectedTypeManager::m_TypeAddedEvent;
ezEvent<const ezReflectedTypeChange&> ezReflectedTypeManager::m_TypeChangedEvent;
ezEvent<const ezReflectedTypeChange&> ezReflectedTypeManager::m_TypeRemovedEvent;

ezSet<const ezRTTI*> ezReflectedTypeManager::m_RegisteredConcreteTypes;
ezHashTable<const char*, ezPhantomRTTI*> ezReflectedTypeManager::m_NameToPhantom;

EZ_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, ReflectedTypeManager)

BEGIN_SUBSYSTEM_DEPENDENCIES
"Core"
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

const ezRTTI* ezReflectedTypeManager::RegisterType(const ezReflectedTypeDescriptor& desc)
{
  ezRTTI* pType = ezRTTI::FindTypeByName(desc.m_sTypeName);
  ezPhantomRTTI* pPhantom = nullptr;
  m_NameToPhantom.TryGetValue(desc.m_sTypeName, pPhantom);

  // concrete type !
  if (pPhantom == nullptr && pType != nullptr)
  {
    if (m_RegisteredConcreteTypes.Contains(pType))
      return pType;

    m_RegisteredConcreteTypes.Insert(pType);

    ezReflectedTypeChange msg;
    msg.m_pChangedType = pType;
    m_TypeAddedEvent.Broadcast(msg);
    return pType;
  }

  if (pPhantom != nullptr && pPhantom->IsEqualToDescriptor(desc))
    return pPhantom;

  if (pPhantom == nullptr)
  {
    pPhantom = EZ_DEFAULT_NEW(ezPhantomRTTI, desc.m_sTypeName.GetData(), ezRTTI::FindTypeByName(desc.m_sParentTypeName),
                              desc.m_uiTypeSize, desc.m_uiTypeVersion, ezVariantType::Invalid, desc.m_Flags, desc.m_sPluginName.GetData());

    pPhantom->SetProperties(desc.m_Properties);

    m_NameToPhantom[pPhantom->GetTypeName()] = pPhantom;

    ezReflectedTypeChange msg;
    msg.m_pChangedType = pPhantom;
    m_TypeAddedEvent.Broadcast(msg);
  }
  else
  {
    pPhantom->UpdateType(desc);

    ezReflectedTypeChange msg;
    msg.m_pChangedType = pPhantom;
    m_TypeChangedEvent.Broadcast(msg);
  }

  return pPhantom;
}

bool ezReflectedTypeManager::UnregisterType(const ezRTTI* pRtti)
{
  ezPhantomRTTI* pPhantom = nullptr;
  m_NameToPhantom.TryGetValue(pRtti->GetTypeName(), pPhantom);

  if (pPhantom == nullptr)
    return false;

  {
    ezReflectedTypeChange msg;
    msg.m_pChangedType = pPhantom;
    m_TypeRemovedEvent.Broadcast(msg);
  }

  m_NameToPhantom.Remove(pPhantom->GetTypeName());

  EZ_DEFAULT_DELETE(pPhantom);
  return true;
}

////////////////////////////////////////////////////////////////////////
// ezReflectedTypeManager private functions
////////////////////////////////////////////////////////////////////////

void ezReflectedTypeManager::Startup()
{

}


void ezReflectedTypeManager::Shutdown()
{
  while (!m_NameToPhantom.IsEmpty())
  {
    UnregisterType(m_NameToPhantom.GetIterator().Value());
  }

  EZ_ASSERT_DEV(m_NameToPhantom.IsEmpty(), "ezReflectedTypeManager::Shutdown: Removal of types failed!");
}
