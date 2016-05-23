#pragma once

#include <Core/World/ComponentManager.h>

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
  virtual void DeleteDeadComponent(ezComponentManagerBase::ComponentStorageEntry storageEntry, ezComponent*& out_pMovedComponent) override;

  /// \brief Returns the first component of this type that has been created.
  ComponentType* GetSingletonComponent() const { return m_pSingleton; }

private:
  ComponentType* m_pSingleton;
};

#include <Core/World/Implementation/SettingsComponentManager_inl.h>

