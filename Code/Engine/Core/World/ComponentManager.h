#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/BlockStorage.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Delegate.h>

#include <Core/World/Component.h>
#include <Core/World/Declarations.h>
#include <Core/World/WorldModule.h>

/// \brief Base class for all component managers. Do not derive directly from this class, but derive from ezComponentManager instead.
///
/// Every component type has its corresponding manager type. The manager stores the components in memory blocks to minimize overhead
/// on creation and deletion of components. Each manager can also register update functions to update its components during
/// the different update phases of ezWorld.
/// Use ezWorld::CreateComponentManager to create an instance of a component manager within a specific world.
class EZ_CORE_DLL ezComponentManagerBase : public ezWorldModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezComponentManagerBase, ezWorldModule);

protected:
  ezComponentManagerBase(ezWorld* pWorld);
  virtual ~ezComponentManagerBase();

public:
  /// \brief Checks whether the given handle references a valid component.
  bool IsValidComponent(const ezComponentHandle& hComponent) const;

  /// \brief Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const ezComponentHandle& hComponent, ezComponent*& out_pComponent);

  /// \brief Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const ezComponentHandle& hComponent, const ezComponent*& out_pComponent) const;

  /// \brief Returns the number of components managed by this manager.
  ezUInt32 GetComponentCount() const;

  /// \brief Create a new component instance and returns a handle to it.
  ezComponentHandle CreateComponent(ezGameObject* pOwnerObject);

  /// \brief Create a new component instance and returns a handle to it.
  template <typename ComponentType>
  ezTypedComponentHandle<ComponentType> CreateComponent(ezGameObject* pOwnerObject, ComponentType*& out_pComponent);

  /// \brief Deletes the given component. Note that the component will be invalidated first and the actual deletion is postponed.
  void DeleteComponent(const ezComponentHandle& hComponent);

  /// \brief Deletes the given component. Note that the component will be invalidated first and the actual deletion is postponed.
  void DeleteComponent(ezComponent* pComponent);

  /// \brief Adds all components that this manager handles to the given array (array is not cleared).
  /// Prefer to use more efficient methods on derived classes, only use this if you need to go through a ezComponentManagerBase pointer.
  virtual void CollectAllComponents(ezDynamicArray<ezComponentHandle>& out_allComponents, bool bOnlyActive) = 0;

  /// \brief Adds all components that this manager handles to the given array (array is not cleared).
  /// Prefer to use more efficient methods on derived classes, only use this if you need to go through a ezComponentManagerBase pointer.
  virtual void CollectAllComponents(ezDynamicArray<ezComponent*>& out_allComponents, bool bOnlyActive) = 0;

protected:
  /// \cond
  // internal methods
  friend class ezWorld;
  friend class ezInternal::WorldData;

  virtual void Deinitialize() override;

protected:
  friend class ezWorldReader;

  ezComponentHandle CreateComponentNoInit(ezGameObject* pOwnerObject, ezComponent*& out_pComponent);
  void InitializeComponent(ezComponent* pComponent);
  void DeinitializeComponent(ezComponent* pComponent);
  void PatchIdTable(ezComponent* pComponent);

  virtual ezComponent* CreateComponentStorage() = 0;
  virtual void DeleteComponentStorage(ezComponent* pComponent, ezComponent*& out_pMovedComponent) = 0;

  /// \endcond

  ezIdTable<ezComponentId, ezComponent*> m_Components;
};

template <typename T, ezBlockStorageType::Enum StorageType>
class ezComponentManager : public ezComponentManagerBase
{
public:
  using ComponentType = T;
  using SUPER = ezComponentManagerBase;

  /// \brief Although the constructor is public always use ezWorld::CreateComponentManager to create an instance.
  ezComponentManager(ezWorld* pWorld);
  virtual ~ezComponentManager();

  /// \brief Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const ezComponentHandle& hComponent, ComponentType*& out_pComponent);

  /// \brief Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const ezComponentHandle& hComponent, const ComponentType*& out_pComponent) const;

  /// \brief Returns an iterator over all components.
  typename ezBlockStorage<ComponentType, ezInternal::DEFAULT_BLOCK_SIZE, StorageType>::Iterator GetComponents(ezUInt32 uiStartIndex = 0);

  /// \brief Returns an iterator over all components.
  typename ezBlockStorage<ComponentType, ezInternal::DEFAULT_BLOCK_SIZE, StorageType>::ConstIterator GetComponents(ezUInt32 uiStartIndex = 0) const;

  /// \brief Returns the type id corresponding to the component type managed by this manager.
  static ezWorldModuleTypeId TypeId();

  virtual void CollectAllComponents(ezDynamicArray<ezComponentHandle>& out_allComponents, bool bOnlyActive) override;
  virtual void CollectAllComponents(ezDynamicArray<ezComponent*>& out_allComponents, bool bOnlyActive) override;

protected:
  friend ComponentType;
  friend class ezComponentManagerFactory;

  virtual ezComponent* CreateComponentStorage() override;
  virtual void DeleteComponentStorage(ezComponent* pComponent, ezComponent*& out_pMovedComponent) override;

  void RegisterUpdateFunction(UpdateFunctionDesc& desc);

  ezBlockStorage<ComponentType, ezInternal::DEFAULT_BLOCK_SIZE, StorageType> m_ComponentStorage;
};


//////////////////////////////////////////////////////////////////////////

struct ezComponentUpdateType
{
  enum Enum
  {
    Always,
    WhenSimulating
  };
};

/// \brief Simple component manager implementation that calls an update method on all components every frame.
template <typename ComponentType, ezComponentUpdateType::Enum UpdateType, ezBlockStorageType::Enum StorageType = ezBlockStorageType::FreeList, ezWorldUpdatePhase::Enum UpdatePhase = ezWorldUpdatePhase::PreAsync>
class ezComponentManagerSimple final : public ezComponentManager<ComponentType, StorageType>
{
public:
  ezComponentManagerSimple(ezWorld* pWorld);

  virtual void Initialize() override;

  /// \brief A simple update function that iterates over all components and calls Update() on every component
  void SimpleUpdate(const ezWorldModule::UpdateContext& context);

private:
  static void SimpleUpdateName(ezStringBuilder& out_sName);
};

//////////////////////////////////////////////////////////////////////////

#define EZ_ADD_COMPONENT_FUNCTIONALITY(componentType, baseType, managerType)                                            \
public:                                                                                                                 \
  using ComponentManagerType = managerType;                                                                             \
  virtual ezWorldModuleTypeId GetTypeId() const override                                                                \
  {                                                                                                                     \
    return s_TypeId;                                                                                                    \
  }                                                                                                                     \
  static EZ_ALWAYS_INLINE ezWorldModuleTypeId TypeId()                                                                  \
  {                                                                                                                     \
    return s_TypeId;                                                                                                    \
  }                                                                                                                     \
  ezTypedComponentHandle<componentType> GetHandle() const                                                               \
  {                                                                                                                     \
    return ezTypedComponentHandle<componentType>(ezComponent::GetHandle());                                             \
  }                                                                                                                     \
  virtual ezComponentMode::Enum GetMode() const override;                                                               \
  static ezTypedComponentHandle<componentType> CreateComponent(ezGameObject* pOwnerObject, componentType*& pComponent); \
                                                                                                                        \
private:                                                                                                                \
  friend managerType;                                                                                                   \
  static ezWorldModuleTypeId s_TypeId

#define EZ_ADD_ABSTRACT_COMPONENT_FUNCTIONALITY(componentType, baseType) \
public:                                                                  \
  virtual ezWorldModuleTypeId GetTypeId() const override                 \
  {                                                                      \
    return ezWorldModuleTypeId(-1);                                      \
  }                                                                      \
  static EZ_ALWAYS_INLINE ezWorldModuleTypeId TypeId()                   \
  {                                                                      \
    return ezWorldModuleTypeId(-1);                                      \
  }

/// \brief Add this macro to a custom component type inside the type declaration.
#define EZ_DECLARE_COMPONENT_TYPE(componentType, baseType, managerType) \
  EZ_ADD_DYNAMIC_REFLECTION(componentType, baseType);                   \
  EZ_ADD_COMPONENT_FUNCTIONALITY(componentType, baseType, managerType);

/// \brief Add this macro to a custom abstract component type inside the type declaration.
#define EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(componentType, baseType) \
  EZ_ADD_DYNAMIC_REFLECTION(componentType, baseType);               \
  EZ_ADD_ABSTRACT_COMPONENT_FUNCTIONALITY(componentType, baseType);


/// \brief Implements rtti and component specific functionality. Add this macro to a cpp file.
///
/// \see EZ_BEGIN_DYNAMIC_REFLECTED_TYPE
#define EZ_BEGIN_COMPONENT_TYPE(componentType, version, mode)                                                                            \
  ezWorldModuleTypeId componentType::s_TypeId =                                                                                          \
    ezWorldModuleFactory::GetInstance()->RegisterWorldModule<typename componentType::ComponentManagerType, componentType>();             \
  ezComponentMode::Enum componentType::GetMode() const                                                                                   \
  {                                                                                                                                      \
    return mode;                                                                                                                         \
  }                                                                                                                                      \
  ezTypedComponentHandle<componentType> componentType::CreateComponent(ezGameObject* pOwnerObject, componentType*& out_pComponent)       \
  {                                                                                                                                      \
    return pOwnerObject->GetWorld()->GetOrCreateComponentManager<ComponentManagerType>()->CreateComponent(pOwnerObject, out_pComponent); \
  }                                                                                                                                      \
  EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(componentType, version, ezRTTINoAllocator)

/// \brief Implements rtti and abstract component specific functionality. Add this macro to a cpp file.
///
/// \see EZ_BEGIN_DYNAMIC_REFLECTED_TYPE
#define EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(componentType, version) EZ_BEGIN_ABSTRACT_DYNAMIC_REFLECTED_TYPE(componentType, version)

/// \brief Ends the component implementation code block that was opened with EZ_BEGIN_COMPONENT_TYPE.
#define EZ_END_COMPONENT_TYPE EZ_END_DYNAMIC_REFLECTED_TYPE
#define EZ_END_ABSTRACT_COMPONENT_TYPE EZ_END_ABSTRACT_DYNAMIC_REFLECTED_TYPE

#include <Core/World/Implementation/ComponentManager_inl.h>
