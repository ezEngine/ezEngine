#include <Core/CorePCH.h>

#include <Core/World/SpatialData.h>

ezHybridArray<ezSpatialData::CategoryData, 32>& ezSpatialData::GetCategoryData()
{
  static ezHybridArray<ezSpatialData::CategoryData, 32> CategoryData;
  return CategoryData;
}

// static
ezSpatialData::Category ezSpatialData::RegisterCategory(const char* szCategoryName, const ezBitflags<Flags>& flags)
{
  Category oldCategory = FindCategory(szCategoryName);
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

  Category newCategory = Category(GetCategoryData().GetCount());

  auto& data = GetCategoryData().ExpandAndGetRef();
  data.m_sName.Assign(szCategoryName);
  data.m_Flags = flags;

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

// static
const ezBitflags<ezSpatialData::Flags>& ezSpatialData::GetCategoryFlags(Category category)
{
  return GetCategoryData()[category.m_uiValue].m_Flags;
}

//////////////////////////////////////////////////////////////////////////

ezSpatialData::Category ezDefaultSpatialDataCategories::RenderStatic = ezSpatialData::RegisterCategory("RenderStatic", ezSpatialData::Flags::None);
ezSpatialData::Category ezDefaultSpatialDataCategories::RenderDynamic = ezSpatialData::RegisterCategory("RenderDynamic", ezSpatialData::Flags::FrequentChanges);


EZ_STATICLINK_FILE(Core, Core_World_Implementation_SpatialData);
