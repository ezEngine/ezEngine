#include <Foundation/PCH.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Configuration/Plugin.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezSubSystem);

bool ezStartup::s_bPrintAllSubSystems = true;
ezStartupStage::Enum ezStartup::s_CurrentState = ezStartupStage::None;

void ezStartup::PrintAllSubsystems()
{
  EZ_LOG_BLOCK("Available Subsystems");

  ezSubSystem* pSub = ezSubSystem::GetFirstInstance();

  while (pSub)
  {
    ezLog::Info("Subsystem: '%s::%s'", pSub->GetGroupName(), pSub->GetSubSystemName());

    if (pSub->GetDependency(0) == nullptr)
      ezLog::Info("  <no dependencies>");
    else
    {
      for (ezInt32 i = 0; pSub->GetDependency(i) != nullptr; ++i)
        ezLog::Info("  -> '%s'", pSub->GetDependency(i));
    }

    ezLog::Info("");

    pSub = pSub->GetNextInstance();
  }
}

void ezStartup::AssignSubSystemPlugin(const char* szPluginName)
{
  // iterates over all existing subsystems and finds those that have no plugin name yet
  // assigns the given name to them

  ezSubSystem* pSub = ezSubSystem::GetFirstInstance();

  while (pSub)
  {
    if (pSub->m_szPluginName == nullptr)
      pSub->m_szPluginName = szPluginName;

    pSub = pSub->GetNextInstance();
  }
}

void ezStartup::PluginEventHandler(const ezPlugin::PluginEvent& EventData)
{
  switch (EventData.m_EventType)
  {
  case ezPlugin::PluginEvent::BeforeLoading:
    {
      AssignSubSystemPlugin("Static");
    }
    break;

  case ezPlugin::PluginEvent::AfterLoadingBeforeInit:
    {
      if (EventData.m_pPluginObject)
      {
        AssignSubSystemPlugin(EventData.m_pPluginObject->GetPluginName());
      }
    }
    break;

  case ezPlugin::PluginEvent::BeforeUnloading:
    {
      if (EventData.m_pPluginObject)
      {
        ezStartup::UnloadPluginSubSystems(EventData.m_pPluginObject->GetPluginName());
      }
    }
    break;

  case ezPlugin::PluginEvent::AfterPluginChanges:
    {
      ezStartup::ReinitToCurrentState();
    }
    break;
      
  default:
    break;
  }
}

static bool IsGroupName(const char* szName)
{
  ezSubSystem* pSub = ezSubSystem::GetFirstInstance();

  bool bGroup = false;
  bool bSubSystem = false;

  while (pSub)
  {
    if (ezStringUtils::IsEqual(pSub->GetGroupName(), szName))
      bGroup = true;

    if (ezStringUtils::IsEqual(pSub->GetSubSystemName(), szName))
      bSubSystem = true;

    pSub = pSub->GetNextInstance();
  }

  EZ_ASSERT_ALWAYS(!bGroup || !bSubSystem, "There cannot be a SubSystem AND a Group called '%s'.", szName);

  return bGroup;
}

static const char* GetGroupSubSystems(const char* szGroup, ezInt32 iSubSystem)
{
  ezSubSystem* pSub = ezSubSystem::GetFirstInstance();

  while (pSub)
  {
    if (ezStringUtils::IsEqual(pSub->GetGroupName(), szGroup))
    {
      if (iSubSystem == 0)
        return pSub->GetSubSystemName();

      --iSubSystem;
    }

    pSub = pSub->GetNextInstance();
  }

  return nullptr;
}

void ezStartup::ComputeOrder(ezDeque<ezSubSystem*>& Order)
{
  Order.Clear();
  ezSet<ezString> sSystemsInited;

  bool bCouldInitAny = true;

  while (bCouldInitAny)
  {
    bCouldInitAny = false;

    ezSubSystem* pSub = ezSubSystem::GetFirstInstance();

    while (pSub)
    {
      if (!sSystemsInited.Find(pSub->GetSubSystemName()).IsValid())
      {
        bool bAllDependsFulfilled = true;
        ezInt32 iDep = 0;

        while (pSub->GetDependency(iDep) != nullptr)
        {
          if (IsGroupName(pSub->GetDependency(iDep)))
          {
            ezInt32 iSubSystemIndex = 0;
            const char* szNextSubSystem = GetGroupSubSystems(pSub->GetDependency(iDep), iSubSystemIndex);
            while (szNextSubSystem)
            {
              if (!sSystemsInited.Find(szNextSubSystem).IsValid())
              {
                bAllDependsFulfilled = false;
                break;
              }

              ++iSubSystemIndex;
              szNextSubSystem = GetGroupSubSystems(pSub->GetDependency(iDep), iSubSystemIndex);
            }
          }
          else
          {
            if (!sSystemsInited.Find(pSub->GetDependency(iDep)).IsValid())
            {
              bAllDependsFulfilled = false;
              break;
            }
          }

          ++iDep;
        }

        if (bAllDependsFulfilled)
        {
          bCouldInitAny = true;
          Order.PushBack(pSub);
          sSystemsInited.Insert(pSub->GetSubSystemName());
        }
      }

      pSub = pSub->GetNextInstance();
    }
  }
}

void ezStartup::Startup(ezStartupStage::Enum stage)
{
  if (stage == ezStartupStage::Base)
  {
    ezFoundation::Initialize();
  }

  const char* szStartup[] = { "Startup Base", "Startup Core", "Startup Engine" };

  if (stage == ezStartupStage::Core)
  {
    Startup(ezStartupStage::Base);

    ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_STARTUP_CORE_BEGIN);

    if (s_bPrintAllSubSystems)
    {
      s_bPrintAllSubSystems = false;
      PrintAllSubsystems();
    }
  }

  if (stage == ezStartupStage::Engine)
  {
    Startup(ezStartupStage::Core);

    ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_STARTUP_ENGINE_BEGIN);
  }

  EZ_LOG_BLOCK(szStartup[stage]);

  ezDeque<ezSubSystem*> Order;
  ComputeOrder(Order);

  for (ezUInt32 i = 0; i < Order.GetCount(); ++i)
  {
    if (!Order[i]->m_bStartupDone[stage])
    {
      ezLog::Info("Starting Sub-System '%s' from Group '%s'", Order[i]->GetSubSystemName(), Order[i]->GetGroupName());

      switch (stage)
      {
      case ezStartupStage::Base:
        Order[i]->OnBaseStartup();
        break;
      case ezStartupStage::Core:
        Order[i]->OnCoreStartup();
        break;
      case ezStartupStage::Engine:
        Order[i]->OnEngineStartup();
        break;
          
      default:
        break;
      }

      Order[i]->m_bStartupDone[stage] = true;
    }
  }

  // ezThreadUtils::IsMainThread can only be called after the Base startup is through, so this is they earliest possible position for the check
  EZ_ASSERT_DEV(ezThreadUtils::IsMainThread(), "The startup system can only be called from the Main Thread.");

  // now everything should be started
  {
    EZ_LOG_BLOCK("Failed SubSystems");

    ezSet<ezString> sSystemsFound;

    ezSubSystem* pSub = ezSubSystem::GetFirstInstance();

    while (pSub)
    {
      sSystemsFound.Insert(pSub->GetSubSystemName());
      pSub = pSub->GetNextInstance();
    }

    pSub = ezSubSystem::GetFirstInstance();

    while (pSub)
    {
      if (!pSub->m_bStartupDone[stage])
      {
        ezInt32 iDep = 0;

        while (pSub->GetDependency(iDep) != nullptr)
        {
          if (!sSystemsFound.Find(pSub->GetDependency(iDep)).IsValid())
          {
            ezLog::Error("SubSystem '%s::%s' could not be started because dependency '%s' is unknown.", pSub->GetGroupName(), pSub->GetSubSystemName(), pSub->GetDependency(iDep));
          }
          else
          {
            ezLog::Error("SubSystem '%s::%s' could not be started because dependency '%s' has not been initialized.", pSub->GetGroupName(), pSub->GetSubSystemName(), pSub->GetDependency(iDep));
          }

          ++iDep;
        }
      }

      pSub = pSub->GetNextInstance();
    }
  }

  switch (stage)
  {
  case ezStartupStage::Base:
    break;
  case ezStartupStage::Core:
    ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_STARTUP_CORE_END);
    break;
  case ezStartupStage::Engine:
    ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_STARTUP_ENGINE_END);
    break;
      
  default:
    break;
  }

  if (s_CurrentState == ezStartupStage::None)
  {
    ezPlugin::s_PluginEvents.AddEventHandler(PluginEventHandler);
  }

  s_CurrentState = stage;
}

void ezStartup::Shutdown(ezStartupStage::Enum stage)
{
  // without that we cannot function, so make sure it is up and running
  ezFoundation::Initialize();

  {
    const char* szStartup[] = { "Shutdown Base", "Shutdown Core", "Shutdown Engine" };

    if (stage == ezStartupStage::Base)
    {
      Shutdown(ezStartupStage::Core);
    }

    if (stage == ezStartupStage::Core)
    {
      Shutdown(ezStartupStage::Engine);
      s_bPrintAllSubSystems = true;

      ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_SHUTDOWN_CORE_BEGIN);
    }

    if (stage == ezStartupStage::Engine)
    {
      ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_SHUTDOWN_ENGINE_BEGIN);
    }

    EZ_LOG_BLOCK(szStartup[stage]);

    ezDeque<ezSubSystem*> Order;
    ComputeOrder(Order);

    for (ezInt32 i = (ezInt32) Order.GetCount() - 1; i >= 0; --i)
    {
      if (Order[i]->m_bStartupDone[stage])
      {
        ezLog::Info("Shutting down Sub-System '%s::%s'", Order[i]->GetGroupName(), Order[i]->GetSubSystemName());

        switch (stage)
        {
        case ezStartupStage::Core:
          Order[i]->OnCoreShutdown();
          break;
            
        case ezStartupStage::Engine:
          Order[i]->OnEngineShutdown();
          break;
            
        default:
          break;
        }

        Order[i]->m_bStartupDone[stage] = false;
      }
    }
  }

  switch (stage)
  {
  case ezStartupStage::Core:
    ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_SHUTDOWN_CORE_END);
    break;
      
  case ezStartupStage::Engine:
    ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_SHUTDOWN_ENGINE_END);
    break;
      
  default:
    break;
  }

  if (s_CurrentState != ezStartupStage::None)
  {
    s_CurrentState = (ezStartupStage::Enum) (((ezInt32) stage) - 1);

    if (s_CurrentState == ezStartupStage::None)
    {
      ezPlugin::s_PluginEvents.RemoveEventHandler(PluginEventHandler);
    }
  }
}

bool ezStartup::HasDependencyOnPlugin(ezSubSystem* pSubSystem, const char* szModule)
{
  if (ezStringUtils::IsEqual(pSubSystem->m_szPluginName, szModule))
    return true;

  for (ezUInt32 i = 0; pSubSystem->GetDependency(i) != nullptr; ++i)
  {
    ezSubSystem* pSub = ezSubSystem::GetFirstInstance();
    while (pSub)
    {
      if (ezStringUtils::IsEqual(pSub->GetSubSystemName(), pSubSystem->GetDependency(i)))
      {
        if (HasDependencyOnPlugin(pSub, szModule))
          return true;

        break;
      }

      pSub = pSub->GetNextInstance();
    }
  }

  return false;
}

void ezStartup::UnloadPluginSubSystems(const char* szPluginName)
{
  EZ_LOG_BLOCK("Unloading Plugin SubSystems", szPluginName);
  ezLog::Dev("Plugin to unload: '%s'", szPluginName);

  ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_UNLOAD_PLUGIN_BEGIN, ezVariant(szPluginName));

  ezDeque<ezSubSystem*> Order;
  ComputeOrder(Order);

  for (ezInt32 i = (ezInt32) Order.GetCount() - 1; i >= 0; --i)
  {
    if (Order[i]->m_bStartupDone[ezStartupStage::Engine] && HasDependencyOnPlugin(Order[i], szPluginName))
    {
      ezLog::Info("Engine shutdown of SubSystem '%s::%s', because it depends on Plugin '%s'.", Order[i]->GetGroupName(), Order[i]->GetSubSystemName(), szPluginName);
      Order[i]->OnEngineShutdown();
      Order[i]->m_bStartupDone[ezStartupStage::Engine] = false;
    }
  }

  for (ezInt32 i = (ezInt32) Order.GetCount() - 1; i >= 0; --i)
  {
    if (Order[i]->m_bStartupDone[ezStartupStage::Core] && HasDependencyOnPlugin(Order[i], szPluginName))
    {
      ezLog::Info("Core shutdown of SubSystem '%s::%s', because it depends on Plugin '%s'.", Order[i]->GetGroupName(), Order[i]->GetSubSystemName(), szPluginName);
      Order[i]->OnCoreShutdown();
      Order[i]->m_bStartupDone[ezStartupStage::Core] = false;
    }
  }


  ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_UNLOAD_PLUGIN_END, ezVariant(szPluginName));
}

void ezStartup::ReinitToCurrentState()
{
  if (s_CurrentState != ezStartupStage::None)
    Startup(s_CurrentState);
}




EZ_STATICLINK_FILE(Foundation, Foundation_Configuration_Implementation_Startup);

