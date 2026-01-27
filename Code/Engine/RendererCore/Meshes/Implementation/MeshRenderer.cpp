#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/CustomMeshComponent.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/Meshes/SkinnedMeshRenderData.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Resources/DynamicBuffer.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshRenderer, 1, ezRTTIDefaultAllocator<ezMeshRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezMeshRenderer::ezMeshRenderer() = default;
ezMeshRenderer::~ezMeshRenderer() = default;

void ezMeshRenderer::GetSupportedRenderDataTypes(ezDynamicArray<const ezRTTI*>& out_types) const
{
  out_types.PushBack(ezGetStaticRTTI<ezMeshRenderData>());
  out_types.PushBack(ezGetStaticRTTI<ezCustomMeshRenderData>());
  out_types.PushBack(ezGetStaticRTTI<ezSkinnedMeshRenderData>());
}

void ezMeshRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezRenderContext* pContext = renderViewContext.m_pRenderContext;
  auto& bg = pContext->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);

  const ezInstanceableRenderData* pRenderData = batch.GetFirstData<ezInstanceableRenderData>();

  ezUInt32 uiPrimitiveCount = 0;
  ezUInt32 uiFirstPrimitive = 0;
  bool bUseSkinning = false;
  if (pRenderData->IsInstanceOf<ezCustomMeshRenderData>())
  {
    const auto* pCustomMeshRenderData = static_cast<const ezCustomMeshRenderData*>(pRenderData);
    uiPrimitiveCount = pCustomMeshRenderData->m_uiNumPrimitives;
    uiFirstPrimitive = pCustomMeshRenderData->m_uiFirstPrimitive;

    pContext->BindMeshBuffer(pCustomMeshRenderData->m_hDynamicMeshBuffer, batch.GetDataOffsetsBuffer(), batch.GetFirstDataOffsetIndex());
    pContext->BindMaterial(pCustomMeshRenderData->m_hMaterial);
  }
  else
  {
    const auto* pMeshRenderData = static_cast<const ezMeshRenderData*>(pRenderData);
    const ezUInt32 uiPartIndex = pMeshRenderData->m_uiSubMeshIndex;

    const ezMeshResourceHandle& hMesh = pMeshRenderData->m_hMesh;
    ezResourceLock<ezMeshResource> pMesh(hMesh, ezResourceAcquireMode::AllowLoadingFallback);

    // This can happen when the resource has been reloaded and now has fewer submeshes.
    const auto& subMeshes = pMesh->GetSubMeshes();
    if (subMeshes.GetCount() <= uiPartIndex)
    {
      return;
    }

    const ezMeshResourceDescriptor::SubMesh& meshPart = subMeshes[uiPartIndex];
    uiPrimitiveCount = meshPart.m_uiPrimitiveCount;
    uiFirstPrimitive = meshPart.m_uiFirstPrimitive;

    pContext->BindMeshBuffer(pMesh->GetMeshBuffer(), batch.GetDataOffsetsBuffer(), batch.GetFirstDataOffsetIndex());
    pContext->BindMaterial(pMeshRenderData->m_hMaterial);

    if (auto pCustomInstanceDataBuffer = pDevice->GetDynamicBuffer(pMeshRenderData->m_hCustomInstanceDataBuffer))
    {
      bg.BindBuffer("perInstanceDataCustom", pCustomInstanceDataBuffer->GetBufferForRendering());
    }

    if (auto pSkinnedMeshRenderData = ezDynamicCast<const ezSkinnedMeshRenderData*>(pRenderData))
    {
      if (auto pSkinningBuffer = pDevice->GetDynamicBuffer(pSkinnedMeshRenderData->m_hSkinningBuffer))
      {
        bg.BindBuffer("skinningTransforms", pSkinningBuffer->GetBufferForRendering());
        bUseSkinning = true;
      }
    }

    SetAdditionalData(renderViewContext, pMeshRenderData);
  }

  constexpr ezTempHashedString sTrue("TRUE");
  constexpr ezTempHashedString sFalse("FALSE");

  pContext->SetShaderPermutationVariable("VERTEX_SKINNING", bUseSkinning ? sTrue : sFalse);
  pContext->SetShaderPermutationVariable("FLIP_WINDING", pRenderData->FlipWinding() ? sTrue : sFalse);

  if (auto pInstanceDataBuffer = pDevice->GetDynamicBuffer(pRenderData->m_hInstanceDataBuffer))
  {
    bg.BindBuffer("perInstanceData", pInstanceDataBuffer->GetBufferForRendering());
  }

  if (pContext->DrawMeshBuffer(uiPrimitiveCount, uiFirstPrimitive, batch.GetInstanceCount()).Failed())
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    // draw bounding box instead
    for (auto it = batch.GetIterator<ezRenderData>(); it.IsValid(); ++it)
    {
      if (auto pMeshRenderData = ezDynamicCast<const ezMeshRenderData*>(it))
      {
        if (pMeshRenderData->m_FallbackGlobalBBox.IsValid())
        {
          ezDebugRenderer::DrawLineBox(*renderViewContext.m_pViewDebugContext, pMeshRenderData->m_FallbackGlobalBBox, ezColor::Magenta);
        }
      }
      else if (auto pCustomMeshRenderData = ezDynamicCast<const ezCustomMeshRenderData*>(it))
      {
        // draw bounding box instead
        if (pCustomMeshRenderData->m_FallbackGlobalBBox.IsValid())
        {
          ezDebugRenderer::DrawLineBox(*renderViewContext.m_pViewDebugContext, pCustomMeshRenderData->m_FallbackGlobalBBox, ezColor::Magenta);
        }
      }
    }
#endif
  }
}

void ezMeshRenderer::SetAdditionalData(const ezRenderViewContext& renderViewContext, const ezMeshRenderData* pRenderData) const
{
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshRenderer);
