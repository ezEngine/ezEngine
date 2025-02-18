#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Types/ScopeExit.h>
#include <ParticlePlugin/Type/Quad/QuadParticleRenderer.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleQuadRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleQuadRenderer, 1, ezRTTIDefaultAllocator<ezParticleQuadRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

bool ezParticleQuadRenderData::CanBatch(const ezRenderData& other0) const
{
  const auto& other = ezStaticCast<const ezParticleQuadRenderData&>(other0);

  return m_RenderMode == other.m_RenderMode && m_hTexture == other.m_hTexture;
}

//////////////////////////////////////////////////////////////////////////

ezParticleQuadRenderer::ezParticleQuadRenderer()
{
  CreateParticleDataBuffer(m_BaseDataBuffer, sizeof(ezBaseParticleShaderData), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_BillboardDataBuffer, sizeof(ezBillboardQuadParticleShaderData), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_TangentDataBuffer, sizeof(ezTangentQuadParticleShaderData), s_uiParticlesPerBatch);

  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Particles/QuadParticle.ezShader");
}

ezParticleQuadRenderer::~ezParticleQuadRenderer()
{
  DestroyParticleDataBuffer(m_BaseDataBuffer);
  DestroyParticleDataBuffer(m_BillboardDataBuffer);
  DestroyParticleDataBuffer(m_TangentDataBuffer);
}

void ezParticleQuadRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(ezGetStaticRTTI<ezParticleQuadRenderData>());
}

void ezParticleQuadRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  ezRenderContext* pRenderContext = renderViewContext.m_pRenderContext;
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  TempSystemCB systemConstants(pRenderContext);

  pRenderContext->BindShader(m_hShader);

  // Bind mesh buffer
  {
    pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, s_uiParticlesPerBatch * 2);
  }

  // now render all particle effects of type Quad
  for (auto it = batch.GetIterator<ezParticleQuadRenderData>(0, batch.GetCount()); it.IsValid(); ++it)
  {
    const ezParticleQuadRenderData* pRenderData = it;

    const ezBaseParticleShaderData* pParticleBaseData = pRenderData->m_BaseParticleData.GetPtr();
    const ezBillboardQuadParticleShaderData* pParticleBillboardData = pRenderData->m_BillboardParticleData.GetPtr();
    const ezTangentQuadParticleShaderData* pParticleTangentData = pRenderData->m_TangentParticleData.GetPtr();

    ezUInt32 uiNumParticles = pRenderData->m_BaseParticleData.GetCount();

    pRenderContext->BindTexture2D("ParticleTexture", pRenderData->m_hTexture);

    ConfigureRenderMode(pRenderData, pRenderContext);

    systemConstants.SetGenericData(
      pRenderData->m_bApplyObjectTransform, pRenderData->m_GlobalTransform, pRenderData->m_TotalEffectLifeTime, pRenderData->m_uiNumVariationsX, pRenderData->m_uiNumVariationsY, pRenderData->m_uiNumFlipbookAnimationsX, pRenderData->m_uiNumFlipbookAnimationsY, pRenderData->m_fDistortionStrength, pRenderData->m_fNormalCurvature, pRenderData->m_fLightDirectionality);

    pRenderContext->SetShaderPermutationVariable("PARTICLE_QUAD_MODE", pRenderData->m_QuadModePermutation);

    while (uiNumParticles > 0)
    {
      // Request new buffers and bind them
      ezGALBufferHandle hBaseDataBuffer = m_BaseDataBuffer.GetNewBuffer();
      ezGALBufferHandle hBillboardDataBuffer = m_BillboardDataBuffer.GetNewBuffer();
      ezGALBufferHandle hTangentDataBuffer = m_TangentDataBuffer.GetNewBuffer();

      pRenderContext->BindBuffer("particleBaseData", pDevice->GetDefaultResourceView(hBaseDataBuffer));
      pRenderContext->BindBuffer("particleBillboardQuadData", pDevice->GetDefaultResourceView(hBillboardDataBuffer));
      pRenderContext->BindBuffer("particleTangentQuadData", pDevice->GetDefaultResourceView(hTangentDataBuffer));

      // upload this batch of particle data
      const ezUInt32 uiNumParticlesInBatch = ezMath::Min<ezUInt32>(uiNumParticles, s_uiParticlesPerBatch);
      uiNumParticles -= uiNumParticlesInBatch;

      pGALCommandEncoder->UpdateBuffer(hBaseDataBuffer, 0, ezMakeArrayPtr(pParticleBaseData, uiNumParticlesInBatch).ToByteArray(), ezGALUpdateMode::AheadOfTime);
      pParticleBaseData += uiNumParticlesInBatch;

      if (pParticleBillboardData != nullptr)
      {
        pGALCommandEncoder->UpdateBuffer(hBillboardDataBuffer, 0, ezMakeArrayPtr(pParticleBillboardData, uiNumParticlesInBatch).ToByteArray(), ezGALUpdateMode::AheadOfTime);
        pParticleBillboardData += uiNumParticlesInBatch;
      }

      if (pParticleTangentData != nullptr)
      {
        pGALCommandEncoder->UpdateBuffer(hTangentDataBuffer, 0, ezMakeArrayPtr(pParticleTangentData, uiNumParticlesInBatch).ToByteArray(), ezGALUpdateMode::AheadOfTime);
        pParticleTangentData += uiNumParticlesInBatch;
      }

      // do one drawcall
      renderViewContext.m_pRenderContext->DrawMeshBuffer(uiNumParticlesInBatch * 2).IgnoreResult();
    }
  }
}

void ezParticleQuadRenderer::ConfigureRenderMode(const ezParticleQuadRenderData* pRenderData, ezRenderContext* pRenderContext) const
{
  switch (pRenderData->m_RenderMode)
  {
    case ezParticleTypeRenderMode::Additive:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_ADDITIVE");
      break;
    case ezParticleTypeRenderMode::Blended:
    case ezParticleTypeRenderMode::BlendedForeground:
    case ezParticleTypeRenderMode::BlendedBackground:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_BLENDED");
      break;
    case ezParticleTypeRenderMode::Opaque:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_OPAQUE");
      break;
    case ezParticleTypeRenderMode::Distortion:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_DISTORTION");
      pRenderContext->BindTexture2D("ParticleDistortionTexture", pRenderData->m_hDistortionTexture);
      break;
    case ezParticleTypeRenderMode::BlendAdd:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_BLENDADD");
      break;
  }

  switch (pRenderData->m_LightingMode)
  {
    case ezParticleLightingMode::Fullbright:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_LIGHTING_MODE", "PARTICLE_LIGHTING_MODE_FULLBRIGHT");
      break;
    case ezParticleLightingMode::VertexLit:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_LIGHTING_MODE", "PARTICLE_LIGHTING_MODE_VERTEX_LIT");
      break;
  }
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Quad_QuadParticleRenderer);
