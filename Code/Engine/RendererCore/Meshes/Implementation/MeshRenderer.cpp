#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/Implementation/MeshRendererUtils.h>
#include <RendererCore/Meshes/InstancedMeshComponent.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshRenderer, 1, ezRTTIDefaultAllocator<ezMeshRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezMeshRenderer::ezMeshRenderer() = default;
ezMeshRenderer::~ezMeshRenderer() = default;

void ezMeshRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(ezGetStaticRTTI<ezMeshRenderData>());
  ref_types.PushBack(ezGetStaticRTTI<ezInstancedMeshRenderData>());
}

void ezMeshRenderer::GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(ezDefaultRenderDataCategories::Sky);
  ref_categories.PushBack(ezDefaultRenderDataCategories::LitOpaque);
  ref_categories.PushBack(ezDefaultRenderDataCategories::LitMasked);
  ref_categories.PushBack(ezDefaultRenderDataCategories::LitTransparent);
  ref_categories.PushBack(ezDefaultRenderDataCategories::LitForeground);
  ref_categories.PushBack(ezDefaultRenderDataCategories::SimpleOpaque);
  ref_categories.PushBack(ezDefaultRenderDataCategories::SimpleTransparent);
  ref_categories.PushBack(ezDefaultRenderDataCategories::SimpleForeground);
  ref_categories.PushBack(ezDefaultRenderDataCategories::Selection);
  ref_categories.PushBack(ezDefaultRenderDataCategories::GUI);
}

void ezMeshRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  ezRenderContext* pContext = renderViewContext.m_pRenderContext;

  const ezMeshRenderData* pRenderData = batch.GetFirstData<ezMeshRenderData>();

  const ezMeshResourceHandle& hMesh = pRenderData->m_hMesh;
  const ezMaterialResourceHandle& hMaterial = pRenderData->m_hMaterial;
  const ezUInt32 uiPartIndex = pRenderData->m_uiSubMeshIndex;
  const bool bHasExplicitInstanceData = pRenderData->IsInstanceOf<ezInstancedMeshRenderData>();

  ezResourceLock<ezMeshResource> pMesh(hMesh, ezResourceAcquireMode::AllowLoadingFallback);

  // This can happen when the resource has been reloaded and now has fewer submeshes.
  const auto& subMeshes = pMesh->GetSubMeshes();
  if (subMeshes.GetCount() <= uiPartIndex)
  {
    return;
  }

  ezInstanceData* pInstanceData = bHasExplicitInstanceData ? static_cast<const ezInstancedMeshRenderData*>(pRenderData)->m_pExplicitInstanceData : pPass->GetPipeline()->GetFrameDataProvider<ezInstanceDataProvider>()->GetData(renderViewContext);


  if (pRenderData->m_uiFlipWinding)
  {
    pContext->SetShaderPermutationVariable("FLIP_WINDING", "TRUE");
  }
  else
  {
    pContext->SetShaderPermutationVariable("FLIP_WINDING", "FALSE");
  }

  pContext->BindMaterial(hMaterial);
  pContext->BindMeshBuffer(pMesh->GetMeshBuffer());

  SetAdditionalData(renderViewContext, pRenderData);

  pInstanceData->BindResources(pContext);

  if (!bHasExplicitInstanceData)
  {
    ezUInt32 uiStartIndex = 0;
    while (uiStartIndex < batch.GetCount())
    {
      const ezUInt32 uiRemainingInstances = batch.GetCount() - uiStartIndex;

      ezUInt32 uiInstanceDataOffset = 0;
      ezArrayPtr<ezPerInstanceData> instanceData = pInstanceData->GetInstanceData(pContext, uiRemainingInstances, uiInstanceDataOffset);

      ezUInt32 uiFilteredCount = 0;
      FillPerInstanceData(instanceData, batch, uiStartIndex, uiFilteredCount);

      if (uiFilteredCount > 0) // Instance data might be empty if all render data was filtered.
      {
        pInstanceData->UpdateInstanceData(pContext, uiFilteredCount);

        const ezMeshResourceDescriptor::SubMesh& meshPart = subMeshes[uiPartIndex];

        if (pContext->DrawMeshBuffer(meshPart.m_uiPrimitiveCount, meshPart.m_uiFirstPrimitive, uiFilteredCount).Failed())
        {
          for (auto it = batch.GetIterator<ezMeshRenderData>(uiStartIndex, instanceData.GetCount()); it.IsValid(); ++it)
          {
            pRenderData = it;

            // draw bounding box instead
            if (pRenderData->m_GlobalBounds.IsValid())
            {
              ezDebugRenderer::DrawLineBox(*renderViewContext.m_pViewDebugContext, pRenderData->m_GlobalBounds.GetBox(), ezColor::Magenta);
            }
          }
        }
      }

      uiStartIndex += instanceData.GetCount();
    }
  }
  else
  {
    ezUInt32 uiInstanceCount = static_cast<const ezInstancedMeshRenderData*>(pRenderData)->m_uiExplicitInstanceCount;

    const ezMeshResourceDescriptor::SubMesh& meshPart = subMeshes[uiPartIndex];

    pContext->DrawMeshBuffer(meshPart.m_uiPrimitiveCount, meshPart.m_uiFirstPrimitive, uiInstanceCount).IgnoreResult();
  }
}

void ezMeshRenderer::SetAdditionalData(const ezRenderViewContext& renderViewContext, const ezMeshRenderData* pRenderData) const
{
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");
}

void ezMeshRenderer::FillPerInstanceData(ezArrayPtr<ezPerInstanceData> instanceData, const ezRenderDataBatch& batch, ezUInt32 uiStartIndex, ezUInt32& out_uiFilteredCount) const
{
  ezUInt32 uiCount = ezMath::Min<ezUInt32>(instanceData.GetCount(), batch.GetCount() - uiStartIndex);
  ezUInt32 uiCurrentIndex = 0;

  for (auto it = batch.GetIterator<ezMeshRenderData>(uiStartIndex, uiCount); it.IsValid(); ++it)
  {
    ezInternal::FillPerInstanceData(instanceData[uiCurrentIndex], it);

    ++uiCurrentIndex;
  }

  out_uiFilteredCount = uiCurrentIndex;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshRenderer);
