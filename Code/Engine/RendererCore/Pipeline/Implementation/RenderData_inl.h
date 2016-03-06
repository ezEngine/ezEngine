
//static 
EZ_FORCE_INLINE const char* ezRenderData::GetCategoryName(Category category)
{
  return s_CategoryData[category].m_sName.GetString().GetData();
}

//static
EZ_FORCE_INLINE ezProfilingId& ezRenderData::GetCategoryProfilingID(Category category)
{
  return s_CategoryData[category].m_ProfilingID;
}

EZ_FORCE_INLINE ezUInt64 ezRenderData::GetSortingKey(Category category, const ezCamera& camera) const
{
  return s_CategoryData[category].m_sortingKeyFunc(this, camera);
}

template <typename T>
static T* ezCreateRenderDataForThisFrame(const ezGameObject* pOwner, ezUInt32 uiBatchId)
{
  EZ_CHECK_AT_COMPILETIME(EZ_IS_DERIVED_FROM_STATIC(ezRenderData, T));

  T* pRenderData = EZ_NEW(ezFrameAllocator::GetCurrentAllocator(), T);
  pRenderData->m_uiBatchId = uiBatchId;
  pRenderData->m_hOwner = pOwner->GetHandle();

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  pRenderData->m_pOwner = pOwner;
#endif

  return pRenderData;
}
