#include <PCH.h>

#include <Foundation/Types/ScopeExit.h>
#include <ParticlePlugin/Type/Trail/ParticleTypeTrail.h>
#include <ParticlePlugin/Type/Trail/TrailRenderer.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/Device.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/ParticleSystemConstants.h>

//clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTrailRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTrailRenderer, 1, ezRTTIDefaultAllocator<ezParticleTrailRenderer>);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleTrailRenderer::~ezParticleTrailRenderer()
{
  DestroyParticleDataBuffer(m_hBaseDataBuffer);
  DestroyParticleDataBuffer(m_hTrailDataBuffer);
  DestroyParticleDataBuffer(m_hTrailPointsDataBuffer8);
  DestroyParticleDataBuffer(m_hTrailPointsDataBuffer16);
  DestroyParticleDataBuffer(m_hTrailPointsDataBuffer32);
  DestroyParticleDataBuffer(m_hTrailPointsDataBuffer64);
}

void ezParticleTrailRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types)
{
  types.PushBack(ezGetStaticRTTI<ezParticleTrailRenderData>());
}

void ezParticleTrailRenderer::CreateDataBuffer()
{
  CreateParticleDataBuffer(m_hBaseDataBuffer, sizeof(ezBaseParticleShaderData), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_hTrailDataBuffer, sizeof(ezTrailParticleShaderData), s_uiParticlesPerBatch);

  // this is kinda stupid, apparently due to stride enforcement I cannot reuse the same buffer for different sizes
  // and instead have to create one buffer with every size ...

  CreateParticleDataBuffer(m_hTrailPointsDataBuffer8, sizeof(ezTrailParticlePointsData8), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_hTrailPointsDataBuffer16, sizeof(ezTrailParticlePointsData16), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_hTrailPointsDataBuffer32, sizeof(ezTrailParticlePointsData32), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_hTrailPointsDataBuffer64, sizeof(ezTrailParticlePointsData64), s_uiParticlesPerBatch);
}

void ezParticleTrailRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass,
                                          const ezRenderDataBatch& batch)
{
  ezRenderContext* pRenderContext = renderViewContext.m_pRenderContext;
  ezGALContext* pGALContext = pRenderContext->GetGALContext();

  TempSystemCB systemConstants(pRenderContext);

  BindParticleShader(pRenderContext, "Shaders/Particles/Trail.ezShader");

  // make sure our structured buffer is allocated and bound
  {
    CreateDataBuffer();
    pRenderContext->BindBuffer("particleBaseData", ezGALDevice::GetDefaultDevice()->GetDefaultResourceView(m_hBaseDataBuffer));
    pRenderContext->BindBuffer("particleTrailData", ezGALDevice::GetDefaultDevice()->GetDefaultResourceView(m_hTrailDataBuffer));
  }

  // now render all particle effects of type Trail
  for (auto it = batch.GetIterator<ezParticleTrailRenderData>(0, batch.GetCount()); it.IsValid(); ++it)
  {
    const ezParticleTrailRenderData* pRenderData = it;

    if (!ConfigureShader(pRenderData, renderViewContext))
      continue;

    const ezUInt32 uiBucketSize = ezParticleTypeTrail::ComputeTrailPointBucketSize(pRenderData->m_uiMaxTrailPoints);
    const ezUInt32 uiMaxTrailSegments = uiBucketSize - 1;
    const ezUInt32 uiPrimFactor = 2;
    const ezUInt32 uiMaxPrimitivesToRender = s_uiParticlesPerBatch * uiMaxTrailSegments * uiPrimFactor;


    pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles,
                                   uiMaxPrimitivesToRender);

    const ezBaseParticleShaderData* pParticleBaseData = pRenderData->m_BaseParticleData.GetPtr();
    const ezTrailParticleShaderData* pParticleTrailData = pRenderData->m_TrailParticleData.GetPtr();


    const ezVec4* pParticlePointsData = pRenderData->m_TrailPointsShared.GetPtr();

    pRenderContext->BindTexture2D("ParticleTexture", pRenderData->m_hTexture);

    systemConstants.SetGenericData(pRenderData->m_bApplyObjectTransform, pRenderData->m_GlobalTransform, pRenderData->m_uiNumVariationsX,
                                   pRenderData->m_uiNumVariationsY, pRenderData->m_uiNumFlipbookAnimationsX,
                                   pRenderData->m_uiNumFlipbookAnimationsY, 0.0f);
    systemConstants.SetTrailData(pRenderData->m_fSnapshotFraction, pRenderData->m_uiMaxTrailPoints);

    ezUInt32 uiNumParticles = pRenderData->m_BaseParticleData.GetCount();
    while (uiNumParticles > 0)
    {
      // upload this batch of particle data
      const ezUInt32 uiNumParticlesInBatch = ezMath::Min<ezUInt32>(uiNumParticles, s_uiParticlesPerBatch);
      uiNumParticles -= uiNumParticlesInBatch;

      pGALContext->UpdateBuffer(m_hBaseDataBuffer, 0, ezMakeArrayPtr(pParticleBaseData, uiNumParticlesInBatch).ToByteArray());
      pParticleBaseData += uiNumParticlesInBatch;

      pGALContext->UpdateBuffer(m_hTrailDataBuffer, 0, ezMakeArrayPtr(pParticleTrailData, uiNumParticlesInBatch).ToByteArray());
      pParticleTrailData += uiNumParticlesInBatch;

      pGALContext->UpdateBuffer(m_hActiveTrailPointsDataBuffer, 0,
                                ezMakeArrayPtr(pParticlePointsData, uiNumParticlesInBatch * uiBucketSize).ToByteArray());
      pParticlePointsData += uiNumParticlesInBatch;

      // do one drawcall
      pRenderContext->DrawMeshBuffer(uiNumParticlesInBatch * uiMaxTrailSegments * uiPrimFactor);
    }
  }
}

bool ezParticleTrailRenderer::ConfigureShader(const ezParticleTrailRenderData* pRenderData, const ezRenderViewContext& renderViewContext)
{
  switch (pRenderData->m_RenderMode)
  {
    case ezParticleTypeRenderMode::Additive:
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_ADDITIVE");
      break;
    case ezParticleTypeRenderMode::Blended:
    case ezParticleTypeRenderMode::BlendedForeground:
    case ezParticleTypeRenderMode::BlendedBackground:
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_BLENDED");
      break;
    case ezParticleTypeRenderMode::Opaque:
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_OPAQUE");
      break;
  }

  switch (ezParticleTypeTrail::ComputeTrailPointBucketSize(pRenderData->m_uiMaxTrailPoints))
  {
    case 8:
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_TRAIL_POINTS", "PARTICLE_TRAIL_POINTS_COUNT8");
      m_hActiveTrailPointsDataBuffer = m_hTrailPointsDataBuffer8;
      break;
    case 16:
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_TRAIL_POINTS", "PARTICLE_TRAIL_POINTS_COUNT16");
      m_hActiveTrailPointsDataBuffer = m_hTrailPointsDataBuffer16;
      break;
    case 32:
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_TRAIL_POINTS", "PARTICLE_TRAIL_POINTS_COUNT32");
      m_hActiveTrailPointsDataBuffer = m_hTrailPointsDataBuffer32;
      break;
    case 64:
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_TRAIL_POINTS", "PARTICLE_TRAIL_POINTS_COUNT64");
      m_hActiveTrailPointsDataBuffer = m_hTrailPointsDataBuffer64;
      break;

    default:
      return false;
  }

  renderViewContext.m_pRenderContext->BindBuffer("particlePointsData",
                                                 ezGALDevice::GetDefaultDevice()->GetDefaultResourceView(m_hActiveTrailPointsDataBuffer));
  return true;
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Trail_TrailRenderer);
