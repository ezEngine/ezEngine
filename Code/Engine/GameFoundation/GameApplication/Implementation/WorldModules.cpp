#include <GameFoundation/PCH.h>
#include <Core/World/WorldModule.h>
#include <Core/World/World.h>
#include <GameFoundation/GameApplication/GameApplication.h>


void ezGameApplication::UpdateWorldModules(ezWorld* pWorld)
{
  for (auto& wm : m_Worlds)
  {
    if (wm.m_pWorld == pWorld)
    {
      wm.Update();
      return;
    }
  }

  EZ_REPORT_FAILURE("ezGameApplication::UpdateWorldModules called with a world that was not created through ezGameApplication");
}


void ezGameApplication::ReinitWorldModules(ezWorld* pWorld)
{
  for (auto& wm : m_Worlds)
  {
    if (wm.m_pWorld == pWorld)
    {
      wm.ReinitWorldModules();
      return;
    }
  }

  EZ_REPORT_FAILURE("ezGameApplication::ReinitWorldModules called with a world that was not created through ezGameApplication");
}


void ezGameApplication::WorldData::Update()
{
  // mark world for write here ?

  for (auto pModule : m_WorldModules)
  {
    pModule->Update();
  }
}

void ezGameApplication::WorldData::ReinitWorldModules()
{
  for (auto pModule : m_WorldModules)
  {
    pModule->Reinit();
  }
}

void ezGameApplication::WorldData::CreateWorldModules()
{
  ezRTTI* pRtti = ezRTTI::GetFirstInstance();

  while (pRtti)
  {
    if (pRtti->IsDerivedFrom<ezWorldModule>())
    {
      ezRTTIAllocator* pAlloc = pRtti->GetAllocator();

      if (pAlloc && pAlloc->CanAllocate())
      {
        ezWorldModule* pModule = static_cast<ezWorldModule*>(pAlloc->Allocate());

        m_WorldModules.PushBack(pModule);
      }
    }

    pRtti = pRtti->GetNextInstance();
  }
}

void ezGameApplication::WorldData::DestroyWorldModules()
{
  for (auto pModule : m_WorldModules)
  {
    pModule->GetDynamicRTTI()->GetAllocator()->Deallocate(pModule);
  }

  m_WorldModules.Clear();
}
