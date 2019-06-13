#include <ProcGenPluginPCH.h>

#include <ProcGenPlugin/Components/ProcVertexColorComponent.h>
#include <ProcGenPlugin/Components/ProcVertexColorRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcVertexColorRenderer, 1, ezRTTIDefaultAllocator<ezProcVertexColorRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezProcVertexColorRenderer::ezProcVertexColorRenderer() = default;
ezProcVertexColorRenderer::~ezProcVertexColorRenderer() = default;

void ezProcVertexColorRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) const
{
  types.PushBack(ezGetStaticRTTI<ezProcVertexColorRenderData>());
}

void ezProcVertexColorRenderer::SetAdditionalData(const ezRenderViewContext& renderViewContext, const ezMeshRenderData* pRenderData) const
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezRenderContext* pContext = renderViewContext.m_pRenderContext;

  auto pProcVertexColorRenderData = static_cast<const ezProcVertexColorRenderData*>(pRenderData);

  pContext->BindBuffer("vertexColors", pDevice->GetDefaultResourceView(pProcVertexColorRenderData->m_hVertexColorBuffer));
}
