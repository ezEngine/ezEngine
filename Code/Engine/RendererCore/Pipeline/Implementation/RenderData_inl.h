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

//////////////////////////////////////////////////////////////////////////

EZ_ALWAYS_INLINE bool ezRenderData::IsDynamic() const
{
  return m_Flags.IsSet(Flags::Dynamic);
}

EZ_ALWAYS_INLINE bool ezRenderData::IsStatic() const
{
  return !m_Flags.IsSet(Flags::Dynamic);
}

EZ_ALWAYS_INLINE bool ezRenderData::FlipWinding() const
{
  return m_Flags.IsSet(Flags::FlipWinding);
}

EZ_FORCE_INLINE ezUInt64 ezRenderData::GetFinalSortingKey(Category category, const ezCamera& camera) const
{
  return s_CategoryData[category.m_uiValue].m_sortingKeyFunc(this, camera);
}

//////////////////////////////////////////////////////////////////////////

EZ_FORCE_INLINE bool ezInstanceableRenderData::CanBatchByBaseValues(const ezInstanceableRenderData& other) const
{
  return FlipWinding() == other.FlipWinding() && m_hInstanceDataBuffer == other.m_hInstanceDataBuffer;
}
