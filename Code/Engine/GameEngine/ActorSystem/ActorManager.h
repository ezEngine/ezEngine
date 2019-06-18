#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Reflection/Reflection.h>

struct ezActorManagerImpl;
class ezActor;
class ezActorApiService;

struct ezActorEvent
{
  enum class Type
  {
    AfterActorCreation,
    BeforeActorDestruction,
  };

  Type m_Type;
  ezActor* m_pActor = nullptr;
};

class EZ_GAMEENGINE_DLL ezActorManager final
{
  EZ_DECLARE_SINGLETON(ezActorManager);

public:
  ezActorManager();
  ~ezActorManager();

  static ezEvent<const ezActorEvent&> s_ActorEvents;

  void Update();

  void Shutdown();

  void AddActor(ezUniquePtr<ezActor>&& pActor);
  void DestroyActor(ezActor* pActor);
  void DestroyAllActors(const void* pCreatedBy);
  void GetAllActors(ezHybridArray<ezActor*, 8>& out_AllActors);

  void AddApiService(ezUniquePtr<ezActorApiService>&& pService);
  void DestroyApiService(ezActorApiService* pService);
  void DestroyAllApiServices();

  ezActorApiService* GetApiService(const ezRTTI* pType);

  template <typename Type>
  Type* GetApiService()
  {
    return static_cast<Type*>(GetApiService(ezGetStaticRTTI<Type>()));
  }

private:
  void ActivateQueuedApiServices();
  void UpdateAllApiServices();
  void UpdateAllActors();

  ezUniquePtr<ezActorManagerImpl> m_pImpl;
};
