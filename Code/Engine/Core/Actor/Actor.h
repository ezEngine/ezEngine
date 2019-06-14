#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Communication/Event.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>

class ezActor;
class ezActorManager;
class ezActorDevice;
struct ezActorImpl;

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
  ezActor(const char* szActorName, const void* pCreatedBy);
  ~ezActor();

  /// \brief Important events about actors
  static ezEvent<const ezActorEvent&> s_Events;

  /// \brief Returns the manager that owns this actor
  ezActorManager* GetManager() const;

  /// \brief Returns the name of this actor
  const char* GetName() const;

  /// \brief Returns the 'created by' pointer of the actor
  const void* GetCreatedBy() const;

  /// \brief Transfers ownership of the ezActorDevice to the ezActor
  void AddDevice(ezUniquePtr<ezActorDevice>&& pDevice);

  /// \brief Queries the ezActor for an ezActorDevice of the given type. Returns null if no such devices was added to the actor.
  ezActorDevice* GetDevice(const ezRTTI* pDeviceType) const;

  /// \brief Templated overload of GetDevice() that automatically casts to the desired class type.
  template <typename ActorDeviceType>
  ActorDeviceType* GetDevice() const
  {
    return static_cast<ActorDeviceType*>(GetDevice(ezGetStaticRTTI<ActorDeviceType>()));
  }

  /// \brief Fills the list with all devices that have been added to the actor.
  void GetAllDevices(ezHybridArray<ezActorDevice*, 8>& out_AllDevices);

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

  ezActorManager* m_pOwningManager = nullptr;

private:
  ezUniquePtr<ezActorImpl> m_pImpl;
};
