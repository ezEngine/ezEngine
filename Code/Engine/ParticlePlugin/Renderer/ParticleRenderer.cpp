#include <PCH.h>

#include <ParticlePlugin/Renderer/ParticleRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleRenderer, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleRenderer::TempSystemCB::TempSystemCB(ezRenderContext* pRenderContext)
{
  // TODO This pattern looks like it is inefficient. Should it use the GPU pool instead somehow?
  m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage(m_pConstants);

  pRenderContext->BindConstantBuffer("ezParticleSystemConstants", m_hConstantBuffer);
}

ezParticleRenderer::TempSystemCB::~TempSystemCB()
{
  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void ezParticleRenderer::TempSystemCB::SetGenericData(bool bApplyObjectTransform, const ezTransform& ObjectTransform, ezUInt8 uiNumSpritesX,
                                                      ezUInt8 uiNumSpritesY, float fDistortionStrength /*= 0*/)
{
  ezParticleSystemConstants& cb = m_pConstants->GetDataForWriting();
  cb.NumSpritesX = uiNumSpritesX;
  cb.NumSpritesY = uiNumSpritesY;
  cb.DistortionStrength = fDistortionStrength;

  if (bApplyObjectTransform)
    cb.ObjectToWorldMatrix = ObjectTransform.GetAsMat4();
  else
    cb.ObjectToWorldMatrix.SetIdentity();
}


void ezParticleRenderer::TempSystemCB::SetTrailData(float fSnapshotFraction, ezInt32 iNumUsedTrailPoints)
{
  ezParticleSystemConstants& cb = m_pConstants->GetDataForWriting();
  cb.SnapshotFraction = fSnapshotFraction;
  cb.NumUsedTrailPoints = iNumUsedTrailPoints;
}

ezParticleRenderer::ezParticleRenderer() {}

ezParticleRenderer::~ezParticleRenderer() {}

void ezParticleRenderer::CreateParticleDataBuffer(ezGALBufferHandle& inout_hBuffer, ezUInt32 uiDataTypeSize,
                                                  ezUInt32 uiNumParticlesPerBatch)
{
  if (inout_hBuffer.IsInvalidated())
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = uiDataTypeSize;
    desc.m_uiTotalSize = uiNumParticlesPerBatch * desc.m_uiStructSize;
    desc.m_BufferType = ezGALBufferType::Generic;
    desc.m_bUseAsStructuredBuffer = true;
    desc.m_bAllowShaderResourceView = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    inout_hBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }
}


void ezParticleRenderer::DestroyParticleDataBuffer(ezGALBufferHandle& inout_hBuffer)
{
  if (!inout_hBuffer.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(inout_hBuffer);
    inout_hBuffer.Invalidate();
  }
}

void ezParticleRenderer::BindParticleShader(ezRenderContext* pRenderContext, const char* szShader)
{
  if (!m_hShader.IsValid())
  {
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>(szShader);
  }

  pRenderContext->BindShader(m_hShader);
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Renderer_ParticleRenderer);
