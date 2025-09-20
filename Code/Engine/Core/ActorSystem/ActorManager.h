#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>

struct ezActorManagerImpl;
class ezActor;
class ezActorApiService;

/// \brief Event data for actor lifecycle notifications.
struct ezActorEvent
{
  enum class Type
  {
    AfterActorCreation,
    AfterActorActivation, ///< Sent after ezActor::Activate() has been called
    BeforeActorDestruction,
  };

  Type m_Type;
  ezActor* m_pActor = nullptr;
};

/// \brief Singleton that manages the lifecycle of all actors and actor API services.
///
/// The actor manager handles creation, updating, and destruction of actors and services.
/// Updates occur every frame, and destruction can be immediate or queued for the next update.
/// Events are broadcast for actor lifecycle changes to allow external systems to respond.
class EZ_CORE_DLL ezActorManager final
{
  EZ_DECLARE_SINGLETON(ezActorManager);

public:
  ezActorManager();
  ~ezActorManager();

  static ezCopyOnBroadcastEvent<const ezActorEvent&> s_ActorEvents;

  /// \brief Updates all Actors and ActorApiServices, deletes actors that are queued for destruction
  void Update();

  /// \brief Destroys all Actors and ActorApiServices.
  void Shutdown();

  /// \brief Specifies whether something should be destroyed right now or delayed during the next Update()
  enum class DestructionMode
  {
    Immediate, ///< Destruction is executed right now
    Queued     ///< Destruction is queued and done during the next Update()
  };

  /// \brief Gives control over the actor to the ezActorManager.
  ///
  /// From now on the actor will be updated every frame and the lifetime will be managed by the ezActorManager.
  void AddActor(ezUniquePtr<ezActor>&& pActor);

  /// \brief Destroys the given actor. If mode is DestructionMode::Queued the destruction will be delayed until the end of the next Update().
  void DestroyActor(ezActor* pActor, DestructionMode mode = DestructionMode::Immediate);

  /// \brief Destroys all actors which have been created by the pCreatedBy object.
  ///
  /// If pCreatedBy == nullptr, all actors are destroyed.
  /// If mode is DestructionMode::Queued the destruction will be delayed until the end of the next Update().
  void DestroyAllActors(const void* pCreatedBy, DestructionMode mode = DestructionMode::Immediate);

  /// \brief Returns all actors currently in the system, including ones that are queued for destruction.
  void GetAllActors(ezDynamicArray<ezActor*>& out_allActors);

  /// \brief Destroys all actors that are queued for destruction.
  /// This is already executed by Update(), calling it directly only makes sense if one needs to clean up actors without also updating the others.
  void DestroyQueuedActors();

  void AddApiService(ezUniquePtr<ezActorApiService>&& pService);
  void DestroyApiService(ezActorApiService* pService, DestructionMode mode = DestructionMode::Immediate);
  void DestroyAllApiServices(DestructionMode mode = DestructionMode::Immediate);
  void DestroyQueuedActorApiServices();

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

  // used during actor updates to force actor destruction to be queued until the actor updating is finished
  bool m_bForceQueueActorDestruction = false;
  ezUniquePtr<ezActorManagerImpl> m_pImpl;
};
