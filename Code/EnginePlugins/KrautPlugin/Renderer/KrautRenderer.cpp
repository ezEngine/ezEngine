#include <KrautPluginPCH.h>

#include <KrautPlugin/Components/KrautTreeComponent.h>
#include <KrautPlugin/Renderer/KrautRenderer.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautRenderData, 1, ezRTTIDefaultAllocator<ezKrautRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautRenderer, 1, ezRTTIDefaultAllocator<ezKrautRenderer>);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezKrautRenderer::ezKrautRenderer() = default;
ezKrautRenderer::~ezKrautRenderer() = default;

void ezKrautRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) const
{
  types.PushBack(ezGetStaticRTTI<ezKrautRenderData>());
}

void ezKrautRenderer::GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& categories) const
{
  categories.PushBack(ezDefaultRenderDataCategories::LitOpaque);
  categories.PushBack(ezDefaultRenderDataCategories::Selection);
}

void ezKrautRenderer::RenderBatch(
  const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  ezRenderContext* pRenderContext = renderViewContext.m_pRenderContext;

  const ezKrautRenderData* pRenderData = batch.GetFirstData<ezKrautRenderData>();

  ezResourceLock<ezMeshResource> pMesh(pRenderData->m_hMesh, ezResourceAcquireMode::AllowFallback);

  // This can happen when the resource has been reloaded and now has fewer sub-meshes.
  if (pMesh->GetSubMeshes().GetCount() <= pRenderData->m_uiSubMeshIndex)
    return;

  TempTreeCB treeConstants(pRenderContext);

  const auto& subMesh = pMesh->GetSubMeshes()[pRenderData->m_uiSubMeshIndex];

  ezInstanceData* pInstanceData = pPass->GetPipeline()->GetFrameDataProvider<ezInstanceDataProvider>()->GetData(renderViewContext);
  pInstanceData->BindResources(pRenderContext);

  // inverted trees are not allowed
  pRenderContext->SetShaderPermutationVariable("FLIP_WINDING", "FALSE");
  // no skinning atm
  pRenderContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");

  pRenderContext->BindMaterial(pMesh->GetMaterials()[subMesh.m_uiMaterialIndex]);
  pRenderContext->BindMeshBuffer(pMesh->GetMeshBuffer());

  treeConstants.SetTreeData(
    pRenderData->m_vLeafCenter, renderViewContext.m_pViewData->m_CameraUsageHint == ezCameraUsageHint::Shadow ? 1.0f : 0.0f);

  const ezVec3 vLodCamPos = renderViewContext.m_pCamera->GetPosition();

  const bool bIsShadowView = renderViewContext.m_pViewData->m_CameraUsageHint == ezCameraUsageHint::Shadow;
  const bool bUpdateMinLod = (renderViewContext.m_pViewData->m_CameraUsageHint == ezCameraUsageHint::Culling ||
                              renderViewContext.m_pViewData->m_CameraUsageHint == ezCameraUsageHint::Thumbnail ||
                              renderViewContext.m_pViewData->m_CameraUsageHint == ezCameraUsageHint::MainView ||
                              renderViewContext.m_pViewData->m_CameraUsageHint == ezCameraUsageHint::EditorView ||
                              renderViewContext.m_pViewData->m_CameraUsageHint == ezCameraUsageHint::RenderTarget);

  for (ezUInt32 uiStartIndex = 0; uiStartIndex < batch.GetCount(); /**/)
  {
    const ezUInt32 uiRemainingInstances = batch.GetCount() - uiStartIndex;

    ezUInt32 uiInstanceDataOffset = 0;
    ezArrayPtr<ezPerInstanceData> instanceData = pInstanceData->GetInstanceData(uiRemainingInstances, uiInstanceDataOffset);

    ezUInt32 uiFilteredCount = 0;
    FillPerInstanceData(vLodCamPos, instanceData, batch, bUpdateMinLod, bIsShadowView, uiStartIndex, uiFilteredCount);

    if (uiFilteredCount > 0) // Instance data might be empty if all render data was filtered.
    {
      pInstanceData->UpdateInstanceData(pRenderContext, uiFilteredCount);

      ezUInt32 uiRenderedInstances = uiFilteredCount;

      if (renderViewContext.m_pCamera->IsStereoscopic())
        uiRenderedInstances *= 2;

      if (pRenderContext->DrawMeshBuffer(subMesh.m_uiPrimitiveCount, subMesh.m_uiFirstPrimitive, uiRenderedInstances).Failed())
      {
        for (auto it = batch.GetIterator<ezKrautRenderData>(uiStartIndex, instanceData.GetCount()); it.IsValid(); ++it)
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

void ezKrautRenderer::FillPerInstanceData(const ezVec3& vLodCamPos, ezArrayPtr<ezPerInstanceData> instanceData,
  const ezRenderDataBatch& batch, bool bUpdateMinLod, bool bIsShadowView, ezUInt32 uiStartIndex, ezUInt32& out_uiFilteredCount) const
{
  const ezUInt64 uiFrameCount = ezRenderWorld::GetFrameCounter();
  const ezUInt8 uiReadLod = uiFrameCount % 4;
  const ezUInt8 uiWriteLod = (uiFrameCount + 1) % 4;
  const ezUInt8 uiClearLod = (uiFrameCount + 2) % 4;

  ezUInt32 uiCount = ezMath::Min<ezUInt32>(instanceData.GetCount(), batch.GetCount() - uiStartIndex);
  ezUInt32 uiCurrentIndex = 0;

  for (auto it = batch.GetIterator<ezKrautRenderData>(uiStartIndex, uiCount); it.IsValid(); ++it)
  {
    const ezKrautRenderData* pRenderData = it;

    const float fDistanceSQR = (pRenderData->m_GlobalTransform.m_vPosition - vLodCamPos).GetLengthSquared();

    pRenderData->m_pTreeLodInfo->m_uiMinLod[uiClearLod] = 5;

    if (bIsShadowView && !pRenderData->m_bCastShadows)
      continue;

    if (bUpdateMinLod)
    {
      if (fDistanceSQR >= pRenderData->m_fLodDistanceMinSQR && fDistanceSQR < pRenderData->m_fLodDistanceMaxSQR)
      {
        pRenderData->m_pTreeLodInfo->m_uiMinLod[uiWriteLod] =
          ezMath::Min<ezUInt32>(pRenderData->m_pTreeLodInfo->m_uiMinLod[uiWriteLod], pRenderData->m_uiThisLodIndex);
      }
    }

    if (pRenderData->m_pTreeLodInfo->m_uiMinLod[uiReadLod] != pRenderData->m_uiThisLodIndex)
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

ezKrautRenderer::TempTreeCB::TempTreeCB(ezRenderContext* pRenderContext)
{
  // TODO This pattern looks like it is inefficient. Should it use the GPU pool instead somehow?
  m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage(m_pConstants);

  pRenderContext->BindConstantBuffer("ezKrautTreeConstants", m_hConstantBuffer);
}

ezKrautRenderer::TempTreeCB::~TempTreeCB()
{
  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void ezKrautRenderer::TempTreeCB::SetTreeData(const ezVec3& vTreeCenter, float fLeafShadowOffset)
{
  ezKrautTreeConstants& cb = m_pConstants->GetDataForWriting();
  cb.LeafCenter = vTreeCenter;
  cb.LeafShadowOffset = fLeafShadowOffset;
}
