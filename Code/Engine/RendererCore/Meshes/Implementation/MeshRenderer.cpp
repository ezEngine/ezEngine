#include <PCH.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshRenderer, 1, ezRTTIDefaultAllocator<ezMeshRenderer>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezMeshRenderer::ezMeshRenderer()
{
  m_iInstancingThreshold = 4;
}

ezMeshRenderer::~ezMeshRenderer()
{
}

void ezMeshRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types)
{
  types.PushBack(ezGetStaticRTTI<ezMeshRenderData>());
}

void ezMeshRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezRenderContext* pContext = renderViewContext.m_pRenderContext;
  ezGALContext* pGALContext = pContext->GetGALContext();

  const ezMeshRenderData* pRenderData = batch.GetFirstData<ezMeshRenderData>();

  const ezMeshResourceHandle& hMesh = pRenderData->m_hMesh;
  const ezMaterialResourceHandle& hMaterial = pRenderData->m_hMaterial;
  const ezUInt32 uiPartIndex = pRenderData->m_uiPartIndex;

  ezResourceLock<ezMeshResource> pMesh(hMesh);

  // This can happen when the resource has been reloaded and now has fewer submeshes.
  const auto& subMeshes = pMesh->GetSubMeshes();
  if (subMeshes.GetCount() <= uiPartIndex)
  {
    return;
  }

  ezInstanceData* pInstanceData = pPass->GetPipeline()->GetFrameDataProvider<ezInstanceDataProvider>()->GetData(renderViewContext);
  pInstanceData->BindResources(pContext);

  const ezMat3& rotation = pRenderData->m_GlobalTransform.m_Rotation;
  const bool bFlipWinding = rotation.GetColumn(0).Cross(rotation.GetColumn(1)).Dot(rotation.GetColumn(2)) < 0.0f;

  if (bFlipWinding)
  {
    pContext->SetShaderPermutationVariable("FLIP_WINDING", "TRUE");
  }
  else
  {
    pContext->SetShaderPermutationVariable("FLIP_WINDING", "FALSE");
  }

  pContext->BindMaterial(hMaterial);
  pContext->BindMeshBuffer(pMesh->GetMeshBuffer());

  ezUInt32 uiStartIndex = 0;
  while (uiStartIndex < batch.GetCount())
  {
    const ezUInt32 uiCount = batch.GetCount() - uiStartIndex;

    ezUInt32 uiInstanceDataOffset = 0;
    ezArrayPtr<ezPerInstanceData> instanceData = pInstanceData->GetInstanceData(uiCount, uiInstanceDataOffset);

    ezUInt32 uiFilteredCount = 0;
    FillPerInstanceData(instanceData, batch, uiStartIndex, uiFilteredCount);

    if (uiFilteredCount > 0) // Instance data might be empty if all render data was filtered.
    {
      pInstanceData->UpdateInstanceData(pContext, uiFilteredCount);

      const ezMeshResourceDescriptor::SubMesh& meshPart = subMeshes[uiPartIndex];
      if (pContext->DrawMeshBuffer(meshPart.m_uiPrimitiveCount, meshPart.m_uiFirstPrimitive, uiFilteredCount).Failed())
      {
        for (auto it = batch.GetIterator<ezMeshRenderData>(uiStartIndex, uiCount); it.IsValid(); ++it)
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

    uiStartIndex += uiCount;
  }
}

void ezMeshRenderer::FillPerInstanceData(ezArrayPtr<ezPerInstanceData> instanceData, const ezRenderDataBatch& batch, ezUInt32 uiStartIndex, ezUInt32& out_uiFilteredCount)
{
  ezUInt32 uiCount = ezMath::Min<ezUInt32>(instanceData.GetCount(), batch.GetCount() - uiStartIndex);
  ezUInt32 uiCurrentIndex = 0;

  for (auto it = batch.GetIterator<ezMeshRenderData>(uiStartIndex, uiCount); it.IsValid(); ++it)
  {
    const ezMeshRenderData* pRenderData = it;

    auto& perInstanceData = instanceData[uiStartIndex + uiCurrentIndex];
    perInstanceData.ObjectToWorld = pRenderData->m_GlobalTransform;

    const ezVec3 scalingFactors = pRenderData->m_GlobalTransform.m_Rotation.GetScalingFactors();
    const float fEpsilon = ezMath::BasicType<float>::DefaultEpsilon();
    const bool bHasUniformScale = ezMath::IsEqual(scalingFactors.x, scalingFactors.y, fEpsilon) && ezMath::IsEqual(scalingFactors.x, scalingFactors.z, fEpsilon);
    if (bHasUniformScale)
    {
      perInstanceData.ObjectToWorldNormal = pRenderData->m_GlobalTransform;
    }
    else
    {
      ezMat3 rotation = pRenderData->m_GlobalTransform.m_Rotation.GetInverse().GetTranspose();
      perInstanceData.ObjectToWorldNormal = ezTransform(ezVec3::ZeroVector(), rotation);
    }

    perInstanceData.GameObjectID = pRenderData->m_uiUniqueID;

    ++uiCurrentIndex;
  }

  out_uiFilteredCount = uiCurrentIndex;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshRenderer);

