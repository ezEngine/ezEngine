#include <PCH.h>

#include <KrautPlugin/Renderer/KrautFrondsRenderer.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautFrondsRenderer, 1, ezRTTIDefaultAllocator<ezKrautFrondsRenderer>);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezKrautFrondsRenderer::ezKrautFrondsRenderer() = default;
ezKrautFrondsRenderer::~ezKrautFrondsRenderer() = default;

void ezKrautFrondsRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types)
{
  //types.PushBack(ezGetStaticRTTI<ezKrautFrondsRenderData>());
}

void ezKrautFrondsRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch)
{
}

void ezKrautFrondsRenderer::FillPerInstanceData(ezArrayPtr<ezPerInstanceData> instanceData, const ezRenderDataBatch& batch, ezUInt32 uiStartIndex, ezUInt32& out_uiFilteredCount)
{
}

