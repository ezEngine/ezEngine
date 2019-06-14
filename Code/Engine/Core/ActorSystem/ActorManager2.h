#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Communication/Event.h>

struct ezActorManager2Impl;
class ezActor2;
class ezActorApiListener;

struct ezActor2Event
{
  enum class Type
  {
    AfterActorCreation,
    BeforeActorDestruction,
  };

  Type m_Type;
  ezActor2* m_pActor = nullptr;
};

class EZ_CORE_DLL ezActorManager2 final
{
  EZ_DECLARE_SINGLETON(ezActorManager2);

public:
  ezActorManager2();
  ~ezActorManager2();

  static ezEvent<const ezActor2Event&> s_ActorEvents;

  void Update();

  void Shutdown();

  void AddActor(ezUniquePtr<ezActor2>&& pActor);
  void DestroyActor(ezActor2* pActor);
  void DestroyAllActors(const void* pCreatedBy);
  void GetAllActors(ezHybridArray<ezActor2*, 8>& out_AllActors);

  void AddApiListener(ezUniquePtr<ezActorApiListener>&& pListener);
  void DestroyApiListener(ezActorApiListener* pListener);
  void DestroyAllApiListeners();

  ezActorApiListener* GetApiListener(const ezRTTI* pType);

  template <typename Type>
  Type* GetApiListener()
  {
    return static_cast<Type*>(GetApiListener(ezGetStaticRTTI<Type>()));
  }

private:
  void ActivateQueuedApiListeners();
  void UpdateAllApiListeners();
  void UpdateAllActors();

  ezUniquePtr<ezActorManager2Impl> m_pImpl;
};
