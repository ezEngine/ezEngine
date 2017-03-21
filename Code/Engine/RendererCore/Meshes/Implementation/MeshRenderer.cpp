#include <PCH.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <Foundation/Types/ScopeExit.h>

namespace
{
  enum
  {
    MAX_RENDER_DATA_PER_BATCH = 1024
  };
}

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

  const bool bUseInstancing = m_iInstancingThreshold > 0 && batch.GetCount() >= (ezUInt32)m_iInstancingThreshold;
  const ezUInt32 uiMaxRenderDataPerBatch = bUseInstancing ? MAX_RENDER_DATA_PER_BATCH : 1;

  ezGALBufferHandle hPerInstanceData = CreateInstanceDataBuffer(bUseInstancing);
  EZ_SCOPE_EXIT(DeleteInstanceDataBuffer(hPerInstanceData));

  if (bUseInstancing)
  {
    pContext->SetShaderPermutationVariable("INSTANCING", "TRUE");

    for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    {
      pContext->BindBuffer((ezGALShaderStage::Enum)stage, "perInstanceData", pDevice->GetDefaultResourceView(hPerInstanceData));
    }
  }
  else
  {
    pContext->SetShaderPermutationVariable("INSTANCING", "FALSE");
    pContext->BindConstantBuffer("ezPerInstanceConstants", hPerInstanceData);
  }

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
    const ezUInt32 uiCount = ezMath::Min(batch.GetCount() - uiStartIndex, uiMaxRenderDataPerBatch);

    FillPerInstanceData(batch, uiStartIndex, uiCount);
    if (m_perInstanceData.GetCount() > 0) // Instance data might be empty if all render data was filtered.
    {
      pGALContext->UpdateBuffer(hPerInstanceData, 0, m_perInstanceData.GetByteArrayPtr());

      const ezMeshResourceDescriptor::SubMesh& meshPart = subMeshes[uiPartIndex];
      if (pContext->DrawMeshBuffer(meshPart.m_uiPrimitiveCount, meshPart.m_uiFirstPrimitive, batch.GetCount()).Failed())
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

ezGALBufferHandle ezMeshRenderer::CreateInstanceDataBuffer(bool bUseInstancing)
{
  ezGALBufferCreationDescription desc;
  desc.m_ResourceAccess.m_bImmutable = false;

  if (bUseInstancing)
  {
    desc.m_uiStructSize = sizeof(ezPerInstanceData);
    desc.m_uiTotalSize = desc.m_uiStructSize * MAX_RENDER_DATA_PER_BATCH;
    desc.m_BufferType = ezGALBufferType::Generic;
    desc.m_bUseAsStructuredBuffer = true;
    desc.m_bAllowShaderResourceView = true;
  }
  else
  {
    desc.m_uiTotalSize = sizeof(ezPerInstanceData);
    desc.m_BufferType = ezGALBufferType::ConstantBuffer;
  }

  return ezGPUResourcePool::GetDefaultInstance()->GetBuffer(desc);
}


void ezMeshRenderer::DeleteInstanceDataBuffer(ezGALBufferHandle hBuffer)
{
  ezGPUResourcePool::GetDefaultInstance()->ReturnBuffer(hBuffer);
}

void ezMeshRenderer::FillPerInstanceData(const ezRenderDataBatch& batch, ezUInt32 uiStartIndex, ezUInt32 uiCount)
{
  m_perInstanceData.Clear();
  m_perInstanceData.Reserve(uiCount);

  for (auto it = batch.GetIterator<ezMeshRenderData>(uiStartIndex, uiCount); it.IsValid(); ++it)
  {
    const ezMeshRenderData* pRenderData = it;

    auto& perInstanceData = m_perInstanceData.ExpandAndGetRef();
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
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshRenderer);

