#include <CorePCH.h>

#include <Core/ActorSystem/ActorManager2.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/UniquePtr.h>
#include <Core/ActorSystem/ActorApiListener.h>
#include <Core/ActorSystem/Actor2.h>

//////////////////////////////////////////////////////////////////////////

static ezUniquePtr<ezActorManager2> s_pActorManager;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Core, ezActorManager2)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    s_pActorManager = EZ_DEFAULT_NEW(ezActorManager2);
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

struct ezActorManager2Impl
{
  ezMutex m_Mutex;
  ezHybridArray<ezUniquePtr<ezActor2>, 8> m_AllActors;
  ezHybridArray<ezUniquePtr<ezActorApiListener>, 8> m_AllApiListeners;
};

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_SINGLETON(ezActorManager2);

ezEvent<const ezActor2Event&> ezActorManager2::s_ActorEvents;

ezActorManager2::ezActorManager2()
  : m_SingletonRegistrar(this)
{
  m_pImpl = EZ_DEFAULT_NEW(ezActorManager2Impl);
}

ezActorManager2::~ezActorManager2()
{
  EZ_LOCK(m_pImpl->m_Mutex);

  DestroyAllActors(nullptr);
  DestroyAllApiListeners();
}

void ezActorManager2::AddActor(ezUniquePtr<ezActor2>&& pActor)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  m_pImpl->m_AllActors.PushBack(std::move(pActor));

  ezActor2Event e;
  e.m_Type = ezActor2Event::Type::AfterActorCreation;
  e.m_pActor = m_pImpl->m_AllActors.PeekBack().Borrow();
  s_ActorEvents.Broadcast(e);
}

void ezActorManager2::DestroyActor(ezActor2* pActor)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  for (ezUInt32 i = 0; i < m_pImpl->m_AllActors.GetCount(); ++i)
  {
    if (m_pImpl->m_AllActors[i].Borrow() == pActor)
    {
      ezActor2Event e;
      e.m_Type = ezActor2Event::Type::BeforeActorDestruction;
      e.m_pActor = pActor;
      s_ActorEvents.Broadcast(e);

      m_pImpl->m_AllActors.RemoveAtAndSwap(i);
      break;
    }
  }
}

void ezActorManager2::DestroyAllActors(const void* pCreatedBy)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  for (ezUInt32 i0 = m_pImpl->m_AllActors.GetCount(); i0 > 0; --i0)
  {
    const ezUInt32 i = i0 - 1;

    if (pCreatedBy == nullptr || m_pImpl->m_AllActors[i]->GetCreatedBy() == pCreatedBy)
    {
      ezActor2Event e;
      e.m_Type = ezActor2Event::Type::BeforeActorDestruction;
      e.m_pActor = m_pImpl->m_AllActors[i].Borrow();
      s_ActorEvents.Broadcast(e);

      m_pImpl->m_AllActors.RemoveAtAndSwap(i);
    }
  }
}

void ezActorManager2::GetAllActors(ezHybridArray<ezActor2*, 8>& out_AllActors)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  out_AllActors.Clear();

  for (auto& pActor : m_pImpl->m_AllActors)
  {
    out_AllActors.PushBack(pActor.Borrow());
  }
}

void ezActorManager2::AddApiListener(ezUniquePtr<ezActorApiListener>&& pListener)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  EZ_ASSERT_DEV(pListener != nullptr, "Invalid API listener");
  EZ_ASSERT_DEV(!pListener->m_bActivated, "Actor API listener already in use");

  for (auto& pExisting : m_pImpl->m_AllApiListeners)
  {
    EZ_ASSERT_ALWAYS(pListener->GetDynamicRTTI() != pExisting->GetDynamicRTTI(), "An actor API listener of this type has already been added");
  }

  m_pImpl->m_AllApiListeners.PushBack(std::move(pListener));
}

void ezActorManager2::DestroyApiListener(ezActorApiListener* pListener)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  EZ_ASSERT_DEV(pListener != nullptr, "Invalid API listener");

  for (ezUInt32 i = 0; i < m_pImpl->m_AllApiListeners.GetCount(); ++i)
  {
    if (m_pImpl->m_AllApiListeners[i].Borrow() == pListener)
    {
      m_pImpl->m_AllApiListeners.RemoveAtAndSwap(i);
      break;
    }
  }
}

void ezActorManager2::DestroyAllApiListeners()
{
  EZ_LOCK(m_pImpl->m_Mutex);

  m_pImpl->m_AllApiListeners.Clear();
}

void ezActorManager2::ActivateQueuedApiListeners()
{
  EZ_LOCK(m_pImpl->m_Mutex);

  for (auto& pManager : m_pImpl->m_AllApiListeners)
  {
    if (!pManager->m_bActivated)
    {
      pManager->Activate();
      pManager->m_bActivated = true;
    }
  }
}

ezActorApiListener* ezActorManager2::GetApiListener(const ezRTTI* pType)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  EZ_ASSERT_DEV(pType->IsDerivedFrom<ezActorApiListener>(), "The queried type has to derive from ezActorApiListener");

  for (auto& pListener : m_pImpl->m_AllApiListeners)
  {
    if (pListener->GetDynamicRTTI()->IsDerivedFrom(pType))
      return pListener.Borrow();
  }

  return nullptr;
}

void ezActorManager2::UpdateAllApiListeners()
{
  EZ_LOCK(m_pImpl->m_Mutex);

  for (auto& pListener : m_pImpl->m_AllApiListeners)
  {
    EZ_ASSERT_DEBUG(pListener->m_bActivated, "All actor API listeners should be active now");

    pListener->Update();
  }
}

void ezActorManager2::UpdateAllActors()
{
  EZ_LOCK(m_pImpl->m_Mutex);

  for (auto& pActor : m_pImpl->m_AllActors)
  {
    pActor->Update();
  }
}

void ezActorManager2::Update()
{
  EZ_LOCK(m_pImpl->m_Mutex);

  ActivateQueuedApiListeners();
  UpdateAllApiListeners();
  UpdateAllActors();
}





