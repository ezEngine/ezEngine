#include <Core/CorePCH.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/ActorSystem/ActorManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>

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
};

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_SINGLETON(ezActorManager);

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
}

void ezActorManager::AddActor(ezUniquePtr<ezActor>&& pActor)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  EZ_ASSERT_DEV(pActor != nullptr, "Actor must exist to be added.");
  m_pImpl->m_AllActors.PushBack(std::move(pActor));
}

void ezActorManager::DestroyAllActors(const void* pCreatedBy)
{
  EZ_LOCK(m_pImpl->m_Mutex);

  for (ezUInt32 i0 = m_pImpl->m_AllActors.GetCount(); i0 > 0; --i0)
  {
    const ezUInt32 i = i0 - 1;
    ezActor* pActor = m_pImpl->m_AllActors[i].Borrow();

    if (pCreatedBy == nullptr || pActor->GetCreatedBy() == pCreatedBy)
    {
      pActor->m_bQueuedForDestruction = true;

      if (m_bForceQueueActorDestruction == false)
      {
        m_pImpl->m_AllActors.RemoveAtAndCopy(i);
      }
      else
      {
        EZ_ASSERT_ALWAYS(false, "queue actors");
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
    m_pImpl->m_AllActors[i]->Update();
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

    if (pActor->m_bQueuedForDestruction)
    {
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
