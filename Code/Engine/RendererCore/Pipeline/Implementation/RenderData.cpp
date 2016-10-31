#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/SortingFunctions.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderData, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderer, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_IMPLEMENT_MESSAGE_TYPE(ezExtractRenderDataMessage);

ezHybridArray<ezRenderData::CategoryData, 32> ezRenderData::s_CategoryData;

//static 
ezRenderData::Category ezRenderData::RegisterCategory(const char* szCategoryName, SortingKeyFunc sortingKeyFunc)
{
  Category oldCategory = FindCategory(szCategoryName);
  if (oldCategory != ezInvalidIndex)
    return oldCategory;

  Category newCategory = s_CategoryData.GetCount();

  auto& data = s_CategoryData.ExpandAndGetRef();
  data.m_sName.Assign(szCategoryName);
  data.m_ProfilingID = ezProfilingSystem::CreateId(szCategoryName);
  data.m_sortingKeyFunc = sortingKeyFunc;

  return newCategory;
}

ezRenderData::Category ezRenderData::FindCategory(const char* szCategoryName)
{
  ezTempHashedString categoryName(szCategoryName);

  for (Category category = 0; category < s_CategoryData.GetCount(); ++category)
  {
    if (s_CategoryData[category].m_sName == categoryName)
      return category;
  }

  return ezInvalidIndex;
}

ezRenderData::Category ezDefaultRenderDataCategories::Light = ezRenderData::RegisterCategory("Light", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::Sky = ezRenderData::RegisterCategory("Sky", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::LitOpaque = ezRenderData::RegisterCategory("LitOpaque", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::LitMasked = ezRenderData::RegisterCategory("LitMasked", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::LitTransparent = ezRenderData::RegisterCategory("LitTransparent", &ezRenderSortingFunctions::BackToFrontThenByRenderData);
ezRenderData::Category ezDefaultRenderDataCategories::LitForeground = ezRenderData::RegisterCategory("LitForeground", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::SimpleOpaque = ezRenderData::RegisterCategory("SimpleOpaque", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::SimpleTransparent = ezRenderData::RegisterCategory("SimpleTransparent", &ezRenderSortingFunctions::BackToFrontThenByRenderData);
ezRenderData::Category ezDefaultRenderDataCategories::SimpleForeground = ezRenderData::RegisterCategory("SimpleForeground", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::Selection = ezRenderData::RegisterCategory("Selection", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);

