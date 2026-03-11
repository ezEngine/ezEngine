#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

/// \brief A thread-safe dependency tracking system for managing resource invalidation.
///
/// This template class tracks dependencies between resources and their dependencies to allow invalidating resources when their dependencies are destroyed.
/// When a dependency is destroyed, all resources that depend on it are automatically identified and an invalidation event is broadcast for each affected resource.
///
/// \tparam Resource The type of resource being tracked (e.g. bind groups)
/// \tparam Dependency The base type of the dependencies (e.g., ezGALResource)
template <typename Resource, typename Dependency>
class ezDependencyTracker
{
public:
  /// \brief Adds a resource and its dependencies to the tracking system.
  ///
  /// This method registers a resource along with all of its dependencies. The resource will be automatically invalidated if any of its dependencies are destroyed. Each resource can only be added once - attempting to add the same resource again will trigger an assertion in debug builds.
  ///
  /// \param pResource Pointer to the resource to track
  /// \param dependencies Set of dependencies that this resource depends on
  void AddResource(Resource* pResource, const ezSet<const Dependency*>& dependencies);

  /// \brief Removes a resource from the tracking system.
  ///
  /// This method removes the resource and all of its dependency relationships from the tracker. The resource will no longer receive invalidation events even if its former dependencies are destroyed.
  ///
  /// \param pResource Pointer to the resource to remove from tracking
  void RemoveResource(Resource* pResource);

  /// \brief Notifies the tracker that a dependency has been destroyed.
  ///
  /// This method should be called when a dependency object is about to be destroyed. It will identify all resources that depend on this dependency, remove the dependency relationships, and broadcast invalidation events for each affected resource.
  ///
  /// \param pDependency Pointer to the dependency that is being destroyed
  void DependencyDestroyed(Dependency* pDependency);

public:
  /// \brief Event that is broadcast when a resource becomes invalid due to dependency destruction.
  ezEvent<Resource*> m_ResourceInvalidatedEvent;

private:
  struct Item
  {
    EZ_DECLARE_POD_TYPE();
    Item* m_pPreviousResource = nullptr;
    Item* m_pNextResource = nullptr;
    Item* m_pPreviousDependency = nullptr;
    Item* m_pNextDependency = nullptr;
    Resource* m_pResource = nullptr;
    const Dependency* m_pDependency = nullptr;
  };
  using ResourceHeadMap = ezMap<Resource*, Item*>;
  using DependencyHeadMap = ezMap<const Dependency*, Item*>;

private:
  void InsertItem(typename ResourceHeadMap::Iterator resourceHead, Resource* pResource, const Dependency* pDependency);
  void RemoveResourceItem(typename ResourceHeadMap::ConstIterator resourceHead, Item* pItem);
  void RemoveDependencyItem(typename DependencyHeadMap::ConstIterator dependencyHead, Item* pItem);

private:
  ezMutex m_Mutex;
  ezDeque<Item> m_Dependencies;
  Item* m_pFreeList = nullptr;
  ResourceHeadMap m_ResourceHead;
  DependencyHeadMap m_DependencyHead;
};

#include <RendererFoundation/Utils/Implementation/DependencyTracker_inl.h>
