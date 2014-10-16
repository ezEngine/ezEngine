#include <RendererCore/PCH.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <CoreUtils/Graphics/Camera.h>

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

ezUInt32 ezMeshRenderer::Render(ezRenderPipelinePass* pPass, const ezArrayPtr<const ezRenderData*>& renderData)
{
  ezGALDevice* pDevice = pPass->GetPipeline()->GetDevice();
  ezGALContext* pContext = pDevice->GetPrimaryContext();

  if (s_hPerObjectBuffer.IsInvalidated())
  {
    s_hPerObjectBuffer = pDevice->CreateConstantBuffer(sizeof(PerObjectCB));
    pContext->SetConstantBuffer(1, s_hPerObjectBuffer);
  }

  if (s_hRasterizerState.IsInvalidated())
  {
    ezGALRasterizerStateCreationDescription RasterStateDesc;
    RasterStateDesc.m_CullMode = ezGALCullMode::Back;
    RasterStateDesc.m_bFrontCounterClockwise = true;
    s_hRasterizerState = pDevice->CreateRasterizerState(RasterStateDesc);
    pContext->SetRasterizerState(s_hRasterizerState);
  }

  const ezCamera& camera = pPass->GetCurrentCamera();
  ezMat4 View; 
  View.SetLookAtMatrix(camera.GetPosition(), camera.GetPosition() + camera.GetDirForwards(), camera.GetDirUp());

  ezMat4 Proj;
  Proj.SetOrthographicProjectionMatrix(45.0f, 45.0f, camera.GetNearPlane(), camera.GetFarPlane(), ezProjectionDepthRange::ZeroToOne);

  ezUInt32 uiDataRendered = 0;

  while (uiDataRendered < renderData.GetCount() && renderData[uiDataRendered]->IsInstanceOf<ezMeshRenderData>())
  { 
    const ezMeshRenderData* pRenderData = static_cast<const ezMeshRenderData*>(renderData[uiDataRendered]);

    PerObjectCB cb;
    cb.world = pRenderData->m_WorldTransform.GetAsMat4();
    cb.mvp = Proj * View * cb.world;
    pContext->UpdateBuffer(s_hPerObjectBuffer, 0, &cb, sizeof(PerObjectCB));

    ezResourceLock<ezMeshResource> pMesh(pRenderData->m_hMesh);
    const ezMeshResource::Part& meshPart = pMesh->GetParts()[pRenderData->m_uiPartIndex];

    ezResourceLock<ezMeshBufferResource> pMeshBuffer(meshPart.m_hMeshBuffer);
    pMeshBuffer->Draw(meshPart.m_uiNumPrimitives, meshPart.m_uiStartPrimitive);

    ++uiDataRendered;
  }

  return uiDataRendered;
}