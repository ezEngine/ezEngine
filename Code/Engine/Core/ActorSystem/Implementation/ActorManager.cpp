#include <CorePCH.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/ActorSystem/ActorApiService.h>
#include <Core/ActorSystem/ActorManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/UniquePtr.h>

//////////////////////////////////////////////////////////////////////////

static ezUniquePtr<ezActorManager> s_pActorManager;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Core, ezActorManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    s_pActorManager = EZ_DEFAULT_NEW(ezActorManager);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    s_pActorManager.Clear();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    if (s_pActorManager)
    {
      s_pActorManager->DestroyAllActors(nullptr);
    }
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on


//////////////////////////////////////////////////////////////////////////

struct ezActorManagerImpl
{
  ezMutex m_Mutex;
  ezHybridArray<ezUniquePtr<ezActor>, 8> m_AllActors;
  ezHybridArray<ezUniquePtr<ezActorApiService>, 8> m_AllApiServices;
};

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_SINGLETON(ezActorManager);

ezEvent<const ezActorEvent&> ezActorManager::s_ActorEvents;

ezActorManager::ezActorManager()
  : m_SingletonRegistrar(this)
{
  m_pImpl = EZ_DEFAULT_NEW(ezActorManagerImpl);
}

ezActorManager::~ezActorManager()
{
  Shutdown();
}

void ezActorManager::Shutdown()
{
  EZ_LOCK(m_pImpl->m_Mutex);

  DestroyAllActors(nullptr);
  DestroyAllApiServices();
}

void ezActorManager::AddActor(ezUniquePtr<ezActor>&& pActor)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  m_pImpl->m_AllActors.PushBack(std::move(pActor));

  ezActorEvent e;
  e.m_Type = ezActorEvent::Type::AfterActorCreation;
  e.m_pActor = m_pImpl->m_AllActors.PeekBack().Borrow();
  s_ActorEvents.Broadcast(e);
}

void ezActorManager::DestroyActor(ezActor* pActor)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  for (ezUInt32 i = 0; i < m_pImpl->m_AllActors.GetCount(); ++i)
  {
    if (m_pImpl->m_AllActors[i].Borrow() == pActor)
    {
      ezActorEvent e;
      e.m_Type = ezActorEvent::Type::BeforeActorDestruction;
      e.m_pActor = pActor;
      s_ActorEvents.Broadcast(e);

      m_pImpl->m_AllActors.RemoveAtAndSwap(i);
      break;
    }
  }
}

void ezActorManager::DestroyAllActors(const void* pCreatedBy)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  for (ezUInt32 i0 = m_pImpl->m_AllActors.GetCount(); i0 > 0; --i0)
  {
    const ezUInt32 i = i0 - 1;

    if (pCreatedBy == nullptr || m_pImpl->m_AllActors[i]->GetCreatedBy() == pCreatedBy)
    {
      ezActorEvent e;
      e.m_Type = ezActorEvent::Type::BeforeActorDestruction;
      e.m_pActor = m_pImpl->m_AllActors[i].Borrow();
      s_ActorEvents.Broadcast(e);

      m_pImpl->m_AllActors.RemoveAtAndSwap(i);
    }
  }
}

void ezActorManager::GetAllActors(ezHybridArray<ezActor*, 8>& out_AllActors)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  out_AllActors.Clear();

  for (auto& pActor : m_pImpl->m_AllActors)
  {
    out_AllActors.PushBack(pActor.Borrow());
  }
}

void ezActorManager::AddApiService(ezUniquePtr<ezActorApiService>&& pApiService)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  EZ_ASSERT_DEV(pApiService != nullptr, "Invalid API service");
  EZ_ASSERT_DEV(!pApiService->m_bActivated, "Actor API service already in use");

  for (auto& pExisting : m_pImpl->m_AllApiServices)
  {
    EZ_ASSERT_ALWAYS(
      pApiService->GetDynamicRTTI() != pExisting->GetDynamicRTTI(), "An actor API service of this type has already been added");
  }

  m_pImpl->m_AllApiServices.PushBack(std::move(pApiService));
}

void ezActorManager::DestroyApiService(ezActorApiService* pApiService)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  EZ_ASSERT_DEV(pApiService != nullptr, "Invalid API service");

  for (ezUInt32 i = 0; i < m_pImpl->m_AllApiServices.GetCount(); ++i)
  {
    if (m_pImpl->m_AllApiServices[i].Borrow() == pApiService)
    {
      m_pImpl->m_AllApiServices.RemoveAtAndSwap(i);
      break;
    }
  }
}

void ezActorManager::DestroyAllApiServices()
{
  EZ_LOCK(m_pImpl->m_Mutex);

  m_pImpl->m_AllApiServices.Clear();
}

void ezActorManager::ActivateQueuedApiServices()
{
  EZ_LOCK(m_pImpl->m_Mutex);

  for (auto& pManager : m_pImpl->m_AllApiServices)
  {
    if (!pManager->m_bActivated)
    {
      pManager->Activate();
      pManager->m_bActivated = true;
    }
  }
}

ezActorApiService* ezActorManager::GetApiService(const ezRTTI* pType)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  EZ_ASSERT_DEV(pType->IsDerivedFrom<ezActorApiService>(), "The queried type has to derive from ezActorApiService");

  for (auto& pApiService : m_pImpl->m_AllApiServices)
  {
    if (pApiService->GetDynamicRTTI()->IsDerivedFrom(pType))
      return pApiService.Borrow();
  }

  return nullptr;
}

void ezActorManager::UpdateAllApiServices()
{
  EZ_LOCK(m_pImpl->m_Mutex);

  for (auto& pApiService : m_pImpl->m_AllApiServices)
  {
    EZ_ASSERT_DEBUG(pApiService->m_bActivated, "All actor API services should be active now");

    pApiService->Update();
  }
}

void ezActorManager::UpdateAllActors()
{
  EZ_LOCK(m_pImpl->m_Mutex);

  for (auto& pActor : m_pImpl->m_AllActors)
  {
    pActor->Update();
  }
}

void ezActorManager::Update()
{
  EZ_LOCK(m_pImpl->m_Mutex);

  ActivateQueuedApiServices();
  UpdateAllApiServices();
  UpdateAllActors();
}
