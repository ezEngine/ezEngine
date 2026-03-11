#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Renderer/ParticleRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/BufferPool.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleRenderer, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleRenderer::TempSystemCB::TempSystemCB(ezRenderContext* pRenderContext)
{
  // TODO This pattern looks like it is inefficient. Should it use the GPU pool instead somehow?
  m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage(m_pConstants);

  ezBindGroupBuilder& bindGroupMaterial = ezRenderContext::GetDefaultInstance()->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
  bindGroupMaterial.BindBuffer("ezParticleSystemConstants", m_hConstantBuffer);
}

ezParticleRenderer::TempSystemCB::~TempSystemCB()
{
  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void ezParticleRenderer::TempSystemCB::SetGenericData(const ezTransform& objectTransform, ezTime effectLifeTime, ezUInt8 uiNumVariationsX, ezUInt8 uiNumVariationsY, ezUInt8 uiNumFlipbookAnimsX, ezUInt8 uiNumFlipbookAnimsY, float fNormalCurvature, float fLightDirectionality)
{
  ezParticleSystemConstants& cb = m_pConstants->GetDataForWriting();
  cb.ObjectToWorldMatrix = objectTransform.GetAsMat4();
  cb.TextureAtlasVariationFramesX = uiNumVariationsX;
  cb.TextureAtlasVariationFramesY = uiNumVariationsY;
  cb.TextureAtlasFlipbookFramesX = uiNumFlipbookAnimsX;
  cb.TextureAtlasFlipbookFramesY = uiNumFlipbookAnimsY;
  cb.TotalEffectLifeTime = effectLifeTime.AsFloatInSeconds();
  cb.NormalCurvature = fNormalCurvature;
  cb.LightDirectionality = fLightDirectionality;
  cb.ParticlePadding.SetZero();
}


void ezParticleRenderer::TempSystemCB::SetTrailData(float fSnapshotFraction, ezInt32 iNumUsedTrailPoints)
{
  ezParticleSystemConstants& cb = m_pConstants->GetDataForWriting();
  cb.SnapshotFraction = fSnapshotFraction;
  cb.NumUsedTrailPoints = iNumUsedTrailPoints;
}

ezParticleRenderer::ezParticleRenderer() = default;
ezParticleRenderer::~ezParticleRenderer() = default;

void ezParticleRenderer::CreateParticleDataBuffer(ezGALBufferPool& inout_Buffer, ezUInt32 uiDataTypeSize, ezUInt32 uiNumParticlesPerBatch)
{
  if (!inout_Buffer.IsInitialized())
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = uiDataTypeSize;
    desc.m_uiTotalSize = uiNumParticlesPerBatch * desc.m_uiStructSize;
    desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource | ezGALBufferUsageFlags::Transient;
    desc.m_ResourceAccess.m_bImmutable = false;

    inout_Buffer.Initialize(desc, "ParticleRenderer - StructuredBuffer");
  }
}


void ezParticleRenderer::DestroyParticleDataBuffer(ezGALBufferPool& inout_Buffer)
{
  if (inout_Buffer.IsInitialized())
  {
    inout_Buffer.Deinitialize();
  }
}

void ezParticleRenderer::BindParticleShader(ezRenderContext* pRenderContext, const char* szShader) const
{
  if (!m_hShader.IsValid())
  {
    // m_hShader = ezResourceManager::LoadResource<ezShaderResource>(szShader);
  }

  pRenderContext->BindShader(m_hShader);
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Renderer_ParticleRenderer);
