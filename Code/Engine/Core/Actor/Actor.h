#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Communication/Event.h>
#include <Foundation/Reflection/Reflection.h>

class ezActor;
class ezActorManager;

/// \brief Events about important actor updates
struct ezActorEvent
{
  enum class Type
  {
    AfterActivation,   ///< An ezActor has just been activated.
    BeforeDeactivation ///< An ezActor is about to be deactivated.
  };

  Type m_Type;
  ezActor* m_pActor = nullptr;
};

class EZ_CORE_DLL ezActor : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActor, ezReflectedClass);

public:
  ezActor(const char* szActorName);
  ~ezActor();

  /// \brief Important events about actors
  static ezEvent<const ezActorEvent&> s_Events;

  /// \brief Returns the manager that owns this actor
  ezActorManager* GetManager() const;

  /// \brief Returns the name of this actor
  const char* GetName() const;

protected:
  virtual void Activate();
  virtual void Deactivate();
  virtual void Update();

private: // Functions and data directly touched by ezActorManager
  friend class ezActorManager;

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

  ezActorManager* m_pManager = nullptr;

private:
  ezString m_sName;
};
