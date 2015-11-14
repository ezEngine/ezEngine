#include <Core/PCH.h>
#include <Core/Scene/Scene.h>
#include <Core/Scene/SceneModule.h>

ezScene::ezScene()
{

}

ezScene::~ezScene()
{
  Deinitialize();
}

void ezScene::Initialize()
{
  CreateSceneModules();

  for (auto pModule : m_SceneModules)
  {
    pModule->Startup();
  }
}

void ezScene::Deinitialize()
{
  for (auto pModule : m_SceneModules)
  {
    pModule->Shutdown();
  }

  DestroySceneModules();
}

void ezScene::Update()
{
  for (auto pModule : m_SceneModules)
  {
    pModule->Update();
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

