#include <CorePCH.h>

#include <Core/World/SpatialData.h>

EZ_CHECK_AT_COMPILETIME(sizeof(ezSpatialData) == 64);

ezHybridArray<ezSpatialData::CategoryData, 32> ezSpatialData::s_CategoryData;

// static
ezSpatialData::Category ezSpatialData::RegisterCategory(const char* szCategoryName)
{
  Category oldCategory = FindCategory(szCategoryName);
  if (oldCategory != ezInvalidSpatialDataCategory)
    return oldCategory;

  if (s_CategoryData.GetCount() == 32)
  {
    EZ_REPORT_FAILURE("Too many spatial data categories");
    return ezInvalidSpatialDataCategory;
  }

  Category newCategory = Category(s_CategoryData.GetCount());

  auto& data = s_CategoryData.ExpandAndGetRef();
  data.m_sName.Assign(szCategoryName);

  return newCategory;
}

// static
ezSpatialData::Category ezSpatialData::FindCategory(const char* szCategoryName)
{
  ezTempHashedString categoryName(szCategoryName);

  for (ezUInt32 uiCategoryIndex = 0; uiCategoryIndex < s_CategoryData.GetCount(); ++uiCategoryIndex)
  {
    if (s_CategoryData[uiCategoryIndex].m_sName == categoryName)
      return Category(uiCategoryIndex);
  }

  return ezInvalidSpatialDataCategory;
}

//////////////////////////////////////////////////////////////////////////

ezSpatialData::Category ezDefaultSpatialDataCategories::RenderStatic = ezSpatialData::RegisterCategory("RenderStatic");
ezSpatialData::Category ezDefaultSpatialDataCategories::RenderDynamic = ezSpatialData::RegisterCategory("RenderDynamic");
