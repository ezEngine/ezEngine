#include <Core/World/GameObject.h>

EZ_ALWAYS_INLINE ezRenderData::Category::Category() = default;

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

// static
EZ_FORCE_INLINE ezHashedString ezRenderData::GetCategoryName(Category category)
{
  if (category.m_uiValue < s_CategoryData.GetCount())
  {
    return s_CategoryData[category.m_uiValue].m_sName;
  }

  return ezHashedString();
}

// static
EZ_FORCE_INLINE const ezRenderer* ezRenderData::GetCategoryRenderer(Category category, const ezRTTI* pRenderDataType)
{
  if (s_bRendererInstancesDirty)
  {
    CreateRendererInstances();
  }

  auto& categoryData = s_CategoryData[category.m_uiValue];

  ezUInt32 uiIndex = 0;
  if (categoryData.m_TypeToRendererIndex.TryGetValue(pRenderDataType, uiIndex))
  {
    return s_RendererInstances[uiIndex].Borrow();
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

EZ_FORCE_INLINE ezUInt64 ezRenderData::GetFinalSortingKey(Category category, const ezCamera& camera) const
{
  return s_CategoryData[category.m_uiValue].m_sortingKeyFunc(this, camera);
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
static T* ezCreateRenderDataForThisFrame(const ezGameObject* pOwner)
{
  static_assert(EZ_IS_DERIVED_FROM_STATIC(ezRenderData, T));

  T* pRenderData = EZ_NEW(ezFrameAllocator::GetCurrentAllocator(), T);

  if (pOwner != nullptr)
  {
    pRenderData->m_Flags.AddOrRemove(ezRenderData::Flags::Dynamic, pOwner->IsDynamic());
    pRenderData->m_hOwner = pOwner->GetHandle();
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  pRenderData->m_pOwner = pOwner;
#endif

  return pRenderData;
}
