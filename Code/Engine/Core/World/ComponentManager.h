#pragma once

#include <Foundation/Types/Delegate.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Memory/BlockStorage.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Logging/Log.h>

#include <Core/World/Declarations.h>
#include <Core/World/Component.h>
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
  bool IsValidComponent(const ezComponentHandle& component) const;

  /// \brief Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const ezComponentHandle& component, ezComponent*& out_pComponent);

  /// \brief Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const ezComponentHandle& component, const ezComponent*& out_pComponent) const;

  /// \brief Returns the number of components managed by this manager.
  ezUInt32 GetComponentCount() const;

  /// \brief Create a new component instance and returns a handle to it.
  ezComponentHandle CreateComponent(ezGameObject* pOwnerObject);

  /// \brief Create a new component instance and returns a handle to it.
  template <typename ComponentType>
  ezComponentHandle CreateComponent(ezGameObject* pOwnerObject, ComponentType*& out_pComponent);

  /// \brief Deletes the given component. Note that the component will be invalidated first and the actual deletion is postponed.
  void DeleteComponent(const ezComponentHandle& component);

  /// \brief Adds all components that this manager handles to the given array (array is not cleared).
  /// Prefer to use more efficient methods on derived classes, only use this if you need to go through a ezComponentManagerBase pointer.
  virtual void CollectAllComponents(ezDynamicArray<ezComponentHandle>& out_AllComponents) = 0;

  /// \brief Adds all components that this manager handles to the given array (array is not cleared).
  /// Prefer to use more efficient methods on derived classes, only use this if you need to go through a ezComponentManagerBase pointer.
  virtual void CollectAllComponents(ezDynamicArray<ezComponent*>& out_AllComponents) = 0;

private:
  /// \cond
  // internal methods
  friend class ezWorld;
  friend class ezInternal::WorldData;

  void DeinitializeInternal() override;

protected:
  void InitializeComponent(ezGameObject* pOwnerObject, ezComponent* pComponent);
  void DeinitializeComponent(ezComponent* pComponent);
  void PatchIdTable(ezComponent* pComponent);

  virtual ezComponent* CreateComponentStorage() = 0;
  virtual void DeleteComponentStorage(ezComponent* pComponent, ezComponent*& out_pMovedComponent) = 0;

  /// \endcond

  ezIdTable<ezGenericComponentId, ezComponent*> m_Components;
};

template <typename T, ezBlockStorageType::Enum StorageType>
class ezComponentManager : public ezComponentManagerBase
{
public:
  typedef T ComponentType;

  /// \brief Although the constructor is public always use ezWorld::CreateComponentManager to create an instance.
  ezComponentManager(ezWorld* pWorld);
  virtual ~ezComponentManager();

  /// \brief Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const ezComponentHandle& component, ComponentType*& out_pComponent);

  /// \brief Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const ezComponentHandle& component, const ComponentType*& out_pComponent) const;

  /// \brief Returns an iterator over all components.
  typename ezBlockStorage<ComponentType, ezInternal::DEFAULT_BLOCK_SIZE, StorageType>::Iterator GetComponents();

  /// \brief Returns an iterator over all components.
  typename ezBlockStorage<ComponentType, ezInternal::DEFAULT_BLOCK_SIZE, StorageType>::ConstIterator GetComponents() const;

  /// \brief Returns the type id corresponding to the component type managed by this manager.
  static ezUInt16 TypeId();

  virtual void CollectAllComponents(ezDynamicArray<ezComponentHandle>& out_AllComponents) override;
  virtual void CollectAllComponents(ezDynamicArray<ezComponent*>& out_AllComponents) override;

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
template <typename ComponentType, ezComponentUpdateType::Enum UpdateType>
class ezComponentManagerSimple : public ezComponentManager<ComponentType, ezBlockStorageType::FreeList>
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

#define EZ_ADD_COMPONENT_FUNCTIONALITY(componentType, baseType, managerType) \
  public: \
    typedef managerType ComponentManagerType; \
    virtual ezUInt16 GetTypeId() const override { return TYPE_ID; } \
    static EZ_ALWAYS_INLINE ezUInt16 TypeId() { return TYPE_ID; } \
    virtual ezComponentMode::Enum GetMode() const override; \
    static ezComponentHandle CreateComponent(ezGameObject* pOwnerObject, componentType*& pComponent); \
    static void DeleteComponent(componentType* pComponent); \
    void DeleteComponent(); \
  private: \
    friend managerType; \
    static ezUInt16 TYPE_ID

#define EZ_ADD_ABSTRACT_COMPONENT_FUNCTIONALITY(componentType, baseType) \
  public: \
    virtual ezUInt16 GetTypeId() const override { return -1; } \
    static EZ_ALWAYS_INLINE ezUInt16 TypeId() { return -1; }

/// \brief Add this macro to a custom component type inside the type declaration.
#define EZ_DECLARE_COMPONENT_TYPE(componentType, baseType, managerType) \
  EZ_ADD_DYNAMIC_REFLECTION(componentType, baseType); \
  EZ_ADD_COMPONENT_FUNCTIONALITY(componentType, baseType, managerType);

/// \brief Add this macro to a custom abstract component type inside the type declaration.
#define EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(componentType, baseType) \
  EZ_ADD_DYNAMIC_REFLECTION(componentType, baseType); \
  EZ_ADD_ABSTRACT_COMPONENT_FUNCTIONALITY(componentType, baseType);


/// \brief Implements rtti and component specific functionality. Add this macro to a cpp file.
///
/// \see EZ_BEGIN_DYNAMIC_REFLECTED_TYPE
#define EZ_BEGIN_COMPONENT_TYPE(componentType, version, mode) \
  ezUInt16 componentType::TYPE_ID = ezWorldModuleFactory::GetInstance()->RegisterWorldModule<typename componentType::ComponentManagerType, componentType>(); \
  ezComponentMode::Enum componentType::GetMode() const { return mode; } \
  ezComponentHandle componentType::CreateComponent(ezGameObject* pOwnerObject, componentType*& out_pComponent) { \
    return pOwnerObject->GetWorld()->GetOrCreateComponentManager<ComponentManagerType>()->CreateComponent(pOwnerObject, out_pComponent); } \
  void componentType::DeleteComponent(componentType* pComponent) { \
    pComponent->GetOwningManager()->DeleteComponent(pComponent->GetHandle()); } \
  void componentType::DeleteComponent() { \
    GetOwningManager()->DeleteComponent(GetHandle()); } \
  EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(componentType, version, ezRTTINoAllocator);

/// \brief Implements rtti and abstract component specific functionality. Add this macro to a cpp file.
///
/// \see EZ_BEGIN_DYNAMIC_REFLECTED_TYPE
#define EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(componentType, version) EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(componentType, version, ezRTTINoAllocator); flags.Add(ezTypeFlags::Abstract);

/// \brief Ends the component implementation code block that was opened with EZ_BEGIN_COMPONENT_TYPE.
#define EZ_END_COMPONENT_TYPE EZ_END_DYNAMIC_REFLECTED_TYPE
#define EZ_END_ABSTRACT_COMPONENT_TYPE EZ_END_DYNAMIC_REFLECTED_TYPE

#include <Core/World/Implementation/ComponentManager_inl.h>

