#include <Core/CorePCH.h>

#include <Core/World/SpatialData.h>

ezHybridArray<ezSpatialData::CategoryData, 32>& ezSpatialData::GetCategoryData()
{
  static ezHybridArray<ezSpatialData::CategoryData, 32> CategoryData;
  return CategoryData;
}

// static
ezSpatialData::Category ezSpatialData::RegisterCategory(ezStringView sCategoryName, const ezBitflags<Flags>& flags)
{
  if (sCategoryName.IsEmpty())
    return ezInvalidSpatialDataCategory;

  Category oldCategory = FindCategory(sCategoryName);
  if (oldCategory != ezInvalidSpatialDataCategory)
  {
    EZ_ASSERT_DEV(GetCategoryFlags(oldCategory) == flags, "Category registered with different flags");
    return oldCategory;
  }

  if (GetCategoryData().GetCount() == 32)
  {
    EZ_REPORT_FAILURE("Too many spatial data categories");
    return ezInvalidSpatialDataCategory;
  }

  Category newCategory = Category(static_cast<ezUInt16>(GetCategoryData().GetCount()));

  auto& data = GetCategoryData().ExpandAndGetRef();
  data.m_sName.Assign(sCategoryName);
  data.m_Flags = flags;

  return newCategory;
}

// static
ezSpatialData::Category ezSpatialData::FindCategory(ezStringView sCategoryName)
{
  ezTempHashedString categoryName(sCategoryName);

  for (ezUInt32 uiCategoryIndex = 0; uiCategoryIndex < GetCategoryData().GetCount(); ++uiCategoryIndex)
  {
    if (GetCategoryData()[uiCategoryIndex].m_sName == categoryName)
      return Category(static_cast<ezUInt16>(uiCategoryIndex));
  }

  return ezInvalidSpatialDataCategory;
}

// static
const ezHashedString& ezSpatialData::GetCategoryName(Category category)
{
  if (category.m_uiValue < GetCategoryData().GetCount())
  {
    return GetCategoryData()[category.m_uiValue].m_sName;
  }

  static ezHashedString sInvalidSpatialDataCategoryName;
  return sInvalidSpatialDataCategoryName;
}

// static
const ezBitflags<ezSpatialData::Flags>& ezSpatialData::GetCategoryFlags(Category category)
{
  return GetCategoryData()[category.m_uiValue].m_Flags;
}

//////////////////////////////////////////////////////////////////////////

ezSpatialData::Category ezDefaultSpatialDataCategories::RenderStatic = ezSpatialData::RegisterCategory("RenderStatic", ezSpatialData::Flags::None);
ezSpatialData::Category ezDefaultSpatialDataCategories::RenderDynamic = ezSpatialData::RegisterCategory("RenderDynamic", ezSpatialData::Flags::FrequentChanges);
ezSpatialData::Category ezDefaultSpatialDataCategories::OcclusionStatic = ezSpatialData::RegisterCategory("OcclusionStatic", ezSpatialData::Flags::None);
ezSpatialData::Category ezDefaultSpatialDataCategories::OcclusionDynamic = ezSpatialData::RegisterCategory("OcclusionDynamic", ezSpatialData::Flags::FrequentChanges);


