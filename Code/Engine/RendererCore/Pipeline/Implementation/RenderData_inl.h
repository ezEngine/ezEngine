#include <Core/World/GameObject.h>

EZ_ALWAYS_INLINE ezRenderData::Category::Category()
  : m_uiValue(0xFFFF)
{
}

EZ_ALWAYS_INLINE ezRenderData::Category::Category(ezUInt16 uiValue)
  : m_uiValue(uiValue)
{
}

EZ_ALWAYS_INLINE bool ezRenderData::Category::operator==(const Category& other) const
{
  return m_uiValue == other.m_uiValue;
}

EZ_ALWAYS_INLINE bool ezRenderData::Category::operator!=(const Category& other) const
{
  return m_uiValue != other.m_uiValue;
}

//////////////////////////////////////////////////////////////////////////

//static
EZ_FORCE_INLINE const ezRenderer* ezRenderData::GetCategoryRenderer(Category category, const ezRTTI* pRenderDataType)
{
  auto& categoryData = s_CategoryData[category.m_uiValue];
  if (categoryData.m_bRendererInstancesDirty)
  {
    categoryData.CreateRendererInstances();
  }

  ezUInt32 uiIndex = 0;
  if (categoryData.m_TypeToRendererIndex.TryGetValue(pRenderDataType, uiIndex))
  {
    return categoryData.m_RendererInstances[uiIndex].Borrow();
  }

  return nullptr;
}

//static
EZ_FORCE_INLINE const char* ezRenderData::GetCategoryName(Category category)
{
  return s_CategoryData[category.m_uiValue].m_sName.GetString();
}

EZ_FORCE_INLINE ezUInt64 ezRenderData::GetCategorySortingKey(Category category, const ezCamera& camera) const
{
  return s_CategoryData[category.m_uiValue].m_sortingKeyFunc(this, m_uiSortingKey, camera);
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
static T* ezCreateRenderDataForThisFrame(const ezGameObject* pOwner)
{
  EZ_CHECK_AT_COMPILETIME(EZ_IS_DERIVED_FROM_STATIC(ezRenderData, T));

  T* pRenderData = EZ_NEW(ezFrameAllocator::GetCurrentAllocator(), T);

  if (pOwner != nullptr)
  {
    pRenderData->m_hOwner = pOwner->GetHandle();
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  pRenderData->m_pOwner = pOwner;
#endif

  return pRenderData;
}

