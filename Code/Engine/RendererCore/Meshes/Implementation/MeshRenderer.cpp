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

  if (pRenderData->m_uiFlipWinding)
  {
    pContext->SetShaderPermutationVariable("FLIP_WINDING", "TRUE");
  }
  else
  {
    pContext->SetShaderPermutationVariable("FLIP_WINDING", "FALSE");
  }

  // Bind skinning matrices if supplied and set the appropriate permutation variable
  if (pRenderData->m_hSkinningMatrices.IsInvalidated())
  {
    pContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");
  }
  else
  {
    pContext->SetShaderPermutationVariable("VERTEX_SKINNING", "TRUE");

    if (!pRenderData->m_pNewSkinningMatricesData.IsEmpty())
    {
      pContext->GetGALContext()->UpdateBuffer(pRenderData->m_hSkinningMatrices, 0, pRenderData->m_pNewSkinningMatricesData);
    }

    pContext->BindBuffer("skinningMatrices", pDevice->GetDefaultResourceView(pRenderData->m_hSkinningMatrices));
  }

  pContext->BindMaterial(hMaterial);
  pContext->BindMeshBuffer(pMesh->GetMeshBuffer());

  ezUInt32 uiStartIndex = 0;
  while (uiStartIndex < batch.GetCount())
  {
    const ezUInt32 uiRemainingInstances = batch.GetCount() - uiStartIndex;

    ezUInt32 uiInstanceDataOffset = 0;
    ezArrayPtr<ezPerInstanceData> instanceData = pInstanceData->GetInstanceData(uiRemainingInstances, uiInstanceDataOffset);

    ezUInt32 uiFilteredCount = 0;
    FillPerInstanceData(instanceData, batch, uiStartIndex, uiFilteredCount);

    if (uiFilteredCount > 0) // Instance data might be empty if all render data was filtered.
    {
      pInstanceData->UpdateInstanceData(pContext, uiFilteredCount);

      const ezMeshResourceDescriptor::SubMesh& meshPart = subMeshes[uiPartIndex];

      unsigned int uiRenderedInstances = uiFilteredCount;
      if (renderViewContext.m_pCamera->IsStereoscopic())
        uiRenderedInstances *= 2;

      if (pContext->DrawMeshBuffer(meshPart.m_uiPrimitiveCount, meshPart.m_uiFirstPrimitive, uiRenderedInstances).Failed())
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

void ezMeshRenderer::FillPerInstanceData(ezArrayPtr<ezPerInstanceData> instanceData, const ezRenderDataBatch& batch, ezUInt32 uiStartIndex, ezUInt32& out_uiFilteredCount)
{
  ezUInt32 uiCount = ezMath::Min<ezUInt32>(instanceData.GetCount(), batch.GetCount() - uiStartIndex);
  ezUInt32 uiCurrentIndex = 0;

  for (auto it = batch.GetIterator<ezMeshRenderData>(uiStartIndex, uiCount); it.IsValid(); ++it)
  {
    const ezMeshRenderData* pRenderData = it;

    ezMat4 objectToWorld = pRenderData->m_GlobalTransform.GetAsMat4();

    auto& perInstanceData = instanceData[uiCurrentIndex];
    perInstanceData.ObjectToWorld = objectToWorld;

    if (pRenderData->m_uiUniformScale)
    {
      perInstanceData.ObjectToWorldNormal = objectToWorld;
    }
    else
    {
      ezShaderTransform shaderT;
      shaderT = objectToWorld.GetRotationalPart().GetInverse(0.0f).GetTranspose();
      perInstanceData.ObjectToWorldNormal = shaderT;
    }

    perInstanceData.GameObjectID = pRenderData->m_uiUniqueID;
    perInstanceData.Color = pRenderData->m_Color;

    ++uiCurrentIndex;
  }

  out_uiFilteredCount = uiCurrentIndex;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshRenderer);

