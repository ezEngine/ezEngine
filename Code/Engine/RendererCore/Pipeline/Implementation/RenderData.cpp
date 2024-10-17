#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/Renderer.h>
#include <RendererCore/Pipeline/SortingFunctions.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, RenderData)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    ezRenderData::UpdateRendererTypes();

    ezPlugin::Events().AddEventHandler(ezRenderData::PluginEventHandler);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    ezPlugin::Events().RemoveEventHandler(ezRenderData::PluginEventHandler);

    ezRenderData::ClearRendererInstances();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderer, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgExtractRenderData);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgExtractRenderData, 1, ezRTTIDefaultAllocator<ezMsgExtractRenderData>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezExcludeFromScript()
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgExtractOccluderData);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgExtractOccluderData, 1, ezRTTIDefaultAllocator<ezMsgExtractOccluderData>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezExcludeFromScript()
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezHybridArray<ezRenderData::CategoryData, 32> ezRenderData::s_CategoryData;

ezHybridArray<const ezRTTI*, 16> ezRenderData::s_RendererTypes;
ezDynamicArray<ezUniquePtr<ezRenderer>> ezRenderData::s_RendererInstances;
bool ezRenderData::s_bRendererInstancesDirty = false;

// static
ezRenderData::Category ezRenderData::RegisterCategory(const char* szCategoryName, SortingKeyFunc sortingKeyFunc)
{
  ezHashedString sCategoryName;
  sCategoryName.Assign(szCategoryName);

  Category oldCategory = FindCategory(sCategoryName);
  if (oldCategory != ezInvalidRenderDataCategory)
    return oldCategory;

  Category newCategory = Category(static_cast<ezUInt16>(s_CategoryData.GetCount()));

  auto& data = s_CategoryData.ExpandAndGetRef();
  data.m_sName = sCategoryName;
  data.m_sortingKeyFunc = sortingKeyFunc;

  return newCategory;
}

// static
ezRenderData::Category ezRenderData::FindCategory(ezTempHashedString sCategoryName)
{
  for (ezUInt32 uiCategoryIndex = 0; uiCategoryIndex < s_CategoryData.GetCount(); ++uiCategoryIndex)
  {
    if (s_CategoryData[uiCategoryIndex].m_sName == sCategoryName)
      return Category(static_cast<ezUInt16>(uiCategoryIndex));
  }

  return ezInvalidRenderDataCategory;
}

// static
void ezRenderData::GetAllCategoryNames(ezDynamicArray<ezHashedString>& out_categoryNames)
{
  out_categoryNames.Clear();

  for (auto& data : s_CategoryData)
  {
    out_categoryNames.PushBack(data.m_sName);
  }
}

// static
void ezRenderData::PluginEventHandler(const ezPluginEvent& e)
{
  switch (e.m_EventType)
  {
    case ezPluginEvent::AfterPluginChanges:
      UpdateRendererTypes();
      break;

    default:
      break;
  }
}

// static
void ezRenderData::UpdateRendererTypes()
{
  s_RendererTypes.Clear();

  ezRTTI::ForEachDerivedType<ezRenderer>([](const ezRTTI* pRtti)
    { s_RendererTypes.PushBack(pRtti); },
    ezRTTI::ForEachOptions::ExcludeNonAllocatable);

  s_bRendererInstancesDirty = true;
}

// static
void ezRenderData::CreateRendererInstances()
{
  ClearRendererInstances();

  for (auto pRendererType : s_RendererTypes)
  {
    EZ_ASSERT_DEV(pRendererType->IsDerivedFrom(ezGetStaticRTTI<ezRenderer>()), "Renderer type '{}' must be derived from ezRenderer",
      pRendererType->GetTypeName());

    auto pRenderer = pRendererType->GetAllocator()->Allocate<ezRenderer>();

    ezUInt32 uiIndex = s_RendererInstances.GetCount();
    s_RendererInstances.PushBack(pRenderer);

    ezHybridArray<Category, 8> supportedCategories;
    pRenderer->GetSupportedRenderDataCategories(supportedCategories);

    ezHybridArray<const ezRTTI*, 8> supportedTypes;
    pRenderer->GetSupportedRenderDataTypes(supportedTypes);

    for (Category category : supportedCategories)
    {
      auto& categoryData = s_CategoryData[category.m_uiValue];

      for (ezUInt32 i = 0; i < supportedTypes.GetCount(); ++i)
      {
        categoryData.m_TypeToRendererIndex.Insert(supportedTypes[i], uiIndex);
      }
    }
  }

  s_bRendererInstancesDirty = false;
}

// static
void ezRenderData::ClearRendererInstances()
{
  s_RendererInstances.Clear();

  for (auto& categoryData : s_CategoryData)
  {
    categoryData.m_TypeToRendererIndex.Clear();
  }
}

//////////////////////////////////////////////////////////////////////////

ezRenderData::Category ezDefaultRenderDataCategories::Light = ezRenderData::RegisterCategory("Light", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::Decal = ezRenderData::RegisterCategory("Decal", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::ReflectionProbe = ezRenderData::RegisterCategory("ReflectionProbe", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::Sky = ezRenderData::RegisterCategory("Sky", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::LitOpaque = ezRenderData::RegisterCategory("LitOpaque", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::LitMasked = ezRenderData::RegisterCategory("LitMasked", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::LitTransparent = ezRenderData::RegisterCategory("LitTransparent", &ezRenderSortingFunctions::BackToFrontThenByRenderData);
ezRenderData::Category ezDefaultRenderDataCategories::LitForeground = ezRenderData::RegisterCategory("LitForeground", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::LitScreenFX = ezRenderData::RegisterCategory("LitScreenFX", &ezRenderSortingFunctions::BackToFrontThenByRenderData);
ezRenderData::Category ezDefaultRenderDataCategories::SimpleOpaque = ezRenderData::RegisterCategory("SimpleOpaque", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::SimpleTransparent = ezRenderData::RegisterCategory("SimpleTransparent", &ezRenderSortingFunctions::BackToFrontThenByRenderData);
ezRenderData::Category ezDefaultRenderDataCategories::SimpleForeground = ezRenderData::RegisterCategory("SimpleForeground", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::Selection = ezRenderData::RegisterCategory("Selection", &ezRenderSortingFunctions::ByRenderDataThenFrontToBack);
ezRenderData::Category ezDefaultRenderDataCategories::GUI = ezRenderData::RegisterCategory("GUI", &ezRenderSortingFunctions::BackToFrontThenByRenderData);

//////////////////////////////////////////////////////////////////////////

void ezMsgExtractRenderData::AddRenderData(const ezRenderData* pRenderData, ezRenderData::Category category, ezRenderData::Caching::Enum cachingBehavior)
{
  auto& cached = m_ExtractedRenderData.ExpandAndGetRef();
  cached.m_pRenderData = pRenderData;
  cached.m_uiCategory = category.m_uiValue;

  if (cachingBehavior == ezRenderData::Caching::IfStatic)
  {
    ++m_uiNumCacheIfStatic;
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderData);
