#include <CorePCH.h>

#include <Core/Actor/ActorService.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/UniquePtr.h>

//////////////////////////////////////////////////////////////////////////

static ezUniquePtr<ezActorService> s_pActorService;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Core, ezActorService)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    s_pActorService = EZ_DEFAULT_NEW(ezActorService);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    s_pActorService.Clear();
  }
  
  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    if (s_pActorService)
    {
      s_pActorService->DestroyAllActors();
    }
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on


//////////////////////////////////////////////////////////////////////////

struct ezActorServiceImpl
{
  ezMutex m_Mutex;
  ezHybridArray<ezUniquePtr<ezActorManager>, 8> m_AllManagers;
};

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_SINGLETON(ezActorService);

ezActorService::ezActorService()
  : m_SingletonRegistrar(this)
{
  m_pImpl = EZ_DEFAULT_NEW(ezActorServiceImpl);
}

ezActorService::~ezActorService()
{
  DestroyAllActorManagers();
}

void ezActorService::DestroyAllActors(const char* szInGroup /*= nullptr*/)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  for (auto& pMan : m_pImpl->m_AllManagers)
  {
    pMan->DestroyAllActors(szInGroup);
  }
}

void ezActorService::DestroyAllActorManagers()
{
  EZ_LOCK(m_pImpl->m_Mutex);

  DeactivateAllManagers();
  DeleteDeactivatedManagers();

  EZ_ASSERT_DEBUG(m_pImpl->m_AllManagers.IsEmpty(), "List of actor managers should be empty now.");
}

void ezActorService::AddActorManager(ezUniquePtr<ezActorManager>&& pManager)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  EZ_ASSERT_DEV(pManager != nullptr, "Invalid actor manager");
  EZ_ASSERT_DEV(pManager->m_ActivationState == ezActorManager::ActivationState::None, "Actor manager already in use");

  pManager->m_ActivationState = ezActorManager::ActivationState::Activate;
  m_pImpl->m_AllManagers.PushBack(std::move(pManager));
}

void ezActorService::DestroyActorManager(ezActorManager* pManager)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  EZ_ASSERT_DEV(pManager != nullptr, "Invalid actor manager");
  EZ_ASSERT_DEV(pManager->m_ActivationState == ezActorManager::ActivationState::Active, "Actor manager already in use");

  pManager->m_ActivationState = ezActorManager::ActivationState::Deactivate;
}

void ezActorService::ActivateQueuedManagers()
{
  EZ_LOCK(m_pImpl->m_Mutex);

  for (auto& pManager : m_pImpl->m_AllManagers)
  {
    if (pManager->m_ActivationState == ezActorManager::ActivationState::Activate)
    {
      pManager->Activate();
      pManager->m_ActivationState = ezActorManager::ActivationState::Active;
    }
  }
}

void ezActorService::DeactivateQueuedManagers()
{
  EZ_LOCK(m_pImpl->m_Mutex);

  for (auto& pManager : m_pImpl->m_AllManagers)
  {
    if (pManager->m_ActivationState == ezActorManager::ActivationState::Deactivate)
    {
      pManager->Deactivate();
      pManager->m_ActivationState = ezActorManager::ActivationState::Deactivated;
    }
  }
}

void ezActorService::DeactivateAllManagers()
{
  EZ_LOCK(m_pImpl->m_Mutex);

  for (auto& pManager : m_pImpl->m_AllManagers)
  {
    if (pManager->m_ActivationState == ezActorManager::ActivationState::Active ||
        pManager->m_ActivationState == ezActorManager::ActivationState::Deactivate)
    {
      pManager->Deactivate();
    }

    // set all managers to the 'Deactivated' state
    pManager->m_ActivationState = ezActorManager::ActivationState::Deactivated;
  }
}

void ezActorService::DeleteDeactivatedManagers()
{
  EZ_LOCK(m_pImpl->m_Mutex);

  for (ezUInt32 i0 = m_pImpl->m_AllManagers.GetCount(); i0 > 0; --i0)
  {
    const ezUInt32 i = i0 - 1;

    if (m_pImpl->m_AllManagers[i]->m_ActivationState == ezActorManager::ActivationState::Deactivated)
    {
      m_pImpl->m_AllManagers.RemoveAtAndSwap(i);
    }
  }
}

void ezActorService::UpdateAllManagers()
{
  EZ_LOCK(m_pImpl->m_Mutex);

  for (auto& pManager : m_pImpl->m_AllManagers)
  {
    EZ_ASSERT_DEBUG(pManager->m_ActivationState == ezActorManager::ActivationState::Active, "All actor managers should be active now");

    pManager->Update();
  }
}

void ezActorService::Update()
{
  EZ_LOCK(m_pImpl->m_Mutex);

  DeactivateQueuedManagers();
  DeleteDeactivatedManagers();
  ActivateQueuedManagers();
  UpdateAllManagers();
}

ezActorManager* ezActorService::GetActorManager(const ezRTTI* pManagerType)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  for (auto& pManager : m_pImpl->m_AllManagers)
  {
    if (pManager->GetDynamicRTTI()->IsDerivedFrom(pManagerType))
      return pManager.Borrow();
  }

  return nullptr;
}

void ezActorService::GetAllActors(ezHybridArray<ezActor*, 8>& out_AllActors)
{
  for (auto& pMan : m_pImpl->m_AllManagers)
  {
    pMan->GetAllActors(out_AllActors);
  }
}


