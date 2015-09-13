#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Reflection/PhantomRtti.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Configuration/Startup.h>

ezEvent<const ezPhantomRttiManager::Event&> ezPhantomRttiManager::m_Events;

ezHashTable<const char*, ezPhantomRTTI*> ezPhantomRttiManager::m_NameToPhantom;

EZ_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, ReflectedTypeManager)

BEGIN_SUBSYSTEM_DEPENDENCIES
"Core"
END_SUBSYSTEM_DEPENDENCIES

ON_CORE_STARTUP
{
  ezPhantomRttiManager::Startup();
}

ON_CORE_SHUTDOWN
{
  ezPhantomRttiManager::Shutdown();
}

EZ_END_SUBSYSTEM_DECLARATION

////////////////////////////////////////////////////////////////////////
// ezPhantomRttiManager public functions
////////////////////////////////////////////////////////////////////////

const ezRTTI* ezPhantomRttiManager::RegisterType(const ezReflectedTypeDescriptor& desc)
{
  ezRTTI* pType = ezRTTI::FindTypeByName(desc.m_sTypeName);
  ezPhantomRTTI* pPhantom = nullptr;
  m_NameToPhantom.TryGetValue(desc.m_sTypeName, pPhantom);

  // concrete type !
  if (pPhantom == nullptr && pType != nullptr)
  {
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

    Event msg;
    msg.m_pChangedType = pPhantom;
    msg.m_Type = Event::Type::TypeAdded;
    m_Events.Broadcast(msg);
  }
  else
  {
    pPhantom->UpdateType(desc);

    Event msg;
    msg.m_pChangedType = pPhantom;
    msg.m_Type = Event::Type::TypeChanged;
    m_Events.Broadcast(msg);
  }

  return pPhantom;
}

bool ezPhantomRttiManager::UnregisterType(const ezRTTI* pRtti)
{
  ezPhantomRTTI* pPhantom = nullptr;
  m_NameToPhantom.TryGetValue(pRtti->GetTypeName(), pPhantom);

  if (pPhantom == nullptr)
    return false;

  {
    Event msg;
    msg.m_pChangedType = pPhantom;
    msg.m_Type = Event::Type::TypeRemoved;
    m_Events.Broadcast(msg);
  }

  m_NameToPhantom.Remove(pPhantom->GetTypeName());

  EZ_DEFAULT_DELETE(pPhantom);
  return true;
}

////////////////////////////////////////////////////////////////////////
// ezPhantomRttiManager private functions
////////////////////////////////////////////////////////////////////////

void ezPhantomRttiManager::Startup()
{

}


void ezPhantomRttiManager::Shutdown()
{
  while (!m_NameToPhantom.IsEmpty())
  {
    UnregisterType(m_NameToPhantom.GetIterator().Value());
  }

  EZ_ASSERT_DEV(m_NameToPhantom.IsEmpty(), "ezPhantomRttiManager::Shutdown: Removal of types failed!");
}
