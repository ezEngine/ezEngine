#include <CorePCH.h>

#include <Core/Actor/Actor.h>
#include <Core/Actor/ActorManager.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezActorManager, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

struct ezActorManagerImpl
{
  mutable ezMutex m_Mutex;
  ezHybridArray<ezUniquePtr<ezActor>, 4> m_AllActors;
};

ezActorManager::ezActorManager()
{
  m_pImpl = EZ_DEFAULT_NEW(ezActorManagerImpl);
}

ezActorManager::~ezActorManager()
{
  DestroyAllActors();
}

void ezActorManager::DestroyAllActors()
{
  EZ_LOCK(m_pImpl->m_Mutex);

  DeactivateAllActors();
  DeleteDeactivatedActors();

  EZ_ASSERT_DEBUG(m_pImpl->m_AllActors.IsEmpty(), "The list of actors should be empty now.");
}

ezMutex& ezActorManager::GetMutex() const
{
  return m_pImpl->m_Mutex;
}

void ezActorManager::AddActor(ezUniquePtr<ezActor>&& pActor)
{
  EZ_LOCK(GetMutex());

  EZ_ASSERT_DEV(pActor != nullptr, "Invalid actor");
  EZ_ASSERT_DEV(pActor->m_pOwningManager == nullptr, "Actor already in use");
  EZ_ASSERT_DEV(pActor->m_ActivationState == ezActor::ActivationState::None, "Actor already in use");

  pActor->m_pOwningManager = this;
  pActor->m_ActivationState = ezActor::ActivationState::Activate;
  m_pImpl->m_AllActors.PushBack(std::move(pActor));
}

void ezActorManager::DestroyActor(ezActor* pActor)
{
  EZ_LOCK(GetMutex());

  EZ_ASSERT_DEV(pActor != nullptr, "Invalid actor");
  EZ_ASSERT_DEV(pActor->m_ActivationState == ezActor::ActivationState::Active, "Actor already in use");

  pActor->m_ActivationState = ezActor::ActivationState::Deactivate;
}

void ezActorManager::Activate() {}
void ezActorManager::Deactivate() {}

void ezActorManager::Update()
{
  UpdateAllActors();
}

void ezActorManager::UpdateActorStates()
{
  EZ_LOCK(GetMutex());

  DeactivateQueuedActors();
  DeleteDeactivatedActors();
  ActivateQueuedActors();
}

void ezActorManager::UpdateAllActors()
{
  EZ_LOCK(GetMutex());

  for (auto& pActor : m_pImpl->m_AllActors)
  {
    EZ_ASSERT_DEBUG(pActor->m_ActivationState == ezActor::ActivationState::Active, "All actors should be active now");

    pActor->Update();
  }
}

void ezActorManager::ActivateQueuedActors()
{
  EZ_LOCK(GetMutex());

  for (auto& pActor : m_pImpl->m_AllActors)
  {
    if (pActor->m_ActivationState == ezActor::ActivationState::Activate)
    {
      pActor->Activate();
      pActor->m_ActivationState = ezActor::ActivationState::Active;

      ezActorEvent e;
      e.m_pActor = pActor.Borrow();
      e.m_Type = ezActorEvent::Type::AfterActivation;
      ezActor::s_Events.Broadcast(e);
    }
  }
}

void ezActorManager::DeactivateQueuedActors()
{
  EZ_LOCK(GetMutex());

  for (auto& pActor : m_pImpl->m_AllActors)
  {
    if (pActor->m_ActivationState == ezActor::ActivationState::Deactivate)
    {
      ezActorEvent e;
      e.m_pActor = pActor.Borrow();
      e.m_Type = ezActorEvent::Type::BeforeDeactivation;
      ezActor::s_Events.Broadcast(e);

      pActor->Deactivate();
      pActor->m_ActivationState = ezActor::ActivationState::Deactivated;
    }
  }
}

void ezActorManager::DeactivateAllActors()
{
  EZ_LOCK(GetMutex());

  for (auto& pActor : m_pImpl->m_AllActors)
  {
    if (pActor->m_ActivationState == ezActor::ActivationState::Active || pActor->m_ActivationState == ezActor::ActivationState::Deactivate)
    {
      ezActorEvent e;
      e.m_pActor = pActor.Borrow();
      e.m_Type = ezActorEvent::Type::BeforeDeactivation;
      ezActor::s_Events.Broadcast(e);

      pActor->Deactivate();
    }

    // set all managers to the 'Deactivated' state
    pActor->m_ActivationState = ezActor::ActivationState::Deactivated;
  }
}

void ezActorManager::DeleteDeactivatedActors()
{
  EZ_LOCK(GetMutex());

  for (ezUInt32 i0 = m_pImpl->m_AllActors.GetCount(); i0 > 0; --i0)
  {
    const ezUInt32 i = i0 - 1;

    if (m_pImpl->m_AllActors[i]->m_ActivationState == ezActor::ActivationState::Deactivated)
    {
      m_pImpl->m_AllActors.RemoveAtAndSwap(i);
    }
  }
}
