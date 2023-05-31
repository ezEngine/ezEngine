#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/ThreadUtils.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezSubSystem);

bool ezStartup::s_bPrintAllSubSystems = true;
ezStartupStage::Enum ezStartup::s_CurrentState = ezStartupStage::None;
ezDynamicArray<ezStringView> ezStartup::s_ApplicationTags;


void ezStartup::AddApplicationTag(ezStringView sTag)
{
  s_ApplicationTags.PushBack(sTag);
}

bool ezStartup::HasApplicationTag(ezStringView sTag)
{
  for (ezUInt32 i = 0; i < s_ApplicationTags.GetCount(); ++i)
  {
    if (s_ApplicationTags[i].IsEqual_NoCase(sTag))
      return true;
  }

  return false;
}

void ezStartup::PrintAllSubsystems()
{
  EZ_LOG_BLOCK("Available Subsystems");

  ezSubSystem* pSub = ezSubSystem::GetFirstInstance();

  while (pSub)
  {
    ezLog::Debug("Subsystem: '{0}::{1}'", pSub->GetGroupName(), pSub->GetSubSystemName());

    if (pSub->GetDependency(0) == nullptr)
      ezLog::Debug("  <no dependencies>");
    else
    {
      for (ezInt32 i = 0; pSub->GetDependency(i) != nullptr; ++i)
        ezLog::Debug("  depends on '{0}'", pSub->GetDependency(i));
    }

    ezLog::Debug("");

    pSub = pSub->GetNextInstance();
  }
}

void ezStartup::AssignSubSystemPlugin(ezStringView sPluginName)
{
  // iterates over all existing subsystems and finds those that have no plugin name yet
  // assigns the given name to them

  ezSubSystem* pSub = ezSubSystem::GetFirstInstance();

  while (pSub)
  {
    if (pSub->m_sPluginName.IsEmpty())
    {
      pSub->m_sPluginName = sPluginName;
    }

    pSub = pSub->GetNextInstance();
  }
}

void ezStartup::PluginEventHandler(const ezPluginEvent& EventData)
{
  switch (EventData.m_EventType)
  {
    case ezPluginEvent::BeforeLoading:
    {
      AssignSubSystemPlugin("Static");
    }
    break;

    case ezPluginEvent::AfterLoadingBeforeInit:
    {
      AssignSubSystemPlugin(EventData.m_szPluginBinary);
    }
    break;

    case ezPluginEvent::StartupShutdown:
    {
      ezStartup::UnloadPluginSubSystems(EventData.m_szPluginBinary);
    }
    break;

    case ezPluginEvent::AfterPluginChanges:
    {
      ezStartup::ReinitToCurrentState();
    }
    break;

    default:
      break;
  }
}

static bool IsGroupName(ezStringView sName)
{
  ezSubSystem* pSub = ezSubSystem::GetFirstInstance();

  bool bGroup = false;
  bool bSubSystem = false;

  while (pSub)
  {
    if (pSub->GetGroupName() == sName)
      bGroup = true;

    if (pSub->GetSubSystemName() == sName)
      bSubSystem = true;

    pSub = pSub->GetNextInstance();
  }

  EZ_ASSERT_ALWAYS(!bGroup || !bSubSystem, "There cannot be a SubSystem AND a Group called '{0}'.", sName);

  return bGroup;
}

static ezStringView GetGroupSubSystems(ezStringView sGroup, ezInt32 iSubSystem)
{
  ezSubSystem* pSub = ezSubSystem::GetFirstInstance();

  while (pSub)
  {
    if (pSub->GetGroupName() == sGroup)
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
            ezStringView sNextSubSystem = GetGroupSubSystems(pSub->GetDependency(iDep), iSubSystemIndex);
            while (sNextSubSystem.IsValid())
            {
              if (!sSystemsInited.Find(sNextSubSystem).IsValid())
              {
                bAllDependsFulfilled = false;
                break;
              }

              ++iSubSystemIndex;
              sNextSubSystem = GetGroupSubSystems(pSub->GetDependency(iDep), iSubSystemIndex);
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
  if (stage == ezStartupStage::BaseSystems)
  {
    ezFoundation::Initialize();
  }

  const char* szStartup[] = {"Startup Base", "Startup Core", "Startup Engine"};

  if (stage == ezStartupStage::CoreSystems)
  {
    Startup(ezStartupStage::BaseSystems);

    ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_STARTUP_CORESYSTEMS_BEGIN);

    if (s_bPrintAllSubSystems)
    {
      s_bPrintAllSubSystems = false;
      PrintAllSubsystems();
    }
  }

  if (stage == ezStartupStage::HighLevelSystems)
  {
    Startup(ezStartupStage::CoreSystems);

    ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_STARTUP_HIGHLEVELSYSTEMS_BEGIN);
  }

  EZ_LOG_BLOCK(szStartup[stage]);

  ezDeque<ezSubSystem*> Order;
  ComputeOrder(Order);

  for (ezUInt32 i = 0; i < Order.GetCount(); ++i)
  {
    if (!Order[i]->m_bStartupDone[stage])
    {
      Order[i]->m_bStartupDone[stage] = true;

      switch (stage)
      {
        case ezStartupStage::BaseSystems:
          ezLog::Debug("Executing 'Base' startup for sub-system '{1}::{0}'", Order[i]->GetSubSystemName(), Order[i]->GetGroupName());
          Order[i]->OnBaseSystemsStartup();
          break;
        case ezStartupStage::CoreSystems:
          ezLog::Debug("Executing 'Core' startup for sub-system '{1}::{0}'", Order[i]->GetSubSystemName(), Order[i]->GetGroupName());
          Order[i]->OnCoreSystemsStartup();
          break;
        case ezStartupStage::HighLevelSystems:
          ezLog::Debug("Executing 'Engine' startup for sub-system '{1}::{0}'", Order[i]->GetSubSystemName(), Order[i]->GetGroupName());
          Order[i]->OnHighLevelSystemsStartup();
          break;

        default:
          break;
      }
    }
  }

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
            ezLog::Error("SubSystem '{0}::{1}' could not be started because dependency '{2}' is unknown.", pSub->GetGroupName(),
              pSub->GetSubSystemName(), pSub->GetDependency(iDep));
          }
          else
          {
            ezLog::Error("SubSystem '{0}::{1}' could not be started because dependency '{2}' has not been initialized.", pSub->GetGroupName(),
              pSub->GetSubSystemName(), pSub->GetDependency(iDep));
          }

          ++iDep;
        }
      }

      pSub = pSub->GetNextInstance();
    }
  }

  switch (stage)
  {
    case ezStartupStage::BaseSystems:
      break;
    case ezStartupStage::CoreSystems:
      ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_STARTUP_CORESYSTEMS_END);
      break;
    case ezStartupStage::HighLevelSystems:
      ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_STARTUP_HIGHLEVELSYSTEMS_END);
      break;

    default:
      break;
  }

  if (s_CurrentState == ezStartupStage::None)
  {
    ezPlugin::Events().AddEventHandler(PluginEventHandler);
  }

  s_CurrentState = stage;
}

void ezStartup::Shutdown(ezStartupStage::Enum stage)
{
  // without that we cannot function, so make sure it is up and running
  ezFoundation::Initialize();

  {
    const char* szStartup[] = {"Shutdown Base", "Shutdown Core", "Shutdown Engine"};

    if (stage == ezStartupStage::BaseSystems)
    {
      Shutdown(ezStartupStage::CoreSystems);
    }

    if (stage == ezStartupStage::CoreSystems)
    {
      Shutdown(ezStartupStage::HighLevelSystems);
      s_bPrintAllSubSystems = true;

      ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_SHUTDOWN_CORESYSTEMS_BEGIN);
    }

    if (stage == ezStartupStage::HighLevelSystems)
    {
      ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_SHUTDOWN_HIGHLEVELSYSTEMS_BEGIN);
    }

    EZ_LOG_BLOCK(szStartup[stage]);

    ezDeque<ezSubSystem*> Order;
    ComputeOrder(Order);

    for (ezInt32 i = (ezInt32)Order.GetCount() - 1; i >= 0; --i)
    {
      if (Order[i]->m_bStartupDone[stage])
      {
        switch (stage)
        {
          case ezStartupStage::CoreSystems:
            ezLog::Debug("Executing 'Core' shutdown of sub-system '{0}::{1}'", Order[i]->GetGroupName(), Order[i]->GetSubSystemName());
            Order[i]->OnCoreSystemsShutdown();
            break;

          case ezStartupStage::HighLevelSystems:
            ezLog::Debug("Executing 'Engine' shutdown of sub-system '{0}::{1}'", Order[i]->GetGroupName(), Order[i]->GetSubSystemName());
            Order[i]->OnHighLevelSystemsShutdown();
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
    case ezStartupStage::CoreSystems:
      ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_SHUTDOWN_CORESYSTEMS_END);
      break;

    case ezStartupStage::HighLevelSystems:
      ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_SHUTDOWN_HIGHLEVELSYSTEMS_END);
      break;

    default:
      break;
  }

  if (s_CurrentState != ezStartupStage::None)
  {
    s_CurrentState = (ezStartupStage::Enum)(((ezInt32)stage) - 1);

    if (s_CurrentState == ezStartupStage::None)
    {
      ezPlugin::Events().RemoveEventHandler(PluginEventHandler);
    }
  }
}

bool ezStartup::HasDependencyOnPlugin(ezSubSystem* pSubSystem, ezStringView sModule)
{
  if (pSubSystem->m_sPluginName == sModule)
    return true;

  for (ezUInt32 i = 0; pSubSystem->GetDependency(i) != nullptr; ++i)
  {
    ezSubSystem* pSub = ezSubSystem::GetFirstInstance();
    while (pSub)
    {
      if (pSub->GetSubSystemName() == pSubSystem->GetDependency(i))
      {
        if (HasDependencyOnPlugin(pSub, sModule))
          return true;

        break;
      }

      pSub = pSub->GetNextInstance();
    }
  }

  return false;
}

void ezStartup::UnloadPluginSubSystems(ezStringView sPluginName)
{
  EZ_LOG_BLOCK("Unloading Plugin SubSystems", sPluginName);
  ezLog::Dev("Plugin to unload: '{0}'", sPluginName);

  ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_UNLOAD_PLUGIN_BEGIN, ezVariant(sPluginName));

  ezDeque<ezSubSystem*> Order;
  ComputeOrder(Order);

  for (ezInt32 i = (ezInt32)Order.GetCount() - 1; i >= 0; --i)
  {
    if (Order[i]->m_bStartupDone[ezStartupStage::HighLevelSystems] && HasDependencyOnPlugin(Order[i], sPluginName))
    {
      ezLog::Info("Engine shutdown of SubSystem '{0}::{1}', because it depends on Plugin '{2}'.", Order[i]->GetGroupName(), Order[i]->GetSubSystemName(), sPluginName);
      Order[i]->OnHighLevelSystemsShutdown();
      Order[i]->m_bStartupDone[ezStartupStage::HighLevelSystems] = false;
    }
  }

  for (ezInt32 i = (ezInt32)Order.GetCount() - 1; i >= 0; --i)
  {
    if (Order[i]->m_bStartupDone[ezStartupStage::CoreSystems] && HasDependencyOnPlugin(Order[i], sPluginName))
    {
      ezLog::Info("Core shutdown of SubSystem '{0}::{1}', because it depends on Plugin '{2}'.", Order[i]->GetGroupName(), Order[i]->GetSubSystemName(), sPluginName);
      Order[i]->OnCoreSystemsShutdown();
      Order[i]->m_bStartupDone[ezStartupStage::CoreSystems] = false;
    }
  }


  ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_UNLOAD_PLUGIN_END, ezVariant(sPluginName));
}

void ezStartup::ReinitToCurrentState()
{
  if (s_CurrentState != ezStartupStage::None)
    Startup(s_CurrentState);
}



EZ_STATICLINK_FILE(Foundation, Foundation_Configuration_Implementation_Startup);
