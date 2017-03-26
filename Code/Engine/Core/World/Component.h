#pragma once

/// \file

#include <Foundation/Reflection/Reflection.h>

#include <Core/World/Declarations.h>

class ezMessage;
class ezWorldWriter;
class ezWorldReader;

/// \brief Base class of all component types.
///
/// Derive from this class to implement custom component types. Also add the EZ_DECLARE_COMPONENT_TYPE macro to your class declaration.
/// Also add a EZ_BEGIN_COMPONENT_TYPE/EZ_END_COMPONENT_TYPE block to a cpp file. In that block you can add reflected members or message handlers.
/// Note that every component type needs a corresponding manager type. Take a look at ezComponentManagerSimple for a simple manager
/// implementation that calls an update method on its components every frame.
/// To create a component instance call CreateComponent on the corresponding manager. Never store a direct pointer to a component but store a
/// component handle instead.
class EZ_CORE_DLL ezComponent : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezComponent, ezReflectedClass);

protected:
  /// \brief Keep the constructor private or protected in derived classes, so it cannot be called manually.
  ezComponent();
  virtual ~ezComponent();

public:
  /// \brief Returns whether this component is dynamic and thus can only be attached to dynamic game objects.
  bool IsDynamic() const;

  /// \brief Sets the active state of the component. Note that it is up to the manager if he differentiates between active and inactive components.
  void SetActive(bool bActive);

  /// \brief Activates the component. Note that it is up to the manager if he differentiates between active and inactive components.
  void Activate();

  /// \brief Deactivates the component.
  void Deactivate();

  /// \brief Returns whether this component is active.
  bool IsActive() const;

  /// \brief Returns whether this component is active and initialized.
  bool IsActiveAndInitialized() const;

  /// \brief Returns whether the OnSimulationStarted method has been called.
  bool IsSimulationStarted() const;


  /// \brief Returns the corresponding manager for this component.
  ezComponentManagerBase* GetManager();

  /// \brief Returns the corresponding manager for this component.
  const ezComponentManagerBase* GetManager() const;

  /// \brief Returns the owner game object if the component is attached to one or nullptr.
  ezGameObject* GetOwner();

  /// \brief Returns the owner game object if the component is attached to one or nullptr.
  const ezGameObject* GetOwner() const;

  /// \brief Returns the corresponding world for this component.
  ezWorld* GetWorld();

  /// \brief Returns the corresponding world for this component.
  const ezWorld* GetWorld() const;


  /// \brief Returns a handle to this component.
  ezComponentHandle GetHandle() const;

  /// \brief Returns the unique id for this component.
  ezUInt32 GetUniqueID() const;

  /// \brief Sets the unique id for this component.
  void SetUniqueID(ezUInt32 uiUniqueID);


  /// \brief Override this to save the current state of the component to the given stream.
  virtual void SerializeComponent(ezWorldWriter& stream) const {}

  /// \brief Override this to load the current state of the component from the given stream.
  ///
  /// The active state will be automatically serialized. The 'initialized' state is not serialized, all components
  /// will be initialized after creation, even if they were already in an initialized state when they were serialized.
  virtual void DeserializeComponent(ezWorldReader& stream) {}


  /// \brief Ensures that the component is initialized.
  void EnsureInitialized();

  /// \brief Ensures that the OnSimulationStarted method has been called. Also ensures initialization first.
  void EnsureSimulationStarted();


  /// \brief Sends a message to this component.
  bool SendMessage(ezMessage& msg);
  bool SendMessage(ezMessage& msg) const;

  /// \brief Queues the message for the given phase and processes it later in that phase.
  void PostMessage(ezMessage& msg, ezObjectMsgQueueType::Enum queueType);

  /// \brief Queues the message for the given phase. The message is processed after the given delay in the corresponding phase.
  void PostMessage(ezMessage& msg, ezObjectMsgQueueType::Enum queueType, ezTime delay);

protected:
  friend class ezWorld;
  friend class ezGameObject;
  friend class ezComponentManagerBase;

  template <typename T>
  ezComponentHandle GetHandleInternal() const;

  ezBitflags<ezObjectFlags> m_ComponentFlags;

  virtual ezUInt16 GetTypeId() const = 0;

  /// \brief Shortcut to GetWorld()->GetIndex() to prevent circular includes.
  ezUInt32 GetWorldIndex() const;


  /// \brief This method is called at the start of the next world update. The global position will be computed before initialization.
  virtual void Initialize() {}

  /// \brief This method is called before the destructor. A derived type can override this method to do common de-initialization work.
  virtual void Deinitialize() {}

  /// \brief Returns whether this component is initialized. Internal method.
  bool IsInitialized() const;

  /// \brief Returns whether this component is currently in the initialization process. Internal method.
  bool IsInitializing() const;

  /// \brief This method is called when the component is activated.
  virtual void OnActivated() {}

  /// \brief This method is called when the component is deactivated.
  virtual void OnDeactivated() {}

  /// \brief This method is called when the component is attached to a game object. At this point the owner pointer is already set. A derived type can override this method to do additional work.
  virtual void OnAfterAttachedToObject() {}

  /// \brief This method is called when the component is detached from a game object. At this point the owner pointer is still set. A derived type can override this method to do additional work.
  virtual void OnBeforeDetachedFromObject() {}

  /// \brief This method is called at the start of the next world update when the world is simulated. This method will be called after the initialization method.
  virtual void OnSimulationStarted() {}

  /// \brief By default disabled. Enable to have OnUnhandledMessage() called for every unhandled message.
  void EnableUnhandledMessageHandler(bool enable);

  /// \brief When EnableUnhandledMessageHandler() was activated, called for messages all unhandled messages.
  virtual bool OnUnhandledMessage(ezMessage& msg) { return false; }

  /// \brief When EnableUnhandledMessageHandler() was activated, called for messages all unhandled messages.
  virtual bool OnUnhandledMessage(ezMessage& msg) const { return false; }

protected:
  /// Messages will be dispatched to this type. Default is what GetDynamicRTTI() returns, can be redirected if necessary.
  const ezRTTI* m_pMessageDispatchType = nullptr;

private:
  ezGenericComponentId m_InternalId;
  ezUInt32 m_uiUniqueID;

  ezComponentManagerBase* m_pManager = nullptr;
  ezGameObject* m_pOwner = nullptr;

  static ezUInt16 TYPE_ID;
};

#include <Core/World/Implementation/Component_inl.h>
