#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>

struct ezActorManagerImpl;
class ezActor;

class EZ_CORE_DLL ezActorManager : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActorManager, ezReflectedClass);

public:
  ezActorManager();
  ~ezActorManager();

  /// \brief Calls ezActor::Deactivate() and then destroys all ezActor instances owned by this manager.
  ///
  /// Automatically called during manager destruction.
  /// If pCreatedBy is not null, only actors which were given the specified pointer during construction
  /// will be destroyed.
  void DestroyAllActors(const void* pCreatedBy);

protected:

  ezMutex& GetMutex() const;

  /// \brief Adds a new actor and takes ownership of it.
  /// ezActor::Activate() will be called on it the next time the ezActorManager is updated.
  void AddActor(ezUniquePtr<ezActor>&& pActor);

  /// \brief Tells the manager to deactivate and delete the given actor.
  /// ezActor::Deactivate() will be called on it the next time the ezActorManager is updated. Then it will be deleted.
  void QueueActorForDestruction(ezActor* pActor);

  /// \brief Override this to implement custom initialization. Called by ezActorService::Update().
  /// The default implementation does nothing.
  virtual void Activate();

  /// \brief Override this to implement custom deinitialization. Called by ezActorService::Update().
  /// The default implementation does nothing.
  virtual void Deactivate();

  /// \brief Should update the manager state. Called by ezActorService::Update().
  /// The default implementation just calls UpdateAllActors().
  virtual void Update();

  /// \brief Calls ezActor::Update() on all actors. By default this is called by ezActorManager::Update().
  void UpdateAllActors();

  /// \brief Fills the list with all currently known actors
  void GetAllActors(ezHybridArray<ezActor*, 8>& out_AllActors);

private: // Functions called directly by ezActorService
  friend class ezActorService;

  // Activation state, inspected and modified by ezActorService
  enum class ActivationState
  {
    None,
    Activate,
    Active,
    Deactivate,
    Deactivated
  };

  ActivationState m_ActivationState = ActivationState::None;

  void UpdateActorStates();

private:
  void ActivateQueuedActors();
  void DeactivateQueuedActors();
  void DeactivateAllActors(const void* pCreatedBy = nullptr);
  void DeleteDeactivatedActors();

  ezUniquePtr<ezActorManagerImpl> m_pImpl;
};
