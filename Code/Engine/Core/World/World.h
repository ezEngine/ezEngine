#pragma once

#include <Core/World/Implementation/WorldData.h>

struct ezEventMessage;
class ezEventMessageHandlerComponent;

/// \brief A world encapsulates a scene graph of game objects and various component managers and their components.
///
/// There can be multiple worlds active at a time, but only 64 at most. The world manages all object storage and might move objects around
/// in memory. Thus it is not allowed to store pointers to objects. They should be referenced by handles.\n The world has a multi-phase
/// update mechanism which is divided in the following phases:\n
/// * Pre-async phase: The corresponding component manager update functions are called synchronously in the order of their dependencies.
/// * Async phase: The update functions are called in batches asynchronously on multiple threads. There is absolutely no guarantee in which
/// order the functions are called.
///   Thus it is not allowed to access any data other than the components own data during that phase.
/// * Post-async phase: Another synchronous phase like the pre-async phase.
/// * Actual deletion of dead objects and components are done now.
/// * Transform update: The world transformation of all dynamic objects is updated.
/// * Post-transform phase: Another synchronous phase like the pre-async phase after the transformation has been updated.
class EZ_CORE_DLL ezWorld
{
public:
  /// \brief Creates a new world with the given name.
  ezWorld(ezWorldDesc& desc);
  ~ezWorld();

  /// \brief Deletes all game objects in a world
  void Clear();

  /// \brief Returns the name of this world.
  const char* GetName() const;

  /// \brief Returns the index of this world.
  ezUInt16 GetIndex() const;

  /// \name Object Functions
  ///@{

  /// \brief Create a new game object from the given description and returns a handle to it.
  ezGameObjectHandle CreateObject(const ezGameObjectDesc& desc);

  /// \brief Create a new game object from the given description, writes a pointer to it to out_pObject and returns a handle to it.
  ezGameObjectHandle CreateObject(const ezGameObjectDesc& desc, ezGameObject*& out_pObject);

  /// \brief Deletes the given object, its children and all components.
  /// \note This function deletes the object immediately! It is unsafe to use this during a game update loop, as other objects
  /// may rely on this object staying valid for the rest of the frame.
  /// Use DeleteObjectDelayed() instead for safe removal at the end of the frame.
  void DeleteObjectNow(const ezGameObjectHandle& object);

  /// \brief Deletes the given object at the beginning of the next world update. The object and its components and children stay completely
  /// valid until then.
  void DeleteObjectDelayed(const ezGameObjectHandle& object);

  /// \brief Returns whether the given handle corresponds to a valid object.
  bool IsValidObject(const ezGameObjectHandle& object) const;

  /// \brief Returns if an object with the given handle exists and if so writes out the corresponding pointer to out_pObject.
  bool TryGetObject(const ezGameObjectHandle& object, ezGameObject*& out_pObject);

  /// \brief Returns if an object with the given handle exists and if so writes out the corresponding pointer to out_pObject.
  bool TryGetObject(const ezGameObjectHandle& object, const ezGameObject*& out_pObject) const;

  /// \brief Returns if an object with the given global key exists and if so writes out the corresponding pointer to out_pObject.
  bool TryGetObjectWithGlobalKey(const ezTempHashedString& sGlobalKey, ezGameObject*& out_pObject);

  /// \brief Returns if an object with the given global key exists and if so writes out the corresponding pointer to out_pObject.
  bool TryGetObjectWithGlobalKey(const ezTempHashedString& sGlobalKey, const ezGameObject*& out_pObject) const;


  /// \brief Returns the total number of objects in this world.
  ezUInt32 GetObjectCount() const;

  /// \brief Returns an iterator over all objects in this world in no specific order.
  ezInternal::WorldData::ObjectIterator GetObjects();

  /// \brief Returns an iterator over all objects in this world in no specific order.
  ezInternal::WorldData::ConstObjectIterator GetObjects() const;

  /// \brief Defines a visitor function that is called for every game-object when using the traverse method.
  /// The function takes a pointer to the game object as argument and returns a bool which indicates whether to continue (true) or abort
  /// (false) traversal.
  typedef ezInternal::WorldData::VisitorFunc VisitorFunc;

  enum TraversalMethod
  {
    BreadthFirst,
    DepthFirst
  };

  /// \brief Traverses the game object tree starting at the top level objects and then recursively all children. The given callback function
  /// is called for every object.
  void Traverse(VisitorFunc visitorFunc, TraversalMethod method = DepthFirst);

  ///@}
  /// \name Module Functions
  ///@{

  /// \brief Creates an instance of the given module type or derived type or returns a pointer to an already existing instance.
  template <typename ModuleType>
  ModuleType* GetOrCreateModule();

  /// \brief Creates an instance of the given module type or derived type or returns a pointer to an already existing instance.
  ezWorldModule* GetOrCreateModule(const ezRTTI* pRtti);

  /// \brief Deletes the module of the given type or derived types.
  template <typename ModuleType>
  void DeleteModule();

  /// \brief Deletes the module of the given type or derived types.
  void DeleteModule(const ezRTTI* pRtti);

  /// \brief Returns the instance to the given module type or derived types.
  template <typename ModuleType>
  ModuleType* GetModule();

  /// \brief Returns the instance to the given module type or derived types.
  template <typename ModuleType>
  const ModuleType* GetModule() const;

  /// \brief Returns the instance to the given module type or derived types.
  ezWorldModule* GetModule(const ezRTTI* pRtti);

  /// \brief Returns the instance to the given module type or derived types.
  const ezWorldModule* GetModule(const ezRTTI* pRtti) const;

  ///@}
  /// \name Component Functions
  ///@{

  /// \brief Creates an instance of the given component manager type or returns a pointer to an already existing instance.
  template <typename ManagerType>
  ManagerType* GetOrCreateComponentManager();

  /// \brief Returns the component manager that handles the given rtti component type.
  ezComponentManagerBase* GetOrCreateManagerForComponentType(const ezRTTI* pComponentRtti);

  /// \brief Deletes the component manager of the given type and all its components.
  template <typename ManagerType>
  void DeleteComponentManager();

  /// \brief Returns the instance to the given component manager type.
  template <typename ManagerType>
  ManagerType* GetComponentManager();

  /// \brief Returns the instance to the given component manager type.
  template <typename ManagerType>
  const ManagerType* GetComponentManager() const;

  /// \brief Returns the component manager that handles the given rtti component type.
  ezComponentManagerBase* GetManagerForComponentType(const ezRTTI* pComponentRtti);

  /// \brief Returns the component manager that handles the given rtti component type.
  const ezComponentManagerBase* GetManagerForComponentType(const ezRTTI* pComponentRtti) const;

  /// \brief Checks whether the given handle references a valid component.
  bool IsValidComponent(const ezComponentHandle& component) const;

  /// \brief Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  template <typename ComponentType>
  bool TryGetComponent(const ezComponentHandle& component, ComponentType*& out_pComponent);

  /// \brief Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  template <typename ComponentType>
  bool TryGetComponent(const ezComponentHandle& component, const ComponentType*& out_pComponent) const;

  /// \brief Creates a new component init batch.
  /// It is ensured that the Initialize function is called for all components in a batch before the OnSimulationStarted is called.
  /// If bMustFinishWithinOneFrame is set to false the processing of an init batch can be distributed over multiple frames if
  /// m_MaxComponentInitializationTimePerFrame in the world desc is set to a reasonable value.
  ezComponentInitBatchHandle CreateComponentInitBatch(const char* szBatchName, bool bMustFinishWithinOneFrame = true);

  /// \brief Deletes a component init batch. It must be completely processed before it can be deleted.
  void DeleteComponentInitBatch(const ezComponentInitBatchHandle& batch);

  /// \brief All components that are created between an BeginAddingComponentsToInitBatch/EndAddingComponentsToInitBatch scope are added to the
  /// given init batch.
  void BeginAddingComponentsToInitBatch(const ezComponentInitBatchHandle& batch);

  /// \brief End adding components to the given batch. Components created after this call are added to the default init batch.
  void EndAddingComponentsToInitBatch(const ezComponentInitBatchHandle& batch);

  /// \brief After all components have been added to the init batch call submit to start processing the batch.
  void SubmitComponentInitBatch(const ezComponentInitBatchHandle& batch);

  /// \brief Returns whether the init batch has been completely processed and all corresponding components are initialized
  /// and their OnSimulationStarted function was called.
  bool IsComponentInitBatchCompleted(const ezComponentInitBatchHandle& batch, double* pCompletionFactor = nullptr);

  ///@}
  /// \name Message Functions
  ///@{

  /// \brief Sends a message to all components of the receiverObject.
  void SendMessage(const ezGameObjectHandle& receiverObject, ezMessage& msg);

  /// \brief Sends a message to all components of the receiverObject and all its children.
  void SendMessageRecursive(const ezGameObjectHandle& receiverObject, ezMessage& msg);

  /// \brief Queues the message for the given phase. The message is send to the receiverObject after the given delay in the corresponding phase.
  void PostMessage(const ezGameObjectHandle& receiverObject, const ezMessage& msg, ezObjectMsgQueueType::Enum queueType, ezTime delay = ezTime()) const;

  /// \brief Queues the message for the given phase. The message is send to the receiverObject and all its children after the given delay in
  /// the corresponding phase.
  void PostMessageRecursive(const ezGameObjectHandle& receiverObject, const ezMessage& msg, ezObjectMsgQueueType::Enum queueType, ezTime delay = ezTime()) const;

  /// \brief Sends a message to the component.
  void SendMessage(const ezComponentHandle& receiverComponent, ezMessage& msg);

  /// \brief Queues the message for the given phase. The message is send to the receiverComponent after the given delay in the corresponding phase.
  void PostMessage(const ezComponentHandle& receiverComponent, const ezMessage& msg, ezObjectMsgQueueType::Enum queueType, ezTime delay = ezTime()) const;

  /// \brief Finds the closest (parent) object, starting at pSearchObject, which has an ezEventMessageHandlerComponent.
  ///
  /// If any such parent object exists, the search is stopped there, even if that component does not handle messages of the given type.
  /// If no such parent object exists, it searches for a ezEventMessageHandlerComponent instance that is set to 'handle global events'
  /// that handles messages of the given type.
  ///
  /// \returns A non-null pointer if an event handler component was found that also handles this type of message. nullptr otherwise.
  const ezComponent* FindEventMsgHandler(ezEventMessage& msg, const ezGameObject* pSearchObject) const;

  ///@}

  /// \brief If enabled, the full simulation should be executed, otherwise only the rendering related updates should be done
  void SetWorldSimulationEnabled(bool bEnable);

  /// \brief If enabled, the full simulation should be executed, otherwise only the rendering related updates should be done
  bool GetWorldSimulationEnabled() const;

  /// \brief Updates the world by calling the various update methods on the component managers and also updates the transformation data of
  /// the game objects. See ezWorld for a detailed description of the update phases.
  void Update();

  /// \brief Returns a task implementation that calls Update on this world.
  ezTask* GetUpdateTask();


  /// \brief Returns the spatial system that is associated with this world.
  ezSpatialSystem* GetSpatialSystem();

  /// \brief Returns the spatial system that is associated with this world.
  const ezSpatialSystem* GetSpatialSystem() const;


  /// \brief Returns the coordinate system for the given position.
  /// By default this always returns a coordinate system with forward = +X, right = +Y and up = +Z.
  /// This can be customized by setting a different coordinate system provider.
  void GetCoordinateSystem(const ezVec3& vGlobalPosition, ezCoordinateSystem& out_CoordinateSystem) const;

  /// \brief Sets the coordinate system provider that should be used in this world.
  void SetCoordinateSystemProvider(const ezSharedPtr<ezCoordinateSystemProvider>& pProvider);

  /// \brief Returns the coordinate system provider that is associated with this world.
  ezCoordinateSystemProvider& GetCoordinateSystemProvider();

  /// \brief Returns the coordinate system provider that is associated with this world.
  const ezCoordinateSystemProvider& GetCoordinateSystemProvider() const;


  /// \brief Returns the clock that is used for all updates in this game world
  ezClock& GetClock();

  /// \brief Returns the clock that is used for all updates in this game world
  const ezClock& GetClock() const;

  /// \brief Accesses the default random number generator.
  /// If more control is desired, individual components should use their own RNG.
  ezRandom& GetRandomNumberGenerator();


  /// \brief Returns the allocator used by this world.
  ezAllocatorBase* GetAllocator();

  /// \brief Returns the block allocator used by this world.
  ezInternal::WorldLargeBlockAllocator* GetBlockAllocator();

  /// \brief Returns the stack allocator used by this world.
  ezDoubleBufferedStackAllocator* GetStackAllocator();

  /// \brief Mark the world for reading by using EZ_LOCK(world.GetReadMarker()). Multiple threads can read simultaneously if none is
  /// writing.
  ezInternal::WorldData::ReadMarker& GetReadMarker() const;

  /// \brief Mark the world for writing by using EZ_LOCK(world.GetWriteMarker()). Only one thread can write at a time.
  ezInternal::WorldData::WriteMarker& GetWriteMarker();


  /// \brief Associates the given user data with the world. The user is responsible for the life time of user data.
  void SetUserData(void* pUserData);

  /// \brief Returns the associated user data.
  void* GetUserData() const;

  using ReferenceResolver = ezDelegate<ezGameObjectHandle(const void*, ezComponentHandle hThis, const char* szProperty)>;

  /// \brief If set, this delegate can be used to map some data (GUID or string) to an ezGameObjectHandle.
  ///
  /// Currently only used in editor settings, to create a runtime handle from a unique editor reference.
  void SetGameObjectReferenceResolver(const ReferenceResolver& resolver);

  /// \sa SetGameObjectReferenceResolver()
  const ReferenceResolver& GetGameObjectReferenceResolver() const;

  /// \name Helper methods to query ezWorld limits
  ///@{
  static constexpr ezUInt64 GetMaxNumGameObjects();
  static constexpr ezUInt64 GetMaxNumHierarchyLevels();
  static constexpr ezUInt64 GetMaxNumComponentsPerType();
  static constexpr ezUInt64 GetMaxNumWorldModules();
  static constexpr ezUInt64 GetMaxNumComponentTypes();
  static constexpr ezUInt64 GetMaxNumWorlds();
  ///@}

public:
  /// \brief Returns the number of active worlds.
  static ezUInt32 GetWorldCount();

  /// \brief Returns the world with the given index.
  static ezWorld* GetWorld(ezUInt32 uiIndex);

  /// \brief Returns the world for the given game object handle.
  static ezWorld* GetWorld(const ezGameObjectHandle& object);

  /// \brief Returns the world for the given component handle.
  static ezWorld* GetWorld(const ezComponentHandle& component);

private:
  friend class ezGameObject;
  friend class ezWorldModule;
  friend class ezComponentManagerBase;
  friend class ezComponent;

  void CheckForReadAccess() const;
  void CheckForWriteAccess() const;

  ezGameObject* GetObjectUnchecked(ezUInt32 uiIndex) const;

  void SetParent(ezGameObject* pObject, ezGameObject* pNewParent, ezGameObject::TransformPreservation preserve = ezGameObject::TransformPreservation::PreserveGlobal);
  void LinkToParent(ezGameObject* pObject);
  void UnlinkFromParent(ezGameObject* pObject);

  void SetObjectGlobalKey(ezGameObject* pObject, const ezHashedString& sGlobalKey);
  const char* GetObjectGlobalKey(const ezGameObject* pObject) const;

  void PostMessage(const ezGameObjectHandle& receiverObject, const ezMessage& msg, ezObjectMsgQueueType::Enum queueType, ezTime delay, bool bRecursive) const;
  void ProcessQueuedMessage(const ezInternal::WorldData::MessageQueue::Entry& entry);
  void ProcessQueuedMessages(ezObjectMsgQueueType::Enum queueType);

  void RegisterUpdateFunction(const ezWorldModule::UpdateFunctionDesc& desc);
  void DeregisterUpdateFunction(const ezWorldModule::UpdateFunctionDesc& desc);
  void DeregisterUpdateFunctions(ezWorldModule* pModule);

  /// \brief Used by component managers to queue a new component for initialization during the next update
  void AddComponentToInitialize(ezComponentHandle hComponent);

  void UpdateFromThread();
  void UpdateSynchronous(const ezArrayPtr<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions);
  void UpdateAsynchronous();

  // returns if the batch was completely initialized
  bool ProcessInitializationBatch(ezInternal::WorldData::InitBatch& batch, ezTime endTime);
  void ProcessComponentsToInitialize();
  void ProcessUpdateFunctionsToRegister();
  ezResult RegisterUpdateFunctionInternal(const ezWorldModule::UpdateFunctionDesc& desc);

  void DeleteDeadObjects();
  void DeleteDeadComponents();

  void PatchHierarchyData(ezGameObject* pObject, ezGameObject::TransformPreservation preserve);
  void RecreateHierarchyData(ezGameObject* pObject, bool bWasDynamic);

  bool ReportErrorWhenStaticObjectMoves() const;

  ezDelegateTask<void> m_UpdateTask;

  ezInternal::WorldData m_Data;

  typedef ezInternal::WorldData::QueuedMsgMetaData QueuedMsgMetaData;

  ezUInt16 m_uiIndex;
  static ezStaticArray<ezWorld*, 256> s_Worlds;
};

#include <Core/World/Implementation/World_inl.h>
