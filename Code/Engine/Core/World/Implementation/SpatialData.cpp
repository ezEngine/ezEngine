#include <CorePCH.h>

#include <Core/World/SpatialData.h>

EZ_CHECK_AT_COMPILETIME(sizeof(ezSpatialData) == 64);

ezHybridArray<ezSpatialData::CategoryData, 32>& ezSpatialData::GetCategoryData()
{
  static ezHybridArray<ezSpatialData::CategoryData, 32> CategoryData;
  return CategoryData;
}

// static
ezSpatialData::Category ezSpatialData::RegisterCategory(const char* szCategoryName)
{
  Category oldCategory = FindCategory(szCategoryName);
  if (oldCategory != ezInvalidSpatialDataCategory)
    return oldCategory;

  if (GetCategoryData().GetCount() == 32)
  {
    EZ_REPORT_FAILURE("Too many spatial data categories");
    return ezInvalidSpatialDataCategory;
  }

  Category newCategory = Category(GetCategoryData().GetCount());

  auto& data = GetCategoryData().ExpandAndGetRef();
  data.m_sName.Assign(szCategoryName);

  return newCategory;
}

// static
ezSpatialData::Category ezSpatialData::FindCategory(const char* szCategoryName)
{
  ezTempHashedString categoryName(szCategoryName);

  for (ezUInt32 uiCategoryIndex = 0; uiCategoryIndex < GetCategoryData().GetCount(); ++uiCategoryIndex)
  {
    if (GetCategoryData()[uiCategoryIndex].m_sName == categoryName)
      return Category(uiCategoryIndex);
  }

  return ezInvalidSpatialDataCategory;
}

//////////////////////////////////////////////////////////////////////////

ezSpatialData::Category ezDefaultSpatialDataCategories::RenderStatic = ezSpatialData::RegisterCategory("RenderStatic");
ezSpatialData::Category ezDefaultSpatialDataCategories::RenderDynamic = ezSpatialData::RegisterCategory("RenderDynamic");
