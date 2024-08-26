#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/AllocatorWithPolicy.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/Reflection/PhantomRtti.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

ezCopyOnBroadcastEvent<const ezPhantomRttiManagerEvent&> ezPhantomRttiManager::s_Events;

ezHashTable<ezStringView, ezPhantomRTTI*> ezPhantomRttiManager::s_NameToPhantom;

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

const ezRTTI* ezPhantomRttiManager::RegisterType(ezReflectedTypeDescriptor& ref_desc)
{
  EZ_PROFILE_SCOPE("RegisterType");
  const ezRTTI* pType = ezRTTI::FindTypeByName(ref_desc.m_sTypeName);
  ezPhantomRTTI* pPhantom = nullptr;
  s_NameToPhantom.TryGetValue(ref_desc.m_sTypeName, pPhantom);

  // concrete type !
  if (pPhantom == nullptr && pType != nullptr)
  {
    return pType;
  }

  if (pPhantom != nullptr && pPhantom->IsEqualToDescriptor(ref_desc))
    return pPhantom;

  if (pPhantom == nullptr)
  {
    pPhantom = EZ_DEFAULT_NEW(ezPhantomRTTI, ref_desc.m_sTypeName.GetData(), ezRTTI::FindTypeByName(ref_desc.m_sParentTypeName), 0,
      ref_desc.m_uiTypeVersion, ezVariantType::Invalid, ref_desc.m_Flags, ref_desc.m_sPluginName.GetData());

    pPhantom->SetProperties(ref_desc.m_Properties);
    pPhantom->SetAttributes(ref_desc.m_Attributes);
    pPhantom->SetFunctions(ref_desc.m_Functions);
    pPhantom->SetupParentHierarchy();

    s_NameToPhantom[pPhantom->GetTypeName()] = pPhantom;

    ezPhantomRttiManagerEvent msg;
    msg.m_pChangedType = pPhantom;
    msg.m_Type = ezPhantomRttiManagerEvent::Type::TypeAdded;
    s_Events.Broadcast(msg, 1); /// \todo Had to increase the recursion depth to allow registering phantom types that are based on actual
                                /// types coming from the engine process
  }
  else
  {
    pPhantom->UpdateType(ref_desc);

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
  s_NameToPhantom.TryGetValue(pRtti->GetTypeName(), pPhantom);

  if (pPhantom == nullptr)
    return false;

  {
    ezPhantomRttiManagerEvent msg;
    msg.m_pChangedType = pPhantom;
    msg.m_Type = ezPhantomRttiManagerEvent::Type::TypeRemoved;
    s_Events.Broadcast(msg);
  }

  s_NameToPhantom.Remove(pPhantom->GetTypeName());

  EZ_DEFAULT_DELETE(pPhantom);
  return true;
}

////////////////////////////////////////////////////////////////////////
// ezPhantomRttiManager private functions
////////////////////////////////////////////////////////////////////////

void ezPhantomRttiManager::PluginEventHandler(const ezPluginEvent& e)
{
  if (e.m_EventType == ezPluginEvent::Type::BeforeUnloading)
  {
    while (!s_NameToPhantom.IsEmpty())
    {
      UnregisterType(s_NameToPhantom.GetIterator().Value());
    }

    EZ_ASSERT_DEV(s_NameToPhantom.IsEmpty(), "ezPhantomRttiManager::Shutdown: Removal of types failed!");
  }
}

void ezPhantomRttiManager::Startup()
{
  ezPlugin::Events().AddEventHandler(&ezPhantomRttiManager::PluginEventHandler);
}


void ezPhantomRttiManager::Shutdown()
{
  ezPlugin::Events().RemoveEventHandler(&ezPhantomRttiManager::PluginEventHandler);

  while (!s_NameToPhantom.IsEmpty())
  {
    UnregisterType(s_NameToPhantom.GetIterator().Value());
  }

  EZ_ASSERT_DEV(s_NameToPhantom.IsEmpty(), "ezPhantomRttiManager::Shutdown: Removal of types failed!");
}
