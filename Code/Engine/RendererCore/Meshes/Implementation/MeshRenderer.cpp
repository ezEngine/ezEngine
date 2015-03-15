#include <RendererCore/PCH.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/RendererCore.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <Core/ResourceManager/ResourceManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshRenderer, ezRenderer, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

static ezGALBufferHandle s_hPerObjectBuffer;
static ezGALRasterizerStateHandle s_hRasterizerState;

struct PerObjectCB
{
  ezMat4 world;
  ezMat4 mvp;
};

void ezMeshRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types)
{
  types.PushBack(ezGetStaticRTTI<ezMeshRenderData>());
}

ezUInt32 ezMeshRenderer::Render(const ezRenderContext& renderContext, ezRenderPipelinePass* pPass, const ezArrayPtr<const ezRenderData*>& renderData)
{
  ezGALContext* pContext = renderContext.m_pGALContext;

  if (s_hPerObjectBuffer.IsInvalidated())
  {
    s_hPerObjectBuffer = ezGALDevice::GetDefaultDevice()->CreateConstantBuffer(sizeof(PerObjectCB));
    pContext->SetConstantBuffer(1, s_hPerObjectBuffer);
  }

  if (s_hRasterizerState.IsInvalidated())
  {
    ezGALRasterizerStateCreationDescription RasterStateDesc;
    RasterStateDesc.m_CullMode = ezGALCullMode::Back;
    RasterStateDesc.m_bFrontCounterClockwise = true;
    s_hRasterizerState = ezGALDevice::GetDefaultDevice()->CreateRasterizerState(RasterStateDesc);
    pContext->SetRasterizerState(s_hRasterizerState);
  }

  const ezMat4& ViewProj = renderContext.m_pView->GetViewProjectionMatrix();
  ezMaterialResourceHandle hLastMaterial;
  
  ezUInt32 uiDataRendered = 0;
  while (uiDataRendered < renderData.GetCount() && renderData[uiDataRendered]->IsInstanceOf<ezMeshRenderData>())
  { 
    const ezMeshRenderData* pRenderData = static_cast<const ezMeshRenderData*>(renderData[uiDataRendered]);

    PerObjectCB cb;
    cb.world = pRenderData->m_WorldTransform.GetAsMat4();
    cb.mvp = ViewProj * cb.world;
    pContext->UpdateBuffer(s_hPerObjectBuffer, 0, &cb, sizeof(PerObjectCB));

    ezResourceLock<ezMeshResource> pMesh(pRenderData->m_hMesh);
    const ezMeshResourceDescriptor::SubMesh& meshPart = pMesh->GetSubMeshes()[pRenderData->m_uiPartIndex];

    if (pRenderData->m_hMaterial != hLastMaterial)
    {
      ezRendererCore::SetMaterialState(pContext, pRenderData->m_hMaterial);
      hLastMaterial = pRenderData->m_hMaterial;
    }

    ezRendererCore::DrawMeshBuffer(pContext, pMesh->GetMeshBuffer(), meshPart.m_uiPrimitiveCount, meshPart.m_uiFirstPrimitive);
    
    ++uiDataRendered;
  }

  return uiDataRendered;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshRenderer);

