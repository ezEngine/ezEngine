#pragma once

/// \file

#include <Core/World/Declarations.h>
#include <Foundation/Reflection/Reflection.h>

class ezMessage;
class ezWorldWriter;
class ezWorldReader;

/// \brief Base class of all component types.
///
/// Derive from this class to implement custom component types. Also add the EZ_DECLARE_COMPONENT_TYPE macro to your class declaration.
/// Also add a EZ_BEGIN_COMPONENT_TYPE/EZ_END_COMPONENT_TYPE block to a cpp file. In that block you can add reflected members or message
/// handlers. Note that every component type needs a corresponding manager type. Take a look at ezComponentManagerSimple for a simple
/// manager implementation that calls an update method on its components every frame. To create a component instance call CreateComponent on
/// the corresponding manager. Never store a direct pointer to a component but store a component handle instead.
class EZ_CORE_DLL ezComponent : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezComponent, ezReflectedClass);

protected:
  /// \brief Keep the constructor private or protected in derived classes, so it cannot be called manually.
  ezComponent();
  virtual ~ezComponent();

public:
  /// \brief Sets the active state of the component. Note that it is up to the manager if he differentiates between active and inactive
  /// components.
  void SetActive(bool bActive);

  /// \brief Activates the component. Note that it is up to the manager if he differentiates between active and inactive components.
  void Activate();

  /// \brief Deactivates the component.
  void Deactivate();

  /// \brief Returns whether this component is active.
  bool IsActive() const;

  /// \brief Returns whether this component is active and initialized.
  bool IsActiveAndInitialized() const;

  /// \brief Whether the component is currently active and simulation has been started as well.
  bool IsActiveAndSimulating() const;

  /// \brief Returns the corresponding manager for this component.
  ezComponentManagerBase* GetOwningManager();

  /// \brief Returns the corresponding manager for this component.
  const ezComponentManagerBase* GetOwningManager() const;

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
  virtual void SerializeComponent(ezWorldWriter& stream) const;

  /// \brief Override this to load the current state of the component from the given stream.
  ///
  /// The active state will be automatically serialized. The 'initialized' state is not serialized, all components
  /// will be initialized after creation, even if they were already in an initialized state when they were serialized.
  virtual void DeserializeComponent(ezWorldReader& stream);


  /// \brief Ensures that the component is initialized. Must only be called from another component's Initialize callback.
  void EnsureInitialized();

  /// \brief Ensures that the OnSimulationStarted method has been called. Must only be called from another component's OnSimulationStarted
  /// callback.
  void EnsureSimulationStarted();


  /// \brief Sends a message to this component.
  bool SendMessage(ezMessage& msg);
  bool SendMessage(ezMessage& msg) const;

  /// \brief Queues the message for the given phase and processes it later in that phase.
  void PostMessage(const ezMessage& msg, ezObjectMsgQueueType::Enum queueType) const;

  /// \brief Queues the message for the given phase. The message is processed after the given delay in the corresponding phase.
  void PostMessage(const ezMessage& msg, ezObjectMsgQueueType::Enum queueType, ezTime delay) const;

  /// \brief Stores a custom flag. Index must be between 0 and 7.
  ///
  /// This is for component specific usage to have some bool values without additional memory consumption.
  /// Be careful to check which flags may already be in use by base classes.
  void SetUserFlag(ezUInt8 flagIndex, bool set);

  /// \brief Retrieves a custom flag. Index must be between 0 and 7.
  bool GetUserFlag(ezUInt8 flagIndex) const;

protected:
  friend class ezWorld;
  friend class ezGameObject;
  friend class ezComponentManagerBase;

  /// \brief Returns whether this component is dynamic and thus can only be attached to dynamic game objects.
  bool IsDynamic() const;

  virtual ezUInt16 GetTypeId() const = 0;
  virtual ezComponentMode::Enum GetMode() const = 0;

  /// \brief Can be overridden for basic initialization that depends on a valid hierarchy and position.
  ///
  /// All trivial initialization should be done in the constructor.
  /// For typical game code, you should prefer to use OnSimulationStarted().
  /// This method is called once for every component, after creation but only at the start of the next world update.
  /// Therefore the global position has already been computed and the owner ezGameObject is set.
  /// Contrary to OnActivated() and OnSimulationStarted(), this function is always called for all components.
  ///
  /// \sa OnActivated(), OnDeactivated(), Initialize(), Deinitialize(), OnSimulationStarted()
  virtual void Initialize();

  /// \brief This method is called before the component is destroyed. A derived type can override this method to do common de-initialization
  /// work.
  ///
  /// This function is always called before destruction, even if the component is currently not active.
  /// The default implementation checks whether the component is currently active and will ensure OnDeactivated() gets called if necessary.
  /// For typical game code, prefer to use OnDeactivated().
  ///
  /// \sa OnActivated(), OnDeactivated(), Initialize(), Deinitialize(), OnSimulationStarted()
  virtual void Deinitialize();

  /// \brief This method is called when the component gets activated.
  ///
  /// By default a component is active, but it can be created in an inactive state. In such a case OnActivated() is only called once a
  /// component is activated. If a component gets switched between active and inactive at runtime, OnActivated() and OnDeactivated() are
  /// called accordingly. In contrast Initialize() and Deinitialize() are only ever called once.
  ///
  /// \sa OnActivated(), OnDeactivated(), Initialize(), Deinitialize(), OnSimulationStarted()
  virtual void OnActivated();

  /// \brief This method is called when the component gets deactivated.
  ///
  /// Upon destruction, a component that is active first gets deactivated. Therefore OnDeactivated() should be used for typical game code
  /// cleanup.
  ///
  /// \sa OnActivated(), OnDeactivated(), Initialize(), Deinitialize(), OnSimulationStarted()
  virtual void OnDeactivated();

  /// \brief This method is called once for active components, at the start of the next world update, but only when the world is simulated.
  ///
  /// This is the one preferred method to setup typical game logic. In a pure game environment there is no practical difference between
  /// OnActivated() and OnSimulationStarted(), as OnSimulationStarted() will be called right after OnActivated().
  ///
  /// However, when a scene is open inside the editor, there is an important difference:
  /// OnActivated() is called once the component was created.
  /// OnSimulationStarted() is only called once the game simulation is started inside the editor.
  /// As an example, if a component starts a sound in OnActivated(), that sound will play right after the scene has been loaded into the
  /// editor. If instead the sound gets started in OnSimulationStarted(), it will only play once the user starts the game mode inside the
  /// editor.
  ///
  /// Additionally, OnSimulationStarted() is only ever executed once on a component, even if the ezWorld pauses and resumes world simulation
  /// multiple times. Thus components that should only execute a thing exactly once, will work correctly. In contrast OnActivated() and
  /// OnDeactivated() will be executed every time the component's active state is toggled, which could re-execute the same behavior multiple
  /// times.
  ///
  /// \sa OnActivated(), OnDeactivated(), Initialize(), Deinitialize(), OnSimulationStarted()
  virtual void OnSimulationStarted();

  /// \brief By default disabled. Enable to have OnUnhandledMessage() called for every unhandled message.
  void EnableUnhandledMessageHandler(bool enable);

  /// \brief When EnableUnhandledMessageHandler() was activated, called for messages all unhandled messages.
  virtual bool OnUnhandledMessage(ezMessage& msg);

  /// \brief When EnableUnhandledMessageHandler() was activated, called for messages all unhandled messages.
  virtual bool OnUnhandledMessage(ezMessage& msg) const;

protected:
  /// Messages will be dispatched to this type. Default is what GetDynamicRTTI() returns, can be redirected if necessary.
  const ezRTTI* m_pMessageDispatchType = nullptr;

private:
  bool IsInitialized() const;
  bool IsInitializing() const;
  bool IsSimulationStarted() const;

  ezBitflags<ezObjectFlags> m_ComponentFlags;

  ezGenericComponentId m_InternalId;
  ezUInt32 m_uiUniqueID;

  ezComponentManagerBase* m_pManager = nullptr;
  ezGameObject* m_pOwner = nullptr;

  static ezUInt16 TYPE_ID;
};

#include <Core/World/Implementation/Component_inl.h>
