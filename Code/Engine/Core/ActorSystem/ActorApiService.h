#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Reflection/Reflection.h>

/// \brief Base class for services that provide API functionality to actors.
///
/// Actor API services are managed by the ezActorManager and follow a lifecycle
/// of activation, updates, and destruction. Derived classes must implement
/// Activate() and Update() methods to provide service functionality.
class EZ_CORE_DLL ezActorApiService : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActorApiService, ezReflectedClass);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezActorApiService);

public:
  ezActorApiService();
  ~ezActorApiService();

protected:
  virtual void Activate() = 0;
  virtual void Update() = 0;

private: // directly accessed by ezActorManager
  friend class ezActorManager;

  enum class State
  {
    New,
    Active,
    QueuedForDestruction
  };

  State m_State = State::New;
};
