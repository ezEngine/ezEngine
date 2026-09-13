#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

/// A thread-safe dependency tracking system for managing resource invalidation.
///
/// This template class tracks dependencies between resources and their dependencies to allow invalidating resources when their dependencies are destroyed.
/// When a dependency is destroyed, all resources that depend on it are automatically identified and an invalidation event is broadcast for each affected resource.
///
/// Both types must be usable as an ezMap key, i.e. provide `operator<`, and should be cheap to copy.
///
/// \tparam Resource The type of resource being tracked (e.g. `ezGALBindGroup*`)
/// \tparam Dependency The type of the dependencies (e.g. `const ezGALResourceBase*`)
template <typename Resource, typename Dependency>
class ezDependencyTracker
{
public:
  /// Adds a resource and its dependencies to the tracking system.
  ///
  /// This method registers a resource along with all of its dependencies. The resource will be automatically invalidated if any of its dependencies are destroyed. Each resource can only be added once - attempting to add the same resource again will trigger an assertion in debug builds.
  ///
  /// \param resource The resource to track
  /// \param dependencies Set of dependencies that this resource depends on
  void AddResource(const Resource& resource, const ezSet<Dependency>& dependencies);

  /// Removes a resource from the tracking system.
  ///
  /// This method removes the resource and all of its dependency relationships from the tracker. The resource will no longer receive invalidation events even if its former dependencies are destroyed.
  ///
  /// This also has to be called from a m_ResourceInvalidatedEvent handler, as DependencyDestroyed only severs the links to the one dependency that went away, not those to the remaining ones.
  ///
  /// \param resource The resource to remove from tracking
  void RemoveResource(const Resource& resource);

  /// Notifies the tracker that a dependency has been destroyed.
  ///
  /// This method should be called when a dependency object is about to be destroyed. It will identify all resources that depend on this dependency, remove the dependency relationships, and broadcast invalidation events for each affected resource.
  ///
  /// \param dependency The dependency that is being destroyed
  void DependencyDestroyed(const Dependency& dependency);

public:
  /// Event that is broadcast when a resource becomes invalid due to dependency destruction.
  ezEvent<Resource> m_ResourceInvalidatedEvent;

private:
  struct Item
  {
    Item* m_pPreviousResource = nullptr;
    Item* m_pNextResource = nullptr;
    Item* m_pPreviousDependency = nullptr;
    Item* m_pNextDependency = nullptr;
    Resource m_Resource = {};
    Dependency m_Dependency = {};
  };
  using ResourceHeadMap = ezMap<Resource, Item*>;
  using DependencyHeadMap = ezMap<Dependency, Item*>;

private:
  void InsertItem(typename ResourceHeadMap::Iterator resourceHead, const Resource& resource, const Dependency& dependency);
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
