#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>

class ezActorManager;
struct ezActorServiceImpl;
class ezActor;

class EZ_CORE_DLL ezActorService final
{
  EZ_DECLARE_SINGLETON(ezActorService);

public:
  ezActorService();
  ~ezActorService();

  /// \brief Calls ezActorManager::DestroyAllActors() on all managers
  void DestroyAllActors(const char* szInGroup = nullptr);

  /// \brief Calls ezActorManager::Deactivate() and then destroys all ezActorManager instances. Automatically called during service
  /// destruction.
  void DestroyAllActorManagers();

  /// \brief Adds a new actor manager and takes ownership of it.
  /// ezActorManager::Activate() will be called on it the next time the ezActorService is updated.
  void AddActorManager(ezUniquePtr<ezActorManager>&& pManager);

  /// \brief Tells the service to deactivate and delete the given manager.
  /// ezActorManager::Deactivate() will be called on it the next time the ezActorService is updated. Then it will be deleted.
  void DestroyActorManager(ezActorManager* pManager);

  /// \brief Returns an actor manager of the given type or null if no such manager has been added before.
  ezActorManager* GetActorManager(const ezRTTI* pManagerType);

  template <typename ManagerType>
  ManagerType* GetActorManager()
  {
    return static_cast<ManagerType*>(GetActorManager(ezGetStaticRTTI<ManagerType>()));
  }

  /// \brief Activates queued managers and calls ezActorManager::Update() on them.
  void Update();

  void GetAllActors(ezHybridArray<ezActor*, 8>& out_AllActors);

private:
  void ActivateQueuedManagers();
  void DeactivateQueuedManagers();
  void DeactivateAllManagers();
  void DeleteDeactivatedManagers();
  void UpdateAllManagers();

  ezUniquePtr<ezActorServiceImpl> m_pImpl;
};
