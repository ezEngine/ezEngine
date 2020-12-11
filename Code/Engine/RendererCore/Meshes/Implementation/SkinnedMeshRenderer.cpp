#include <RendererCorePCH.h>

#include <RendererCore/Meshes/SkinnedMeshComponent.h>
#include <RendererCore/Meshes/SkinnedMeshRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSkinnedMeshRenderer, 1, ezRTTIDefaultAllocator<ezSkinnedMeshRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSkinnedMeshRenderer::ezSkinnedMeshRenderer() = default;
ezSkinnedMeshRenderer::~ezSkinnedMeshRenderer() = default;

void ezSkinnedMeshRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) const
{
  types.PushBack(ezGetStaticRTTI<ezSkinnedMeshRenderData>());
}

void ezSkinnedMeshRenderer::SetAdditionalData(const ezRenderViewContext& renderViewContext, const ezMeshRenderData* pRenderData) const
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezRenderContext* pContext = renderViewContext.m_pRenderContext;

  auto pSkinnedRenderData = static_cast<const ezSkinnedMeshRenderData*>(pRenderData);

  if (pSkinnedRenderData->m_hSkinningMatrices.IsInvalidated())
  {
    pContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");
  }
  else
  {
    pContext->SetShaderPermutationVariable("VERTEX_SKINNING", "TRUE");

    if (!pSkinnedRenderData->m_pNewSkinningMatricesData.IsEmpty())
    {
      pContext->GetCommandEncoder()->UpdateBuffer(pSkinnedRenderData->m_hSkinningMatrices, 0, pSkinnedRenderData->m_pNewSkinningMatricesData);
    }

    pContext->BindBuffer("skinningMatrices", pDevice->GetDefaultResourceView(pSkinnedRenderData->m_hSkinningMatrices));
  }
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_SkinnedMeshRenderer);

