#include <Core/CorePCH.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/ActorSystem/ActorManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
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
      s_pActorManager->DestroyAllActors(nullptr, ezActorManager::DestructionMode::Immediate);
    }
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on


//////////////////////////////////////////////////////////////////////////

struct ezActorManagerImpl
{
  ezMutex m_Mutex;
  ezHybridArray<ezUniquePtr<ezActor>, 8> m_AllActors;
};

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_SINGLETON(ezActorManager);

ezCopyOnBroadcastEvent<const ezActorEvent&> ezActorManager::s_ActorEvents;

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

  DestroyAllActors(nullptr, DestructionMode::Immediate);

  s_ActorEvents.Clear();
}

void ezActorManager::AddActor(ezUniquePtr<ezActor>&& pActor)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  EZ_ASSERT_DEV(pActor != nullptr, "Actor must exist to be added.");
  m_pImpl->m_AllActors.PushBack(std::move(pActor));

  ezActorEvent e;
  e.m_Type = ezActorEvent::Type::AfterActorCreation;
  e.m_pActor = m_pImpl->m_AllActors.PeekBack().Borrow();
  s_ActorEvents.Broadcast(e);
}

void ezActorManager::DestroyActor(ezActor* pActor, DestructionMode mode)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  pActor->m_State = ezActor::State::QueuedForDestruction;

  if (mode == DestructionMode::Immediate && m_bForceQueueActorDestruction == false)
  {
    for (ezUInt32 i = 0; i < m_pImpl->m_AllActors.GetCount(); ++i)
    {
      if (m_pImpl->m_AllActors[i] == pActor)
      {
        ezActorEvent e;
        e.m_Type = ezActorEvent::Type::BeforeActorDestruction;
        e.m_pActor = pActor;
        s_ActorEvents.Broadcast(e);

        m_pImpl->m_AllActors.RemoveAtAndCopy(i);
        break;
      }
    }
  }
}

void ezActorManager::DestroyAllActors(const void* pCreatedBy, DestructionMode mode)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  for (ezUInt32 i0 = m_pImpl->m_AllActors.GetCount(); i0 > 0; --i0)
  {
    const ezUInt32 i = i0 - 1;
    ezActor* pActor = m_pImpl->m_AllActors[i].Borrow();

    if (pCreatedBy == nullptr || pActor->GetCreatedBy() == pCreatedBy)
    {
      pActor->m_State = ezActor::State::QueuedForDestruction;

      if (mode == DestructionMode::Immediate && m_bForceQueueActorDestruction == false)
      {
        ezActorEvent e;
        e.m_Type = ezActorEvent::Type::BeforeActorDestruction;
        e.m_pActor = pActor;
        s_ActorEvents.Broadcast(e);

        m_pImpl->m_AllActors.RemoveAtAndCopy(i);
      }
    }
  }
}

void ezActorManager::GetAllActors(ezDynamicArray<ezActor*>& out_allActors)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  out_allActors.Clear();

  for (auto& pActor : m_pImpl->m_AllActors)
  {
    out_allActors.PushBack(pActor.Borrow());
  }
}

void ezActorManager::UpdateAllActors()
{
  EZ_LOCK(m_pImpl->m_Mutex);

  m_bForceQueueActorDestruction = true;
  EZ_SCOPE_EXIT(m_bForceQueueActorDestruction = false);

  for (ezUInt32 i0 = m_pImpl->m_AllActors.GetCount(); i0 > 0; --i0)
  {
    const ezUInt32 i = i0 - 1;
    ezActor* pActor = m_pImpl->m_AllActors[i].Borrow();

    if (pActor->m_State == ezActor::State::New)
    {
      pActor->m_State = ezActor::State::Active;

      pActor->Activate();

      ezActorEvent e;
      e.m_Type = ezActorEvent::Type::AfterActorActivation;
      e.m_pActor = pActor;
      s_ActorEvents.Broadcast(e);
    }

    if (pActor->m_State == ezActor::State::Active)
    {
      pActor->Update();
    }
  }
}

void ezActorManager::DestroyQueuedActors()
{
  EZ_LOCK(m_pImpl->m_Mutex);

  EZ_ASSERT_DEV(!m_bForceQueueActorDestruction, "Cannot execute this function right now");

  for (ezUInt32 i0 = m_pImpl->m_AllActors.GetCount(); i0 > 0; --i0)
  {
    const ezUInt32 i = i0 - 1;
    ezActor* pActor = m_pImpl->m_AllActors[i].Borrow();

    if (pActor->m_State == ezActor::State::QueuedForDestruction)
    {
      ezActorEvent e;
      e.m_Type = ezActorEvent::Type::BeforeActorDestruction;
      e.m_pActor = pActor;
      s_ActorEvents.Broadcast(e);

      m_pImpl->m_AllActors.RemoveAtAndCopy(i);
    }
  }
}

void ezActorManager::Update()
{
  EZ_LOCK(m_pImpl->m_Mutex);

  DestroyQueuedActors();
  UpdateAllActors();
}


EZ_STATICLINK_FILE(Core, Core_ActorSystem_Implementation_ActorManager);
