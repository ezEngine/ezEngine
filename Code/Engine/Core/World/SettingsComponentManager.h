#pragma once

#include <Core/World/ComponentManager.h>
#include <Foundation/Types/UniquePtr.h>

/// \brief A component manager that does no update at all on components and expects only a single instance to be created per world.
///
/// Easy access to this single component is provided through the GetSingletonComponent() function.
/// If a second component is created, the manager will log an error. The first created component will be used as the 'singleton',
/// all other components are ignored.
/// Use this for components derived from ezSettingsComponent, of which one should only have zero or one per world.
template <typename ComponentType>
class ezSettingsComponentManager : public ezComponentManagerBase
{
public:
  ezSettingsComponentManager(ezWorld* pWorld);
  ~ezSettingsComponentManager();

  /// \brief Returns the first component of this type that has been created.
  ComponentType* GetSingletonComponent();
  const ComponentType* GetSingletonComponent() const;

  static ezWorldModuleTypeId TypeId();

  // ezComponentManagerBase implementation
  virtual void CollectAllComponents(ezDynamicArray<ezComponentHandle>& out_AllComponents, bool bOnlyActive) override;
  virtual void CollectAllComponents(ezDynamicArray<ezComponent*>& out_AllComponents, bool bOnlyActive) override;

private:
  friend class ezComponentManagerFactory;

  virtual ezComponent* CreateComponentStorage() override;
  virtual void DeleteComponentStorage(ezComponent* pComponent, ezComponent*& out_pMovedComponent) override;

  ezHybridArray<ezUniquePtr<ComponentType>, 2> m_Components;
};

#include <Core/World/Implementation/SettingsComponentManager_inl.h>
