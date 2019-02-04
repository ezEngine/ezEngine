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
EZ_FORCE_INLINE const char* ezRenderData::GetCategoryName(Category category)
{
  return s_CategoryData[category.m_uiValue].m_sName.GetString();
}

EZ_FORCE_INLINE ezUInt64 ezRenderData::GetCategorySortingKey(Category category, ezUInt32 uiRenderDataSortingKey, const ezCamera& camera) const
{
  return s_CategoryData[category.m_uiValue].m_sortingKeyFunc(this, uiRenderDataSortingKey, camera);
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
static T* ezCreateRenderDataForThisFrame(const ezGameObject* pOwner, ezUInt32 uiBatchId)
{
  EZ_CHECK_AT_COMPILETIME(EZ_IS_DERIVED_FROM_STATIC(ezRenderData, T));

  T* pRenderData = EZ_NEW(ezFrameAllocator::GetCurrentAllocator(), T);
  //ezMemoryUtils::ZeroFill(pRenderData);

  pRenderData->m_uiBatchId = uiBatchId;

  if (pOwner != nullptr)
  {
    pRenderData->m_hOwner = pOwner->GetHandle();
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  pRenderData->m_pOwner = pOwner;
#endif

  return pRenderData;
}

