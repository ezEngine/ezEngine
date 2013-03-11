#include <Foundation/PCH.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Threading/ThreadUtils.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezSubSystemDeclarationBase);

bool ezStartup::s_bPrintAllSubSystems = true;

void ezStartup::PrintAllSubsystems()
{
  EZ_LOG_BLOCK("Available Subsystems");

  ezSubSystemDeclarationBase* pSub = ezSubSystemDeclarationBase::GetFirstInstance();

  while(pSub)
  {
    ezLog::Info("Subsystem: '%s' in Module '%s'", pSub->GetSubSystemName(), pSub->GetModuleName());

    if (pSub->GetDependency(0) == NULL)
      ezLog::Info("  <no dependencies>");
    else
    {
      for (ezInt32 i = 0; pSub->GetDependency(i) != NULL; ++i)
        ezLog::Info("  -> '%s'", pSub->GetDependency(i));
    }

    ezLog::Info("");

    pSub = pSub->GetNextInstance();
  }
}

void ezStartup::ComputeOrder(ezDeque<ezSubSystemDeclarationBase*>& Order)
{
  Order.Clear();
  ezSet<ezString> sSystemsInited;

  bool bCouldInitAny = true;

  while(bCouldInitAny)
  {
    bCouldInitAny = false;

    ezSubSystemDeclarationBase* pSub = ezSubSystemDeclarationBase::GetFirstInstance();

    while(pSub)
    {
      if (!sSystemsInited.Find(pSub->GetSubSystemName()).IsValid())
      {
        bool bAllDependsFullfilled = true;
        ezInt32 iDep = 0;

        while(pSub->GetDependency(iDep) != NULL)
        {
          if (!sSystemsInited.Find(pSub->GetDependency(iDep)).IsValid())
          {
            bAllDependsFullfilled = false;
            break;
          }

          ++iDep;
        }

        if (bAllDependsFullfilled)
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

  ezDeque<ezSubSystemDeclarationBase*> Order;
  ComputeOrder(Order);

  for (ezUInt32 i = 0; i < Order.GetCount(); ++i)
  {
    if (!Order[i]->m_bStartupDone[stage])
    {
      ezLog::Info("Starting Sub-System '%s' from Module '%s'", Order[i]->GetSubSystemName(), Order[i]->GetModuleName());

      switch(stage)
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
      }

      Order[i]->m_bStartupDone[stage] = true;
    }
  }

  // ezThreadUtils::IsMainThread can only be called after the Base startup is through, so this is they earliest possible position for the check
  EZ_ASSERT(ezThreadUtils::IsMainThread(), "The startup system can only be called from the Main Thread.");

  // now everything should be started
  {
    EZ_LOG_BLOCK("Failed SubSystems");

    ezSet<ezString> sSystemsFound;

    ezSubSystemDeclarationBase* pSub = ezSubSystemDeclarationBase::GetFirstInstance();

    while(pSub)
    {
      sSystemsFound.Insert(pSub->GetSubSystemName());
      pSub = pSub->GetNextInstance();
    }

    pSub = ezSubSystemDeclarationBase::GetFirstInstance();

    while(pSub)
    {
      if (!pSub->m_bStartupDone[stage])
      {
        ezInt32 iDep = 0;

        while(pSub->GetDependency(iDep) != NULL)
        {
          if (!sSystemsFound.Find(pSub->GetDependency(iDep)).IsValid())
          {
            ezLog::Error("SubSystem '%s' from Module '%s' could not be started because dependency '%s' is unknown.", pSub->GetSubSystemName(), pSub->GetModuleName(), pSub->GetDependency(iDep));
          }
          else
          {
            ezLog::Error("SubSystem '%s' from Module '%s' could not be started because dependency '%s' has not been initialized.", pSub->GetSubSystemName(), pSub->GetModuleName(), pSub->GetDependency(iDep));
          }

          ++iDep;
        }
      }

      pSub = pSub->GetNextInstance();
    }
  }

  switch(stage)
  {
  case ezStartupStage::Base:
    break;
  case ezStartupStage::Core:
    ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_STARTUP_CORE_END);
    break;
  case ezStartupStage::Engine:
    ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_STARTUP_ENGINE_END);
    break;
  }
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

    ezDeque<ezSubSystemDeclarationBase*> Order;
    ComputeOrder(Order);

    for (ezInt32 i = (ezInt32) Order.GetCount() - 1; i >= 0; --i)
    {
      if (Order[i]->m_bStartupDone[stage])
      {
        ezLog::Info("Shutting down Sub-System '%s' from Module '%s'", Order[i]->GetSubSystemName(), Order[i]->GetModuleName());

        switch(stage)
        {
        case ezStartupStage::Base:
          Order[i]->OnBaseShutdown();
          break;
        case ezStartupStage::Core:
          Order[i]->OnCoreShutdown();
          break;
        case ezStartupStage::Engine:
          Order[i]->OnEngineShutdown();
          break;
        }

        Order[i]->m_bStartupDone[stage] = false;
      }
    }
  }

  switch(stage)
  {
  case ezStartupStage::Base:
    break;
  case ezStartupStage::Core:
    ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_SHUTDOWN_CORE_END);
    break;
  case ezStartupStage::Engine:
    ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_SHUTDOWN_ENGINE_END);
    break;
  }

  if (stage == ezStartupStage::Base)
  {
    ezFoundation::Shutdown();
  }
}

bool ezStartup::HasDependencyOnModule(ezSubSystemDeclarationBase* pSubSystem, const char* szModule)
{
  if (ezStringUtils::IsEqual(pSubSystem->GetModuleName(), szModule))
    return true;

  for (ezUInt32 i = 0; pSubSystem->GetDependency(i) != NULL; ++i)
  {
    ezSubSystemDeclarationBase* pSub = ezSubSystemDeclarationBase::GetFirstInstance();
    while (pSub)
    {
      if (ezStringUtils::IsEqual(pSub->GetSubSystemName(), pSubSystem->GetDependency(i)))
      {
        if (HasDependencyOnModule(pSub, szModule))
          return true;

        break;
      }

      pSub = pSub->GetNextInstance();
    }
  }

  return false;
}

void ezStartup::UnloadModuleSubSystems(const char* szModuleName)
{
  EZ_LOG_BLOCK("Unloadg Module SubSystems");
  ezLog::Info("Module to unload: '%s'", szModuleName);

  ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_UNLOAD_MODULE_BEGIN, ezVariant(szModuleName));

  ezDeque<ezSubSystemDeclarationBase*> Order;
  ComputeOrder(Order);

  for (ezInt32 i = (ezInt32) Order.GetCount() - 1; i >= 0; --i)
  {
    if (Order[i]->m_bStartupDone[ezStartupStage::Engine] && HasDependencyOnModule(Order[i], szModuleName))
    {
      ezLog::Info("Engine shutdown of SubSystem '%s' of Module '%s', because it depends on Module '%s'.", Order[i]->GetSubSystemName(), Order[i]->GetModuleName(), szModuleName);
      Order[i]->OnEngineShutdown();
      Order[i]->m_bStartupDone[ezStartupStage::Engine] = false;
    }
  }

  for (ezInt32 i = (ezInt32) Order.GetCount() - 1; i >= 0; --i)
  {
    if (Order[i]->m_bStartupDone[ezStartupStage::Engine] && HasDependencyOnModule(Order[i], szModuleName))
    {
      ezLog::Info("Core shutdown of SubSystem '%s' of Module '%s', because it depends on Module '%s'.", Order[i]->GetSubSystemName(), Order[i]->GetModuleName(), szModuleName);
      Order[i]->OnCoreShutdown();
      Order[i]->m_bStartupDone[ezStartupStage::Core] = false;
    }
  }

  ezGlobalEvent::Broadcast(EZ_GLOBALEVENT_UNLOAD_MODULE_END, ezVariant(szModuleName));
}


