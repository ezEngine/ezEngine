#include <ToolsFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/Reflection/PhantomRtti.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <Foundation/Profiling/Profiling.h>

ezEvent<const ezPhantomRttiManagerEvent&> ezPhantomRttiManager::s_Events;

ezHashTable<const char*, ezPhantomRTTI*> ezPhantomRttiManager::m_NameToPhantom;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, ReflectedTypeManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezPhantomRttiManager::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezPhantomRttiManager::Shutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

////////////////////////////////////////////////////////////////////////
// ezPhantomRttiManager public functions
////////////////////////////////////////////////////////////////////////

const ezRTTI* ezPhantomRttiManager::RegisterType(ezReflectedTypeDescriptor& desc)
{
  EZ_PROFILE_SCOPE("RegisterType");
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
    pPhantom = EZ_DEFAULT_NEW(ezPhantomRTTI, desc.m_sTypeName.GetData(), ezRTTI::FindTypeByName(desc.m_sParentTypeName), desc.m_uiTypeSize,
                              desc.m_uiTypeVersion, ezVariantType::Invalid, desc.m_Flags, desc.m_sPluginName.GetData());

    pPhantom->SetProperties(desc.m_Properties);
    pPhantom->SetAttributes(desc.m_Attributes);

    m_NameToPhantom[pPhantom->GetTypeName()] = pPhantom;

    ezPhantomRttiManagerEvent msg;
    msg.m_pChangedType = pPhantom;
    msg.m_Type = ezPhantomRttiManagerEvent::Type::TypeAdded;
    s_Events.Broadcast(msg, 1); /// \todo Had to increase the recursion depth to allow registering phantom types that are based on actual
                                /// types coming from the engine process
  }
  else
  {
    pPhantom->UpdateType(desc);

    ezPhantomRttiManagerEvent msg;
    msg.m_pChangedType = pPhantom;
    msg.m_Type = ezPhantomRttiManagerEvent::Type::TypeChanged;
    s_Events.Broadcast(msg, 1);
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
    ezPhantomRttiManagerEvent msg;
    msg.m_pChangedType = pPhantom;
    msg.m_Type = ezPhantomRttiManagerEvent::Type::TypeRemoved;
    s_Events.Broadcast(msg);
  }

  m_NameToPhantom.Remove(pPhantom->GetTypeName());

  EZ_DEFAULT_DELETE(pPhantom);
  return true;
}

////////////////////////////////////////////////////////////////////////
// ezPhantomRttiManager private functions
////////////////////////////////////////////////////////////////////////

void ezPhantomRttiManager::PluginEventHandler(const ezPlugin::PluginEvent& e)
{
  if (e.m_EventType == ezPlugin::PluginEvent::Type::BeforeUnloading)
  {
    while (!m_NameToPhantom.IsEmpty())
    {
      UnregisterType(m_NameToPhantom.GetIterator().Value());
    }

    EZ_ASSERT_DEV(m_NameToPhantom.IsEmpty(), "ezPhantomRttiManager::Shutdown: Removal of types failed!");
  }
}

void ezPhantomRttiManager::Startup()
{
  ezPlugin::s_PluginEvents.AddEventHandler(&ezPhantomRttiManager::PluginEventHandler);
}


void ezPhantomRttiManager::Shutdown()
{
  ezPlugin::s_PluginEvents.RemoveEventHandler(&ezPhantomRttiManager::PluginEventHandler);

  while (!m_NameToPhantom.IsEmpty())
  {
    UnregisterType(m_NameToPhantom.GetIterator().Value());
  }

  EZ_ASSERT_DEV(m_NameToPhantom.IsEmpty(), "ezPhantomRttiManager::Shutdown: Removal of types failed!");
}
