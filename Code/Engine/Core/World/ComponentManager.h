#pragma once

#include <Foundation/Types/Delegate.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Memory/BlockStorage.h>
#include <Foundation/Reflection/Reflection.h>

#include <Core/World/Declarations.h>
#include <Core/World/Component.h>

/// \brief Base class for all component managers. Do not derive directly from this class, but derive from ezComponentManager instead.
///
/// Every component type has its corresponding manager type. The manager stores the components in memory blocks to minimize overhead 
/// on creation and deletion of components. Each manager can also register update functions to update its components during
/// the different update phases of ezWorld.
/// Use ezWorld::CreateComponentManager to create an instance of a component manager within a specific world.
class EZ_CORE_DLL ezComponentManagerBase
{
protected:
  ezComponentManagerBase(ezWorld* pWorld);
  virtual ~ezComponentManagerBase();
  
public:
  /// \brief Returns the corresponding world to this manager.
  ezWorld* GetWorld();

  /// \brief Returns the corresponding world to this manager.
  const ezWorld* GetWorld() const;

  /// \brief Checks whether the given handle references a valid component.
  bool IsValidComponent(const ezComponentHandle& component) const;

  /// \brief Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const ezComponentHandle& component, ezComponent*& out_pComponent);

  /// \brief Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const ezComponentHandle& component, const ezComponent*& out_pComponent) const;

  /// \brief Returns the number of components managed by this manager.
  ezUInt32 GetComponentCount() const;

  /// \brief Create a new component instance and returns a handle to it. This method is implemented by ezComponentManager.
  virtual ezComponentHandle AllocateComponent() = 0;

  /// \brief Deletes the given component. Note that the component will be invalidated first and the actual deletion is postponed.
  void DeleteComponent(const ezComponentHandle& component);

  /// \brief Returns the rtti info of the component type that this manager handles.
  virtual const ezRTTI* GetComponentType() const = 0;

  void SetUserData(void* pUserData) { m_pUserData = pUserData; }

  void* GetUserData() const { return m_pUserData; }

protected:
  friend class ezWorld;
  friend class ezInternal::WorldData;
  friend class ezMemoryUtils;

  /// \brief Update function delegate. The first parameter is the first index to the components that should be updated. The second parameter is the number of components that should be updated.
  typedef ezDelegate<void (ezUInt32, ezUInt32)> UpdateFunction;

  /// \brief Description of an update function that can be registered at the world.
  struct UpdateFunctionDesc
  {
    enum Phase
    {
      PreAsync,
      Async,
      PostAsync,
      PostTransform,
      PHASE_COUNT
    };

    UpdateFunctionDesc(const UpdateFunction& function, const char* szFunctionName)
    {
      m_Function = function;
      m_szFunctionName = szFunctionName;
      m_Phase = PreAsync;
      m_uiGranularity = 0;
    }

    UpdateFunction m_Function;                    ///< Delegate to the actual update function.
    const char* m_szFunctionName;                 ///< Name of the function. Use the EZ_CREATE_COMPONENT_UPDATE_FUNCTION_DESC macro to create a description with the correct name.
    ezHybridArray<UpdateFunction, 4> m_DependsOn; ///< Array of other functions on which this function depends on. This function will be called after all its dependencies have been called.
    Phase m_Phase;                                ///< The update phase in which this update function should be called. See ezWorld for a description on the different phases.
    ezUInt32 m_uiGranularity;                     ///< The granularity in which batch updates should happen during the asynchronous phase. Has to be 0 for synchronous functions.
  };

  /// \brief Registers the given update function at the world.
  void RegisterUpdateFunction(const UpdateFunctionDesc& desc);

  /// \brief Deregisters the given update function from the world. Note that only the m_Function and the m_Phase of the description have to be valid for deregistration.
  void DeregisterUpdateFunction(const UpdateFunctionDesc& desc);

  /// \brief Returns the allocator used by the world.
  ezAllocatorBase* GetAllocator();

  /// \brief Returns the block allocator used by the world.
  ezInternal::WorldLargeBlockAllocator* GetBlockAllocator();

protected:
  /// \cond
  // internal methods
  typedef ezBlockStorage<ezComponent, ezInternal::DEFAULT_BLOCK_SIZE, false>::Entry ComponentStorageEntry;

  ezComponentHandle CreateComponentEntry(ComponentStorageEntry storageEntry, ezUInt16 uiTypeId);
  void DeinitializeComponent(ezComponent* pComponent);
  void DeleteComponentEntry(ComponentStorageEntry storageEntry);
  virtual void DeleteDeadComponent(ComponentStorageEntry storageEntry, ezComponent*& out_pMovedComponent);

  static ezComponentId GetIdFromHandle(const ezComponentHandle& component);
  static ezComponentHandle GetHandle(ezGenericComponentId internalId, ezUInt16 uiTypeId);

  /// \endcond

  ezIdTable<ezGenericComponentId, ComponentStorageEntry> m_Components;

private:
  /// \brief This method is called after the constructor. A derived type can override this method to do initialization work. Typically this is the method where updates function are registered.
  virtual ezResult Initialize() { return EZ_SUCCESS; }

  /// \brief This method is called before the destructor. A derived type can override this method to do deinitialization work.
  virtual ezResult Deinitialize() { return EZ_SUCCESS; }

  ezWorld* m_pWorld;

  void* m_pUserData;
};

template <typename T, bool CompactStorage = false>
class ezComponentManager : public ezComponentManagerBase
{
public:
  typedef T ComponentType;

  /// \brief Although the constructor is public always use ezWorld::CreateComponentManager to create an instance.
  ezComponentManager(ezWorld* pWorld);
  virtual ~ezComponentManager();

  /// \brief Create a new component instance and returns a handle to it.
  virtual ezComponentHandle AllocateComponent() override;

  /// \brief Create a new component instance and returns a handle to it and writes a pointer to out_pComponent.
  ezComponentHandle CreateComponent(ComponentType*& out_pComponent);

  /// \brief Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const ezComponentHandle& component, ComponentType*& out_pComponent);

  /// \brief Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const ezComponentHandle& component, const ComponentType*& out_pComponent) const;

  /// \brief Returns an iterator over all components.
  typename ezBlockStorage<ComponentType, ezInternal::DEFAULT_BLOCK_SIZE, CompactStorage>::Iterator GetComponents();

  /// \brief Returns an iterator over all components.
  typename ezBlockStorage<ComponentType, ezInternal::DEFAULT_BLOCK_SIZE, CompactStorage>::ConstIterator GetComponents() const;

  /// \brief Returns the rtti info of the component type that this manager handles.
  virtual const ezRTTI* GetComponentType() const override;

  /// \brief Returns the type id corresponding to the component type managed by this manager.
  static ezUInt16 TypeId();

protected:
  friend ComponentType;

  virtual void DeleteDeadComponent(ComponentStorageEntry storageEntry, ezComponent*& out_pMovedComponent) override;

  void RegisterUpdateFunction(UpdateFunctionDesc& desc);

  ezBlockStorage<ComponentType, ezInternal::DEFAULT_BLOCK_SIZE, CompactStorage> m_ComponentStorage;
};


//////////////////////////////////////////////////////////////////////////


/// \brief Simple component manager implementation that calls an update method on all components every frame.
template <typename ComponentType, bool OnlyUpdateWhenSimulating>
class ezComponentManagerSimple : public ezComponentManager<ComponentType>
{
public:
  ezComponentManagerSimple(ezWorld* pWorld);

  virtual ezResult Initialize() override;

  /// \brief A simple update function that iterates over all components and calls Update() on every component
  void SimpleUpdate(ezUInt32 uiStartIndex, ezUInt32 uiCount);
};


//////////////////////////////////////////////////////////////////////////


/// \brief A component manager that does no update at all on components and expects only a single instance to be created per world.
///
/// Easy access to this single function is provided through the GetSingletonComponent() function.
/// If a second component is created, the manager will log an error. The first created component will be used as the 'singleton',
/// all other components are ignored.
/// Use this for components derived from ezSettingsComponent, of which one should only of zero or one per world.
template <typename ComponentType>
class ezSettingsComponentManager : public ezComponentManager<ComponentType>
{
public:
  ezSettingsComponentManager(ezWorld* pWorld) : ezComponentManager<ComponentType>(pWorld)
  {
    m_pSingleton = nullptr;
  }

  virtual ezComponentHandle AllocateComponent() override;
  virtual void DeleteDeadComponent(ComponentStorageEntry storageEntry, ezComponent*& out_pMovedComponent) override;

  /// \brief Returns the first component of this type that has been created.
  ComponentType* GetSingletonComponent() const { return m_pSingleton; }

private:
  ComponentType* m_pSingleton;
};


//////////////////////////////////////////////////////////////////////////


/// \brief Helper class to get component type ids and create new instances of component managers from rtti.
class EZ_CORE_DLL ezComponentManagerFactory
{
public:
  template <typename ComponentType>
  static ezUInt16 RegisterComponentManager();

  /// \brief Returns the component type id to the given rtti component type.
  static ezUInt16 GetTypeId(const ezRTTI* pRtti);

  /// \brief Creates a new instance of the component manager with the given type id and world.
  static ezComponentManagerBase* CreateComponentManager(ezUInt16 typeId, ezWorld* pWorld);

private:
  static ezUInt16 s_uiNextTypeId;
  static ezHashTable<const ezRTTI*, ezUInt16> s_TypeToId;

  typedef ezComponentManagerBase* (*CreatorFunc)(ezAllocatorBase*, ezWorld*);
  static ezDynamicArray<CreatorFunc> s_CreatorFuncs;
};

/// \brief Add this macro to a custom component type inside the type declaration.
#define EZ_DECLARE_COMPONENT_TYPE(componentType, baseType, managerType) \
  EZ_ADD_DYNAMIC_REFLECTION(componentType, baseType); \
  public: \
    typedef managerType ComponentManagerType; \
    ezComponentHandle GetHandle() const; \
    ComponentManagerType* GetManager() const; \
    virtual ezUInt16 GetTypeId() const override { return TYPE_ID; } \
    static EZ_FORCE_INLINE ezUInt16 TypeId() { return TYPE_ID; } \
    static ezComponentHandle CreateComponent(ezWorld* pWorld, componentType*& pComponent); \
  private: \
    friend managerType; \
    static ezUInt16 TYPE_ID;

/// \brief Add this macro to a custom abstract component type inside the type declaration.
#define EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(componentType, baseType) \
  EZ_ADD_DYNAMIC_REFLECTION(componentType, baseType); \
  public: \
    virtual ezUInt16 GetTypeId() const override { return -1; } \
    static EZ_FORCE_INLINE ezUInt16 TypeId() { return -1; }


/// \brief Implements rtti and component specific functionality. Add this macro to a cpp file.
///
/// \see EZ_BEGIN_DYNAMIC_REFLECTED_TYPE
#define EZ_BEGIN_COMPONENT_TYPE(componentType, version) \
  ezUInt16 componentType::TYPE_ID = ezComponentManagerFactory::RegisterComponentManager<componentType>(); \
  ezComponentHandle componentType::GetHandle() const { return ezComponent::GetHandle<componentType>(); } \
  componentType::ComponentManagerType* componentType::GetManager() const { return static_cast<componentType::ComponentManagerType*>(ezComponent::GetManager()); } \
  ezComponentHandle componentType::CreateComponent(ezWorld* pWorld, componentType*& pComponent) { return pWorld->GetOrCreateComponentManager<ComponentManagerType>()->CreateComponent(pComponent); } \
  EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(componentType, version, ezRTTINoAllocator);

/// \brief Implements rtti and abstract component specific functionality. Add this macro to a cpp file.
///
/// \see EZ_BEGIN_DYNAMIC_REFLECTED_TYPE
#define EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(componentType, version) EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(componentType, version, ezRTTINoAllocator);

/// \brief Ends the component implementation code block that was opened with EZ_BEGIN_COMPONENT_TYPE.
#define EZ_END_COMPONENT_TYPE EZ_END_DYNAMIC_REFLECTED_TYPE
#define EZ_END_ABSTRACT_COMPONENT_TYPE EZ_END_DYNAMIC_REFLECTED_TYPE

/// \brief Helper macro to create an update function description with proper name
#define EZ_CREATE_COMPONENT_UPDATE_FUNCTION_DESC(func, instance) \
  ezComponentManagerBase::UpdateFunctionDesc(ezComponentManagerBase::UpdateFunction(&func, instance), #func)

#include <Core/World/Implementation/ComponentManager_inl.h>

