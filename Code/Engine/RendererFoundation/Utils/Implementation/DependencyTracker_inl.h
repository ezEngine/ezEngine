template <typename Resource, typename Dependency>
void ezDependencyTracker<Resource, Dependency>::AddResource(Resource* pResource, const ezSet<const Dependency*>& dependencies)
{
  EZ_LOCK(m_Mutex);
  bool bExisted = false;
  auto resourceHead = m_ResourceHead.FindOrAdd(pResource, &bExisted);
  EZ_IGNORE_UNUSED(bExisted);
  EZ_ASSERT_DEBUG(!bExisted, "Resource already tracked");

  for (const Dependency* pDependency : dependencies)
  {
    InsertItem(resourceHead, pResource, pDependency);
  }
}

template <typename Resource, typename Dependency>
void ezDependencyTracker<Resource, Dependency>::RemoveResource(Resource* pResource)
{
  EZ_LOCK(m_Mutex);

  auto resourceHead = m_ResourceHead.Find(pResource);
  EZ_ASSERT_DEBUG(resourceHead.IsValid(), "Resource not tracked");

  Item* pItem = resourceHead.Value();
  while (pItem != nullptr)
  {
    Item* pCurrentItem = pItem;
    pItem = pItem->m_pNextDependency;
    RemoveResourceItem(resourceHead, pCurrentItem);
  }

  m_ResourceHead.Remove(resourceHead);
}

template <typename Resource, typename Dependency>
void ezDependencyTracker<Resource, Dependency>::DependencyDestroyed(Dependency* pDependency)
{
  ezSet<Resource*> invalidResources;
  {
    EZ_LOCK(m_Mutex);
    auto dependencyHead = m_DependencyHead.Find(pDependency);
    if (!dependencyHead.IsValid())
      return;

    Item* pItem = dependencyHead.Value();
    while (pItem != nullptr)
    {
      Item* pCurrentItem = pItem;
      pItem = pItem->m_pNextResource;
      invalidResources.Insert(pCurrentItem->m_pResource);
      // This will destroy pCurrentItem so we must move pItem forward before this.
      RemoveDependencyItem(dependencyHead, pCurrentItem);
    }
    m_DependencyHead.Remove(dependencyHead);
  }

  for (Resource* pResource : invalidResources)
  {
    m_ResourceInvalidatedEvent.Broadcast(pResource);
  }
}

template <typename Resource, typename Dependency>
void ezDependencyTracker<Resource, Dependency>::InsertItem(typename ResourceHeadMap::Iterator resourceHead, Resource* pResource, const Dependency* pDependency)
{
  Item* pItem = nullptr;
  if (m_pFreeList != nullptr)
  {
    pItem = m_pFreeList;
    m_pFreeList = m_pFreeList->m_pNextDependency;
    *pItem = {};
  }
  else
  {
    m_Dependencies.PushBack();
    pItem = &m_Dependencies.PeekBack();
  }
  pItem->m_pResource = pResource;
  pItem->m_pDependency = pDependency;

  auto dependencyHead = m_DependencyHead.FindOrAdd(pDependency);

  if (dependencyHead.Value() != nullptr)
  {
    dependencyHead.Value()->m_pPreviousResource = pItem;
    pItem->m_pNextResource = dependencyHead.Value();
  }

  if (resourceHead.Value() != nullptr)
  {
    resourceHead.Value()->m_pPreviousDependency = pItem;
    pItem->m_pNextDependency = resourceHead.Value();
  }

  dependencyHead.Value() = pItem;
  resourceHead.Value() = pItem;
}

template <typename Resource, typename Dependency>
void ezDependencyTracker<Resource, Dependency>::RemoveResourceItem(typename ResourceHeadMap::ConstIterator resourceHead, Item* pItem)
{
  if (pItem->m_pNextResource != nullptr)
  {
    Item* pNextItem = pItem->m_pNextResource;
    pNextItem->m_pPreviousResource = pItem->m_pPreviousResource;
  }

  if (pItem->m_pPreviousResource != nullptr)
  {
    Item* pPreviousItem = pItem->m_pPreviousResource;
    pPreviousItem->m_pNextResource = pItem->m_pNextResource;
  }
  else
  {
    // We delete a dependency head
    if (pItem->m_pNextResource == nullptr)
    {
      // Last instance of this dependency, remove key.
      m_DependencyHead.Remove(pItem->m_pDependency);
    }
    else
    {
      // Update head to next item in list.
      m_DependencyHead.Insert(pItem->m_pDependency, pItem->m_pNextResource);
    }
  }

  *pItem = {};
  pItem->m_pNextDependency = m_pFreeList;
  m_pFreeList = pItem;
}

template <typename Resource, typename Dependency>
void ezDependencyTracker<Resource, Dependency>::RemoveDependencyItem(typename DependencyHeadMap::ConstIterator dependencyHead, Item* pItem)
{
  if (pItem->m_pNextDependency != nullptr)
  {
    Item* pNextItem = pItem->m_pNextDependency;
    pNextItem->m_pPreviousDependency = pItem->m_pPreviousDependency;
  }

  if (pItem->m_pPreviousDependency != nullptr)
  {
    Item* pPreviousItem = pItem->m_pPreviousDependency;
    pPreviousItem->m_pNextDependency = pItem->m_pNextDependency;
  }
  else
  {
    // We delete a resource head
    if (pItem->m_pNextDependency == nullptr)
    {
      // Last instance of this resource, set to nullptr
      m_ResourceHead.Insert(pItem->m_pResource, nullptr);
    }
    else
    {
      // Update head to next item in list.
      m_ResourceHead.Insert(pItem->m_pResource, pItem->m_pNextDependency);
    }
  }

  *pItem = {};
  pItem->m_pNextDependency = m_pFreeList;
  m_pFreeList = pItem;
}
