
template <typename HandleType, typename DescType, typename KeyType>
HandleType ezGALPipelineCache::TryGetPipeline(const DescType& description, ezHashTable<KeyType, HandleType, ezGALPipelineCache::CacheKeyHasher>& table)
{
  EZ_ASSERT_DEV(m_pDevice != nullptr, "GAL device not initialized");

  KeyType key;
  key.m_Desc = description;
  key.m_uiHash = description.CalculateHash();

  {
    EZ_LOCK(m_Mutex);

    HandleType* pExistingPipeline = table.GetValue(key);
    if (pExistingPipeline != nullptr)
    {
      return *pExistingPipeline;
    }
  }
  return {};
}

template <typename HandleType, typename DescType, typename KeyType>
EZ_ALWAYS_INLINE ezResult ezGALPipelineCache::TryInsertPipeline(const DescType& description, HandleType hNewPipeline, ezHashTable<KeyType, HandleType, ezGALPipelineCache::CacheKeyHasher>& table)
{
  KeyType key;
  key.m_Desc = description;
  key.m_uiHash = description.CalculateHash();

  EZ_LOCK(m_Mutex);

  HandleType existingPipeline;
  if (table.Insert(key, hNewPipeline, &existingPipeline))
  {
    EZ_ASSERT_DEBUG(existingPipeline == hNewPipeline, "On collision, both pipelines must be the same (create should have just increased the ref count)");
    return EZ_FAILURE;
  }
  return EZ_SUCCESS;
}
