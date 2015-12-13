#include <Core/PCH.h>
#include <Core/Scene/Scene.h>
#include <Core/Scene/SceneModule.h>
#include <Core/World/World.h>

ezScene::ezScene()
{

}

ezScene::~ezScene()
{
  Deinitialize();
}

void ezScene::Initialize(const char* szWorldName)
{
  CreateSceneModules();

  m_pWorld = EZ_DEFAULT_NEW(ezWorld, szWorldName);
  m_pWorld->GetClock().SetTimeStepSmoothing(&m_TimeStepSmoothing);

  // scene modules will most likely register component managers, so just mark it for write once here
  EZ_LOCK(m_pWorld->GetWriteMarker());

  for (auto pModule : m_SceneModules)
  {
    pModule->Startup(this);
  }
}

void ezScene::Deinitialize()
{
  if (m_pWorld != nullptr)
  {
    // scene modules will most likely register component managers, so just mark it for write once here
    EZ_LOCK(m_pWorld->GetWriteMarker());

    for (auto pModule : m_SceneModules)
    {
      pModule->Shutdown();
    }
  }

  EZ_DEFAULT_DELETE(m_pWorld);

  DestroySceneModules();
}

void ezScene::Update()
{
  // mark world for write here ?

  for (auto pModule : m_SceneModules)
  {
    pModule->Update();
  }
}

void ezScene::ReinitSceneModules()
{
  for (auto pModule : m_SceneModules)
  {
    pModule->Reinit();
  }
}

void ezScene::CreateSceneModules()
{
  ezRTTI* pRtti = ezRTTI::GetFirstInstance();

  while (pRtti)
  {
    if (pRtti->IsDerivedFrom<ezSceneModule>())
    {
      ezRTTIAllocator* pAlloc = pRtti->GetAllocator();

      if (pAlloc && pAlloc->CanAllocate())
      {
        ezSceneModule* pModule = static_cast<ezSceneModule*>(pAlloc->Allocate());

        m_SceneModules.PushBack(pModule);
      }
    }

    pRtti = pRtti->GetNextInstance();
  }
}

void ezScene::DestroySceneModules()
{
  for (auto pModule : m_SceneModules)
  {
    pModule->GetDynamicRTTI()->GetAllocator()->Deallocate(pModule);
  }

  m_SceneModules.Clear();
}

