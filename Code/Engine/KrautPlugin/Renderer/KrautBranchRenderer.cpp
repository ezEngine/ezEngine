#include <PCH.h>

#include <KrautPlugin/Renderer/KrautBranchRenderer.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautBranchRenderData, 1, ezRTTIDefaultAllocator<ezKrautBranchRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautBranchRenderer, 1, ezRTTIDefaultAllocator<ezKrautBranchRenderer>);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezKrautBranchRenderer::ezKrautBranchRenderer() = default;
ezKrautBranchRenderer::~ezKrautBranchRenderer() = default;

void ezKrautBranchRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types)
{
  types.PushBack(ezGetStaticRTTI<ezKrautBranchRenderData>());
}

void ezKrautBranchRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass,
                                        const ezRenderDataBatch& batch)
{
  ezRenderContext* pContext = renderViewContext.m_pRenderContext;

  const ezKrautBranchRenderData* pRenderData = batch.GetFirstData<ezKrautBranchRenderData>();

  ezResourceLock<ezMeshResource> pMesh(pRenderData->m_hMesh);

  // This can happen when the resource has been reloaded and now has fewer sub-meshes.
  if (pMesh->GetSubMeshes().GetCount() <= pRenderData->m_uiPartIndex)
    return;

  const auto& subMesh = pMesh->GetSubMeshes()[pRenderData->m_uiPartIndex];

  ezInstanceData* pInstanceData = pPass->GetPipeline()->GetFrameDataProvider<ezInstanceDataProvider>()->GetData(renderViewContext);
  pInstanceData->BindResources(pContext);

  // inverted trees are not allowed
  pContext->SetShaderPermutationVariable("FLIP_WINDING", "FALSE");
  // no skinning atm
  pContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");

  pContext->BindMaterial(pMesh->GetMaterials()[subMesh.m_uiMaterialIndex]);
  pContext->BindMeshBuffer(pMesh->GetMeshBuffer());

  const ezVec3 vLodCamPos = renderViewContext.m_pCamera->GetPosition();

  for (ezUInt32 uiStartIndex = 0; uiStartIndex < batch.GetCount(); /**/)
  {
    const ezUInt32 uiRemainingInstances = batch.GetCount() - uiStartIndex;

    ezUInt32 uiInstanceDataOffset = 0;
    ezArrayPtr<ezPerInstanceData> instanceData = pInstanceData->GetInstanceData(uiRemainingInstances, uiInstanceDataOffset);

    ezUInt32 uiFilteredCount = 0;
    FillPerInstanceData(vLodCamPos, instanceData, batch, uiStartIndex, uiFilteredCount);

    if (uiFilteredCount > 0) // Instance data might be empty if all render data was filtered.
    {
      pInstanceData->UpdateInstanceData(pContext, uiFilteredCount);

      ezUInt32 uiRenderedInstances = uiFilteredCount;

      if (renderViewContext.m_pCamera->IsStereoscopic())
        uiRenderedInstances *= 2;

      if (pContext->DrawMeshBuffer(subMesh.m_uiPrimitiveCount, subMesh.m_uiFirstPrimitive, uiRenderedInstances).Failed())
      {
        for (auto it = batch.GetIterator<ezKrautBranchRenderData>(uiStartIndex, instanceData.GetCount()); it.IsValid(); ++it)
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

void ezKrautBranchRenderer::FillPerInstanceData(const ezVec3& vLodCamPos, ezArrayPtr<ezPerInstanceData> instanceData,
                                                const ezRenderDataBatch& batch, ezUInt32 uiStartIndex, ezUInt32& out_uiFilteredCount)
{
  ezUInt32 uiCount = ezMath::Min<ezUInt32>(instanceData.GetCount(), batch.GetCount() - uiStartIndex);
  ezUInt32 uiCurrentIndex = 0;

  for (auto it = batch.GetIterator<ezKrautBranchRenderData>(uiStartIndex, uiCount); it.IsValid(); ++it)
  {
    const ezKrautBranchRenderData* pRenderData = it;

    const float fDistanceSQR = (pRenderData->m_GlobalTransform.m_vPosition - vLodCamPos).GetLengthSquared();

    if (fDistanceSQR < pRenderData->m_fLodDistanceMinSQR || fDistanceSQR > pRenderData->m_fLodDistanceMaxSQR)
      continue;

    const ezMat4 objectToWorld = pRenderData->m_GlobalTransform.GetAsMat4();

    auto& perInstanceData = instanceData[uiCurrentIndex];
    perInstanceData.ObjectToWorld = objectToWorld;

    // always assumes uniform-scale only
    perInstanceData.ObjectToWorldNormal = objectToWorld;
    perInstanceData.GameObjectID = pRenderData->m_uiUniqueID;
    perInstanceData.Color = ezColor::White; // pRenderData->m_Color;

    ++uiCurrentIndex;
  }

  out_uiFilteredCount = uiCurrentIndex;
}
