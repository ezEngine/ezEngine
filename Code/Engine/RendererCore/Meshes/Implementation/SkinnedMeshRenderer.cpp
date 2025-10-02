#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Meshes/SkinnedMeshComponent.h>
#include <RendererCore/Meshes/SkinnedMeshRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSkinnedMeshRenderer, 1, ezRTTIDefaultAllocator<ezSkinnedMeshRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSkinnedMeshRenderer::ezSkinnedMeshRenderer() = default;
ezSkinnedMeshRenderer::~ezSkinnedMeshRenderer() = default;

void ezSkinnedMeshRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(ezGetStaticRTTI<ezSkinnedMeshRenderData>());
}

void ezSkinnedMeshRenderer::SetAdditionalData(const ezRenderViewContext& renderViewContext, const ezMeshRenderData* pRenderData) const
{
  // Don't call base class implementation here since the state will be overwritten in this method anyways.

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezRenderContext* pContext = renderViewContext.m_pRenderContext;

  auto pSkinnedRenderData = static_cast<const ezSkinnedMeshRenderData*>(pRenderData);
  auto pSkinningBuffer = pDevice->GetBuffer(pSkinnedRenderData->m_hSkinningTransforms);

  if (pSkinningBuffer == nullptr)
  {
    pContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");
  }
  else
  {
    pContext->SetShaderPermutationVariable("VERTEX_SKINNING", "TRUE");

    ezBindGroupBuilder& bindGroupDraw = pContext->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
    bindGroupDraw.BindBuffer("skinningTransforms", pSkinnedRenderData->m_hSkinningTransforms);
  }
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_SkinnedMeshRenderer);
