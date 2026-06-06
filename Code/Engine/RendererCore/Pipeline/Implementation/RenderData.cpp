#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/SortingFunctions.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezInstanceableRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgExtractRenderData);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgExtractRenderData, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezExcludeFromScript()
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgExtractOccluderData);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgExtractOccluderData, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezExcludeFromScript()
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgCustomInstanceDataOffsetChanged);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgCustomInstanceDataOffsetChanged, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezExcludeFromScript()
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
static_assert(sizeof(ezRenderData) == 48);
static_assert(sizeof(ezInstanceableRenderData) == 72);
#else
static_assert(sizeof(ezRenderData) == 40);
static_assert(sizeof(ezInstanceableRenderData) == 64);
#endif

ezHybridArray<ezRenderData::CategoryData, 32> ezRenderData::s_CategoryData;

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
ezRenderData::Category ezRenderData::RegisterDerivedCategory(const char* szCategoryName, Category baseCategory)
{
  auto& baseCategoryData = s_CategoryData[baseCategory.m_uiValue];

  Category derivedCategory = RegisterCategory(szCategoryName, baseCategoryData.m_sortingKeyFunc);
  s_CategoryData[derivedCategory.m_uiValue].m_baseCategory = baseCategory;

  return derivedCategory;
}

// static
ezRenderData::Category ezRenderData::RegisterRedirectedCategory(const char* szCategoryName, Category staticCategory, Category dynamicCategory)
{
  Category newCategory = RegisterCategory(szCategoryName, nullptr);
  s_CategoryData[newCategory.m_uiValue].m_staticCategory = staticCategory;
  s_CategoryData[newCategory.m_uiValue].m_dynamicCategory = dynamicCategory;

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
ezRenderData::Category ezRenderData::ResolveCategory(Category category, bool bDynamic)
{
  auto& categoryData = s_CategoryData[category.m_uiValue];
  if (categoryData.m_staticCategory != ezInvalidRenderDataCategory)
  {
    return bDynamic ? categoryData.m_dynamicCategory : categoryData.m_staticCategory;
  }

  return category;
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

//////////////////////////////////////////////////////////////////////////

ezRenderData::Category ezDefaultRenderDataCategories::Light = ezRenderData::RegisterCategory("Light", &ezRenderSortingFunctions::BySortingKeyOnlyFunc);
ezRenderData::Category ezDefaultRenderDataCategories::Decal = ezRenderData::RegisterCategory("Decal", &ezRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);
ezRenderData::Category ezDefaultRenderDataCategories::ReflectionProbe = ezRenderData::RegisterCategory("ReflectionProbe", &ezRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);
ezRenderData::Category ezDefaultRenderDataCategories::Sky = ezRenderData::RegisterCategory("Sky", &ezRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);

ezRenderData::Category ezDefaultRenderDataCategories::LitOpaqueStatic = ezRenderData::RegisterCategory("LitOpaqueStatic", &ezRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);
ezRenderData::Category ezDefaultRenderDataCategories::LitOpaqueDynamic = ezRenderData::RegisterCategory("LitOpaqueDynamic", &ezRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);
ezRenderData::Category ezDefaultRenderDataCategories::LitOpaque = ezRenderData::RegisterRedirectedCategory("LitOpaque", ezDefaultRenderDataCategories::LitOpaqueStatic, ezDefaultRenderDataCategories::LitOpaqueDynamic);

ezRenderData::Category ezDefaultRenderDataCategories::LitMaskedStatic = ezRenderData::RegisterCategory("LitMaskedStatic", &ezRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);
ezRenderData::Category ezDefaultRenderDataCategories::LitMaskedDynamic = ezRenderData::RegisterCategory("LitMaskedDynamic", &ezRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);
ezRenderData::Category ezDefaultRenderDataCategories::LitMasked = ezRenderData::RegisterRedirectedCategory("LitMasked", ezDefaultRenderDataCategories::LitMaskedStatic, ezDefaultRenderDataCategories::LitMaskedDynamic);

ezRenderData::Category ezDefaultRenderDataCategories::LitMeshDecal = ezRenderData::RegisterCategory("LitMeshDecal", &ezRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);
ezRenderData::Category ezDefaultRenderDataCategories::LitTransparent = ezRenderData::RegisterCategory("LitTransparent", &ezRenderSortingFunctions::BackToFrontThenByRenderDataFunc);
ezRenderData::Category ezDefaultRenderDataCategories::LitForeground = ezRenderData::RegisterCategory("LitForeground", &ezRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);

ezRenderData::Category ezDefaultRenderDataCategories::LensEffects = ezRenderData::RegisterCategory("LensEffects", &ezRenderSortingFunctions::BackToFrontThenByRenderDataFunc);

ezRenderData::Category ezDefaultRenderDataCategories::SimpleOpaque = ezRenderData::RegisterCategory("SimpleOpaque", &ezRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);
ezRenderData::Category ezDefaultRenderDataCategories::SimpleTransparent = ezRenderData::RegisterCategory("SimpleTransparent", &ezRenderSortingFunctions::BackToFrontThenByRenderDataFunc);
ezRenderData::Category ezDefaultRenderDataCategories::SimpleForeground = ezRenderData::RegisterCategory("SimpleForeground", &ezRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);

ezRenderData::Category ezDefaultRenderDataCategories::Selection = ezRenderData::RegisterCategory("Selection", &ezRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);
ezRenderData::Category ezDefaultRenderDataCategories::GUI = ezRenderData::RegisterCategory("GUI", &ezRenderSortingFunctions::BackToFrontThenByRenderDataFunc);

//////////////////////////////////////////////////////////////////////////

void ezMsgExtractRenderData::AddRenderData(const ezRenderData* pRenderData, ezRenderData::Category category, ezRenderData::Caching::Enum cachingBehavior)
{
  auto& cached = m_ExtractedRenderData.ExpandAndGetRef();
  cached.m_pRenderData = pRenderData;
  cached.m_Category = ezRenderData::ResolveCategory(category, pRenderData->m_Flags.IsSet(ezRenderData::Flags::Dynamic));

  if (cachingBehavior == ezRenderData::Caching::IfStatic)
  {
    ++m_uiNumCacheIfStatic;
  }
}

void ezMsgExtractRenderData::AddDependency(ezGALTextureHandle hTexture, ezRenderData::Category category, ezBitflags<ezGALResourceState> requiredState, ezBitflags<ezGALShaderStageFlags> stage)
{
  EZ_ASSERT_DEBUG(requiredState != ezGALResourceState::Unknown, "The required state must be valid");
  if (hTexture.IsInvalidated())
    return;
  auto& dep = m_TextureDependencies.ExpandAndGetRef();
  dep.m_hTexture = hTexture;
  dep.m_RequiredState = requiredState;
  dep.m_Stage = stage;
  dep.m_uiCategory = category.m_uiValue;
}

void ezMsgExtractRenderData::AddDependency(ezGALBufferHandle hBuffer, ezRenderData::Category category, ezBitflags<ezGALResourceState> requiredState, ezBitflags<ezGALShaderStageFlags> stage)
{
  EZ_ASSERT_DEBUG(requiredState != ezGALResourceState::Unknown, "The required state must be valid");
  if (hBuffer.IsInvalidated())
    return;
  auto& dep = m_BufferDependencies.ExpandAndGetRef();
  dep.m_hBuffer = hBuffer;
  dep.m_RequiredState = requiredState;
  dep.m_Stage = stage;
  dep.m_uiCategory = category.m_uiValue;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderData);
