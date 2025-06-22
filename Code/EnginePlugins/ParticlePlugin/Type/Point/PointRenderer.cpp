#include <ParticlePlugin/ParticlePluginPCH.h>

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
  CreateParticleDataBuffer(m_BaseDataBuffer, sizeof(ezBaseParticleShaderData), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_BillboardDataBuffer, sizeof(ezBillboardQuadParticleShaderData), s_uiParticlesPerBatch);

  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Particles/Point.ezShader");
}


ezParticlePointRenderer::~ezParticlePointRenderer()
{
  DestroyParticleDataBuffer(m_BaseDataBuffer);
  DestroyParticleDataBuffer(m_BillboardDataBuffer);
}

void ezParticlePointRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(ezGetStaticRTTI<ezParticlePointRenderData>());
}

void ezParticlePointRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  ezRenderContext* pRenderContext = renderViewContext.m_pRenderContext;
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  TempSystemCB systemConstants(pRenderContext);

  pRenderContext->BindShader(m_hShader);

  // Bind mesh buffer
  {
    pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Points, s_uiParticlesPerBatch);
  }

  ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
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
      // Request new buffers and bind them
      ezGALBufferHandle hBaseDataBuffer = m_BaseDataBuffer.GetNewBuffer();
      ezGALBufferHandle hBillboardDataBuffer = m_BillboardDataBuffer.GetNewBuffer();
      bindGroup.BindBuffer("particleBaseData", hBaseDataBuffer);
      bindGroup.BindBuffer("particleBillboardQuadData", hBillboardDataBuffer);

      // upload this batch of particle data
      const ezUInt32 uiNumParticlesInBatch = ezMath::Min<ezUInt32>(uiNumParticles, s_uiParticlesPerBatch);
      uiNumParticles -= uiNumParticlesInBatch;

      pGALCommandEncoder->UpdateBuffer(hBaseDataBuffer, 0, ezMakeArrayPtr(pParticleBaseData, uiNumParticlesInBatch).ToByteArray(), ezGALUpdateMode::AheadOfTime);
      pParticleBaseData += uiNumParticlesInBatch;

      pGALCommandEncoder->UpdateBuffer(hBillboardDataBuffer, 0, ezMakeArrayPtr(pParticleBillboardData, uiNumParticlesInBatch).ToByteArray(), ezGALUpdateMode::AheadOfTime);
      pParticleBillboardData += uiNumParticlesInBatch;

      // do one drawcall
      pRenderContext->DrawMeshBuffer(uiNumParticlesInBatch).IgnoreResult();
    }
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Point_PointRenderer);
