#include <RendererCorePCH.h>

#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/SortingFunctions.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderData, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderer, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgExtractRenderData);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgExtractRenderData, 1, ezRTTIDefaultAllocator<ezMsgExtractRenderData>);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezHybridArray<ezRenderData::CategoryData, 32> ezRenderData::s_CategoryData;

// static
ezRenderData::Category ezRenderData::RegisterCategory(const char* szCategoryName, SortingKeyFunc sortingKeyFunc)
{
  Category oldCategory = FindCategory(szCategoryName);
  if (oldCategory != ezInvalidRenderDataCategory)
    return oldCategory;

  Category newCategory = Category(s_CategoryData.GetCount());

  auto& data = s_CategoryData.ExpandAndGetRef();
  data.m_sName.Assign(szCategoryName);
  data.m_sortingKeyFunc = sortingKeyFunc;

  return newCategory;
}

ezRenderData::Category ezRenderData::FindCategory(const char* szCategoryName)
{
  ezTempHashedString categoryName(szCategoryName);

  for (ezUInt32 uiCategoryIndex = 0; uiCategoryIndex < s_CategoryData.GetCount(); ++uiCategoryIndex)
  {
    if (s_CategoryData[uiCategoryIndex].m_sName == categoryName)
      return Category(uiCategoryIndex);
  }

  return ezInvalidRenderDataCategory;
}

//////////////////////////////////////////////////////////////////////////

ezRenderData::Category ezDefaultRenderDataCategories::Light =
  ezRenderData::RegisterCategory("Light", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::Decal =
  ezRenderData::RegisterCategory("Decal", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::Sky =
  ezRenderData::RegisterCategory("Sky", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::LitOpaque =
  ezRenderData::RegisterCategory("LitOpaque", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::LitMasked =
  ezRenderData::RegisterCategory("LitMasked", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::LitTransparent =
  ezRenderData::RegisterCategory("LitTransparent", &ezRenderSortingFunctions::BackToFrontThenByRenderData);
ezRenderData::Category ezDefaultRenderDataCategories::LitForeground =
  ezRenderData::RegisterCategory("LitForeground", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::SimpleOpaque =
  ezRenderData::RegisterCategory("SimpleOpaque", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::SimpleTransparent =
  ezRenderData::RegisterCategory("SimpleTransparent", &ezRenderSortingFunctions::BackToFrontThenByRenderData);
ezRenderData::Category ezDefaultRenderDataCategories::SimpleForeground =
  ezRenderData::RegisterCategory("SimpleForeground", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::Selection =
  ezRenderData::RegisterCategory("Selection", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::GUI =
  ezRenderData::RegisterCategory("GUI", &ezRenderSortingFunctions::BackToFrontThenByRenderData);

//////////////////////////////////////////////////////////////////////////

void ezMsgExtractRenderData::AddRenderData(
  const ezRenderData* pRenderData, ezRenderData::Category category, ezRenderData::Caching::Enum cachingBehavior)
{
  auto& cached = m_ExtractedRenderData.ExpandAndGetRef();
  cached.m_pRenderData = pRenderData;
  cached.m_uiCategory = category.m_uiValue;
  cached.m_uiComponentIndex = 0x7FFF;
  cached.m_uiCacheIfStatic = (cachingBehavior == ezRenderData::Caching::IfStatic);
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderData);
