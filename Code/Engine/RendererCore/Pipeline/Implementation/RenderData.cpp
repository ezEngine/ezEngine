#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/SortingFunctions.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderData, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderer, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_IMPLEMENT_MESSAGE_TYPE(ezExtractRenderDataMessage);

ezHybridArray<ezRenderData::CategoryData, 32> ezRenderData::s_CategoryData;

//static 
ezRenderData::Category ezRenderData::RegisterCategory(const char* szCategoryName, SortingKeyFunc sortingKeyFunc)
{
  ezTempHashedString categoryName(szCategoryName);

  for (Category category = 0; category < s_CategoryData.GetCount(); ++category)
  {
    if (s_CategoryData[category].m_sName == categoryName)
      return category;
  }

  Category newCategory = s_CategoryData.GetCount();

  auto& data = s_CategoryData.ExpandAndGetRef();
  data.m_sName.Assign(szCategoryName);
  data.m_ProfilingID = ezProfilingSystem::CreateId(szCategoryName);
  data.m_sortingKeyFunc = sortingKeyFunc;

  return newCategory;
}

ezRenderData::Category ezDefaultRenderDataCategories::Light = ezRenderData::RegisterCategory("Light", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::Opaque = ezRenderData::RegisterCategory("Opaque", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::Masked = ezRenderData::RegisterCategory("Masked", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::Transparent = ezRenderData::RegisterCategory("Transparent", &ezRenderSortingFunctions::BackToFrontThenByRenderData);
ezRenderData::Category ezDefaultRenderDataCategories::Foreground1 = ezRenderData::RegisterCategory("Foreground1", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::Foreground2 = ezRenderData::RegisterCategory("Foreground2", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::Selection = ezRenderData::RegisterCategory("Selection", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);

