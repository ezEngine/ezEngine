#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>

struct ezActorManagerImpl;
class ezActor;

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

  /// \brief Updates all Actors and ActorApiServices, deletes actors that are queued for destruction
  void Update();

  /// \brief Destroys all Actors and ActorApiServices.
  void Shutdown();

  /// \brief Gives control over the actor to the ezActorManager.
  ///
  /// From now on the actor will be updated every frame and the lifetime will be managed by the ezActorManager.
  void AddActor(ezUniquePtr<ezActor>&& pActor);

  /// \brief Destroys all actors which have been created by the pCreatedBy object.
  ///
  /// If pCreatedBy == nullptr, all actors are destroyed.
  /// If mode is DestructionMode::Queued the destruction will be delayed until the end of the next Update().
  void DestroyAllActors(const void* pCreatedBy);

  /// \brief Returns all actors currently in the system, including ones that are queued for destruction.
  void GetAllActors(ezDynamicArray<ezActor*>& out_allActors);

private:
  /// \brief Destroys all actors that are queued for destruction.
  /// This is already executed by Update(), calling it directly only makes sense if one needs to clean up actors without also updating the others.
  void DestroyQueuedActors();
  void UpdateAllActors();

  // used during actor updates to force actor destruction to be queued until the actor updating is finished
  bool m_bForceQueueActorDestruction = false;
  ezUniquePtr<ezActorManagerImpl> m_pImpl;
};
