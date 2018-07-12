#include <Foundation/Types/ScopeExit.h>
#include <PCH.h>
#include <ParticlePlugin/Type/Point/PointRenderer.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/Device.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/ParticleSystemConstants.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticlePointRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticlePointRenderer, 1, ezRTTIDefaultAllocator<ezParticlePointRenderer>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticlePointRenderer::~ezParticlePointRenderer()
{
  DestroyParticleDataBuffer(m_hBaseDataBuffer);
  DestroyParticleDataBuffer(m_hBillboardDataBuffer);
}

void ezParticlePointRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types)
{
  types.PushBack(ezGetStaticRTTI<ezParticlePointRenderData>());
}

void ezParticlePointRenderer::CreateDataBuffer()
{
  CreateParticleDataBuffer(m_hBaseDataBuffer, sizeof(ezBaseParticleShaderData), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_hBillboardDataBuffer, sizeof(ezBillboardQuadParticleShaderData), s_uiParticlesPerBatch);
}

void ezParticlePointRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass,
                                          const ezRenderDataBatch& batch)
{
  ezRenderContext* pRenderContext = renderViewContext.m_pRenderContext;
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALContext* pGALContext = pRenderContext->GetGALContext();

  TempSystemCB systemConstants(pRenderContext);

  BindParticleShader(pRenderContext, "Shaders/Particles/Point.ezShader");

  // make sure our structured buffer is allocated and bound
  {
    CreateDataBuffer();
    pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Points,
                                                       s_uiParticlesPerBatch);
    pRenderContext->BindBuffer("particleBaseData", pDevice->GetDefaultResourceView(m_hBaseDataBuffer));
    pRenderContext->BindBuffer("particleBillboardQuadData", pDevice->GetDefaultResourceView(m_hBillboardDataBuffer));
  }

  // now render all particle effects of type Point
  for (auto it = batch.GetIterator<ezParticlePointRenderData>(0, batch.GetCount()); it.IsValid(); ++it)
  {
    const ezParticlePointRenderData* pRenderData = it;

    const ezBaseParticleShaderData* pParticleBaseData = pRenderData->m_BaseParticleData.GetPtr();
    const ezBillboardQuadParticleShaderData* pParticleBillboardData = pRenderData->m_BillboardParticleData.GetPtr();

    ezUInt32 uiNumParticles = pRenderData->m_BaseParticleData.GetCount();

    systemConstants.SetGenericData(pRenderData->m_bApplyObjectTransform, pRenderData->m_GlobalTransform, 1, 1, 1, 1);

    while (uiNumParticles > 0)
    {
      // upload this batch of particle data
      const ezUInt32 uiNumParticlesInBatch = ezMath::Min<ezUInt32>(uiNumParticles, s_uiParticlesPerBatch);
      uiNumParticles -= uiNumParticlesInBatch;

      pGALContext->UpdateBuffer(m_hBaseDataBuffer, 0, ezMakeArrayPtr(pParticleBaseData, uiNumParticlesInBatch).ToByteArray());
      pParticleBaseData += uiNumParticlesInBatch;

      pGALContext->UpdateBuffer(m_hBillboardDataBuffer, 0, ezMakeArrayPtr(pParticleBillboardData, uiNumParticlesInBatch).ToByteArray());
      pParticleBillboardData += uiNumParticlesInBatch;

      // do one drawcall
      pRenderContext->DrawMeshBuffer(uiNumParticlesInBatch);
    }
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Point_PointRenderer);
