#include <ParticlePluginPCH.h>

#include <Foundation/Types/ScopeExit.h>
#include <ParticlePlugin/Type/Point/PointRenderer.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Device/Device.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/ParticleSystemConstants.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticlePointRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticlePointRenderer, 1, ezRTTIDefaultAllocator<ezParticlePointRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticlePointRenderer::ezParticlePointRenderer()
{
  CreateParticleDataBuffer(m_hBaseDataBuffer, sizeof(ezBaseParticleShaderData), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_hBillboardDataBuffer, sizeof(ezBillboardQuadParticleShaderData), s_uiParticlesPerBatch);

  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Particles/Point.ezShader");
}


ezParticlePointRenderer::~ezParticlePointRenderer()
{
  DestroyParticleDataBuffer(m_hBaseDataBuffer);
  DestroyParticleDataBuffer(m_hBillboardDataBuffer);
}

void ezParticlePointRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) const
{
  types.PushBack(ezGetStaticRTTI<ezParticlePointRenderData>());
}

void ezParticlePointRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  ezRenderContext* pRenderContext = renderViewContext.m_pRenderContext;
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  TempSystemCB systemConstants(pRenderContext);

  pRenderContext->BindShader(m_hShader);

  // make sure our structured buffer is allocated and bound
  {
    pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Points, s_uiParticlesPerBatch);
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

    systemConstants.SetGenericData(pRenderData->m_bApplyObjectTransform, pRenderData->m_GlobalTransform, pRenderData->m_TotalEffectLifeTime, 1, 1, 1, 1);

    while (uiNumParticles > 0)
    {
      // upload this batch of particle data
      const ezUInt32 uiNumParticlesInBatch = ezMath::Min<ezUInt32>(uiNumParticles, s_uiParticlesPerBatch);
      uiNumParticles -= uiNumParticlesInBatch;

      pGALCommandEncoder->UpdateBuffer(m_hBaseDataBuffer, 0, ezMakeArrayPtr(pParticleBaseData, uiNumParticlesInBatch).ToByteArray());
      pParticleBaseData += uiNumParticlesInBatch;

      pGALCommandEncoder->UpdateBuffer(m_hBillboardDataBuffer, 0, ezMakeArrayPtr(pParticleBillboardData, uiNumParticlesInBatch).ToByteArray());
      pParticleBillboardData += uiNumParticlesInBatch;

      // do one drawcall
      pRenderContext->DrawMeshBuffer(uiNumParticlesInBatch).IgnoreResult();
    }
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Point_PointRenderer);
