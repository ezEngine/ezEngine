#pragma once

/// \file

#include <Core/World/Declarations.h>
#include <Foundation/Reflection/Reflection.h>

class ezMessage;
class ezWorldWriter;
class ezWorldReader;

// TODO: windows.h workaround
#ifdef SendMessage
#  undef SendMessage
#endif

/// \brief Base class of all component types.
///
/// Derive from this class to implement custom component types. Also add the EZ_DECLARE_COMPONENT_TYPE macro to your class declaration.
/// Also add a EZ_BEGIN_COMPONENT_TYPE/EZ_END_COMPONENT_TYPE block to a cpp file. In that block you can add reflected members or message
/// handlers. Note that every component type needs a corresponding manager type. Take a look at ezComponentManagerSimple for a simple
/// manager implementation that calls an update method on its components every frame. To create a component instance call CreateComponent on
/// the corresponding manager. Never store a direct pointer to a component but store an ezComponentHandle instead.
class EZ_CORE_DLL ezComponent : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezComponent, ezReflectedClass);

protected:
  /// \brief Keep the constructor private or protected in derived classes, so it cannot be called manually.
  ezComponent();
  virtual ~ezComponent();

public:
  /// \brief Sets the active flag of the component, which affects its active state.
  ///
  /// The active flag affects the 'active state' of the component. Ie. a component without the active flag will always be inactive.
  /// However, the active state is also affected by the active state of the owning game object. Thus a component attached to an inactive
  /// game object, will also be inactive.
  ///
  /// Note that it is up to the component manager though, whether it differentiates between active and inactive components.
  ///
  /// \sa ezGameObject::IsActive(), ezGameObject::SetActiveFlag()
  void SetActiveFlag(bool bEnabled);

  /// \brief Checks whether the 'active flag' is set on this component. Note that this does not mean that the component is also 'active'.
  ///
  /// \sa IsActive(), SetActiveFlag()
  bool GetActiveFlag() const;

  /// \brief Checks whether this component is in an active state.
  ///
  /// The active state is determined by the active state of the owning game object and the 'active flag' of this component.
  /// Only if the owning game object is active (and thus all of its parent objects as well) and the component has the active flag set,
  /// will this component be active.
  ///
  /// \sa ezGameObject::IsActive(), ezGameObject::SetActiveFlag()
  bool IsActive() const;

  /// \brief Returns whether this component is active and initialized.
  ///
  /// \sa IsActive()
  bool IsActiveAndInitialized() const;

  /// \brief Whether the component is currently active and simulation has been started as well.
  ///
  /// \sa IsActive()
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
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const;

  /// \brief Override this to load the current state of the component from the given stream.
  ///
  /// The active state will be automatically serialized. The 'initialized' state is not serialized, all components
  /// will be initialized after creation, even if they were already in an initialized state when they were serialized.
  virtual void DeserializeComponent(ezWorldReader& inout_stream);


  /// \brief Ensures that the component is initialized. Must only be called from another component's Initialize callback.
  void EnsureInitialized();

  /// \brief Ensures that the OnSimulationStarted method has been called. Must only be called from another component's OnSimulationStarted
  /// callback.
  void EnsureSimulationStarted();


  /// \brief Sends a message to this component.
  EZ_ALWAYS_INLINE bool SendMessage(ezMessage& ref_msg) { return SendMessageInternal(ref_msg, false); }
  EZ_ALWAYS_INLINE bool SendMessage(ezMessage& ref_msg) const { return SendMessageInternal(ref_msg, false); }

  /// \brief Queues the message for the given phase. The message is processed after the given delay in the corresponding phase.
  void PostMessage(const ezMessage& msg, ezTime delay = ezTime::MakeZero(), ezObjectMsgQueueType::Enum queueType = ezObjectMsgQueueType::NextFrame) const;

  /// \brief Returns whether the given Message is handled by this component.
  virtual bool HandlesMessage(const ezMessage& msg) const;

  /// Be careful to check which flags may already be in use by base classes.
  void SetUserFlag(ezUInt8 uiFlagIndex, bool bSet);

  /// \brief Retrieves a custom flag. Index must be between 0 and 7.
  bool GetUserFlag(ezUInt8 uiFlagIndex) const;

  /// \brief Adds ezObjectFlags::CreatedByPrefab to the component. See the flag for details.
  void SetCreatedByPrefab() { m_ComponentFlags.Add(ezObjectFlags::CreatedByPrefab); }

  /// \brief Checks whether the ezObjectFlags::CreatedByPrefab flag is set on this component.
  bool WasCreatedByPrefab() const { return m_ComponentFlags.IsSet(ezObjectFlags::CreatedByPrefab); }


  /// \brief Deletes this component. Note that the component will be invalidated first and the actual deletion is postponed.
  void DeleteComponent();

protected:
  friend class ezWorld;
  friend class ezGameObject;
  friend class ezComponentManagerBase;

  /// \brief Returns whether this component is dynamic and thus can only be attached to dynamic game objects.
  bool IsDynamic() const;

  virtual ezWorldModuleTypeId GetTypeId() const = 0;
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
  /// Additionally, OnSimulationStarted() is only executed once, even if the ezWorld pauses and resumes world simulation multiple times.
  /// However, note that it will be called again after the component has been deactivated and is activated again.
  ///
  /// \sa OnActivated(), OnDeactivated(), Initialize(), Deinitialize(), OnSimulationStarted()
  virtual void OnSimulationStarted();

  /// \brief By default disabled. Enable to have OnUnhandledMessage() called for every unhandled message.
  void EnableUnhandledMessageHandler(bool enable);

  /// \brief When EnableUnhandledMessageHandler() was activated, this is called for all messages for which there is no dedicated message handler.
  ///
  /// \return Should return true if the given message was handled, false otherwise.
  virtual bool OnUnhandledMessage(ezMessage& msg, bool bWasPostedMsg);

  /// \brief When EnableUnhandledMessageHandler() was activated, this is called for all messages for which there is no dedicated message handler.
  ///
  /// \return Should return true if the given message was handled, false otherwise.
  virtual bool OnUnhandledMessage(ezMessage& msg, bool bWasPostedMsg) const;

protected:
  /// Messages will be dispatched to this type. Default is what GetDynamicRTTI() returns, can be redirected if necessary.
  const ezRTTI* m_pMessageDispatchType = nullptr;

  bool IsInitialized() const;
  bool IsInitializing() const;
  bool IsSimulationStarted() const;

private:
  // updates the component's active state depending on the owner object's active state
  void UpdateActiveState(bool bOwnerActive);

  ezGameObject* Reflection_GetOwner() const;
  ezWorld* Reflection_GetWorld() const;
  void Reflection_Update(ezTime deltaTime);

  bool SendMessageInternal(ezMessage& msg, bool bWasPostedMsg);
  bool SendMessageInternal(ezMessage& msg, bool bWasPostedMsg) const;

  ezComponentId m_InternalId;
  ezBitflags<ezObjectFlags> m_ComponentFlags = ezObjectFlags::ActiveFlag;
  ezUInt32 m_uiUniqueID = ezInvalidIndex;

  ezComponentManagerBase* m_pManager = nullptr;
  ezGameObject* m_pOwner = nullptr;

  static ezWorldModuleTypeId s_TypeId;
};

struct ezComponent_ScriptBaseClassFunctions
{
  enum Enum
  {
    Initialize,
    Deinitialize,
    OnActivated,
    OnDeactivated,
    OnSimulationStarted,
    Update,

    Count
  };
};

#include <Core/World/Implementation/Component_inl.h>
