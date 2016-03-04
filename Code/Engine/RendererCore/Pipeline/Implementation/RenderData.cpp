#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/RenderData.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderData, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderer, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_IMPLEMENT_MESSAGE_TYPE(ezExtractRenderDataMessage);

namespace
{
  struct CategoryData
  {
    ezHashedString m_sName;
    ezProfilingId m_ProfilingID;
  };

  static ezHybridArray<CategoryData, 32> s_CategoryData;
}

//static 
ezRenderData::Category ezRenderData::FindOrRegisterCategory(const char* szCategoryName)
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

  return newCategory;
}

//static 
const char* ezRenderData::GetCategoryName(Category category)
{
  return s_CategoryData[category].m_sName.GetString().GetData();
}

//static
ezProfilingId& ezRenderData::GetCategoryProfilingID(Category category)
{
  return s_CategoryData[category].m_ProfilingID;
}

ezRenderData::Category ezDefaultRenderDataCategories::Light = ezRenderData::FindOrRegisterCategory( "Light" );
ezRenderData::Category ezDefaultRenderDataCategories::Opaque = ezRenderData::FindOrRegisterCategory("Opaque");
ezRenderData::Category ezDefaultRenderDataCategories::Masked = ezRenderData::FindOrRegisterCategory("Masked");
ezRenderData::Category ezDefaultRenderDataCategories::Transparent = ezRenderData::FindOrRegisterCategory("Transparent");
ezRenderData::Category ezDefaultRenderDataCategories::Foreground1 = ezRenderData::FindOrRegisterCategory("Foreground1");
ezRenderData::Category ezDefaultRenderDataCategories::Foreground2 = ezRenderData::FindOrRegisterCategory("Foreground2");
ezRenderData::Category ezDefaultRenderDataCategories::Selection = ezRenderData::FindOrRegisterCategory("Selection");

