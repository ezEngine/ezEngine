#include <RmlUiPluginPCH.h>

#include <RmlUiPlugin/Implementation/RmlUiRenderer.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRmlUiRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRmlUiRenderer, 1, ezRTTIDefaultAllocator<ezRmlUiRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezRmlUiRenderer::ezRmlUiRenderer() = default;
ezRmlUiRenderer::~ezRmlUiRenderer() = default;

void ezRmlUiRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) const
{
  types.PushBack(ezGetStaticRTTI<ezRmlUiRenderData>());
}

void ezRmlUiRenderer::GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& categories) const
{
  categories.PushBack(ezDefaultRenderDataCategories::GUI);
}

void ezRmlUiRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
}
