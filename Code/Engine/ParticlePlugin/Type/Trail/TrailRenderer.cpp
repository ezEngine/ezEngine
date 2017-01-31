#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Type/Trail/TrailRenderer.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <Foundation/Types/ScopeExit.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/ParticleSystemConstants.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTrailRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTrailRenderer, 1, ezRTTIDefaultAllocator<ezParticleTrailRenderer>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleTrailRenderer::~ezParticleTrailRenderer()
{
  if (!m_hParticleDataBuffer.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hParticleDataBuffer);
    m_hParticleDataBuffer.Invalidate();
  }

  //m_hActiveTrailPointsDataBuffer.Invalidate();

  if (!m_hTrailPointsDataBuffer8.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hTrailPointsDataBuffer8);
    m_hTrailPointsDataBuffer8.Invalidate();
  }

  if (!m_hTrailPointsDataBuffer16.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hTrailPointsDataBuffer16);
    m_hTrailPointsDataBuffer16.Invalidate();
  }

  if (!m_hTrailPointsDataBuffer32.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hTrailPointsDataBuffer32);
    m_hTrailPointsDataBuffer32.Invalidate();
  }

  if (!m_hTrailPointsDataBuffer64.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hTrailPointsDataBuffer64);
    m_hTrailPointsDataBuffer64.Invalidate();
  }
}

void ezParticleTrailRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types)
{
  types.PushBack(ezGetStaticRTTI<ezParticleTrailRenderData>());
}

void ezParticleTrailRenderer::CreateDataBuffer()
{
  if (m_hParticleDataBuffer.IsInvalidated())
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ezTrailParticleData);
    desc.m_uiTotalSize = s_uiParticlesPerBatch * desc.m_uiStructSize;
    desc.m_BufferType = ezGALBufferType::Generic;
    desc.m_bUseAsStructuredBuffer = true;
    desc.m_bAllowShaderResourceView = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hParticleDataBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }

  // this is kinda stupid, apparently due to stride enforcement I cannot reuse the same buffer for different sizes
  // and instead have to create one buffer with every size ...

  if (m_hTrailPointsDataBuffer8.IsInvalidated())
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ezTrailParticlePointsData8);
    desc.m_uiTotalSize = s_uiParticlesPerBatch * desc.m_uiStructSize;
    desc.m_BufferType = ezGALBufferType::Generic;
    desc.m_bUseAsStructuredBuffer = true;
    desc.m_bAllowShaderResourceView = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hTrailPointsDataBuffer8 = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }

  if (m_hTrailPointsDataBuffer16.IsInvalidated())
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ezTrailParticlePointsData16);
    desc.m_uiTotalSize = s_uiParticlesPerBatch * desc.m_uiStructSize;
    desc.m_BufferType = ezGALBufferType::Generic;
    desc.m_bUseAsStructuredBuffer = true;
    desc.m_bAllowShaderResourceView = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hTrailPointsDataBuffer16 = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }

  if (m_hTrailPointsDataBuffer32.IsInvalidated())
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ezTrailParticlePointsData32);
    desc.m_uiTotalSize = s_uiParticlesPerBatch * desc.m_uiStructSize;
    desc.m_BufferType = ezGALBufferType::Generic;
    desc.m_bUseAsStructuredBuffer = true;
    desc.m_bAllowShaderResourceView = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hTrailPointsDataBuffer32 = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }

  if (m_hTrailPointsDataBuffer64.IsInvalidated())
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ezTrailParticlePointsData64);
    desc.m_uiTotalSize = s_uiParticlesPerBatch * desc.m_uiStructSize;
    desc.m_BufferType = ezGALBufferType::Generic;
    desc.m_bUseAsStructuredBuffer = true;
    desc.m_bAllowShaderResourceView = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hTrailPointsDataBuffer64 = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }
}

void ezParticleTrailRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();

  /// \todo This pattern looks like it is inefficient. Should it use the GPU pool instead somehow?
  // prepare the constant buffer
  ezConstantBufferStorage<ezParticleSystemConstants>* pConstantBuffer;
  ezConstantBufferStorageHandle hConstantBuffer = ezRenderContext::CreateConstantBufferStorage(pConstantBuffer);
  EZ_SCOPE_EXIT(ezRenderContext::DeleteConstantBufferStorage(hConstantBuffer));
  renderViewContext.m_pRenderContext->BindConstantBuffer("ezParticleSystemConstants", hConstantBuffer);

  // Bind the Trail particle shader
  {
    if (!m_hShader.IsValid())
    {
      m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Particles/Trail.ezShader");
    }

    renderViewContext.m_pRenderContext->BindShader(m_hShader);
  }

  // make sure our structured buffer is allocated and bound
  {
    CreateDataBuffer();
    renderViewContext.m_pRenderContext->BindBuffer(ezGALShaderStage::VertexShader, "particleData", pDevice->GetDefaultResourceView(m_hParticleDataBuffer));
  }

  // now render all particle effects of type Trail
  for (auto it = batch.GetIterator<ezParticleTrailRenderData>(0, batch.GetCount()); it.IsValid(); ++it)
  {
    const ezParticleTrailRenderData* pRenderData = it;

    if (!ConfigureShader(pRenderData, renderViewContext))
      continue;

    renderViewContext.m_pRenderContext->BindBuffer(ezGALShaderStage::VertexShader, "particlePointsData", pDevice->GetDefaultResourceView(m_hActiveTrailPointsDataBuffer));

    const ezUInt32 uiBucketSize = ezParticleTypeTrail::ComputeTrailPointBucketSize(pRenderData->m_uiMaxTrailPoints);
    const ezUInt32 uiMaxTrailSegments = uiBucketSize - 1;
    const ezUInt32 uiPrimFactor = 2;
    const ezUInt32 uiMaxPrimitivesToRender = s_uiParticlesPerBatch * uiMaxTrailSegments * uiPrimFactor;
    renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, uiMaxPrimitivesToRender);

    const ezTrailParticleData* pParticleData = pRenderData->m_ParticleDataShared.GetPtr();
    const ezVec4* pParticlePointsData = pRenderData->m_TrailPointsShared.GetPtr();

    renderViewContext.m_pRenderContext->BindTexture2D(ezGALShaderStage::PixelShader, "ParticleTexture", pRenderData->m_hTexture);

    // fill the constant buffer
    {
      ezParticleSystemConstants& cb = pConstantBuffer->GetDataForWriting();
      cb.ObjectToWorldMatrix = pRenderData->m_GlobalTransform.GetAsMat4();
      cb.NumUsedTrailPoints = pRenderData->m_uiMaxTrailPoints;
      cb.SnapshotFraction = pRenderData->m_fSnapshotFraction;
    }

    ezUInt32 uiNumParticles = pRenderData->m_ParticleDataShared.GetCount();
    while (uiNumParticles > 0)
    {
      // upload this batch of particle data
      const ezUInt32 uiNumParticlesInBatch = ezMath::Min<ezUInt32>(uiNumParticles, s_uiParticlesPerBatch);
      pGALContext->UpdateBuffer(m_hParticleDataBuffer, 0, ezMakeArrayPtr(pParticleData, uiNumParticlesInBatch).ToByteArray());
      pGALContext->UpdateBuffer(m_hActiveTrailPointsDataBuffer, 0, ezMakeArrayPtr(pParticlePointsData, uiNumParticlesInBatch * uiBucketSize).ToByteArray());

      // do one drawcall
      renderViewContext.m_pRenderContext->DrawMeshBuffer(uiNumParticlesInBatch * uiMaxTrailSegments * uiPrimFactor);

      uiNumParticles -= uiNumParticlesInBatch;
      pParticleData += uiNumParticlesInBatch;
      pParticlePointsData += uiNumParticlesInBatch;
    }
  }
}

bool ezParticleTrailRenderer::ConfigureShader(const ezParticleTrailRenderData* pRenderData, const ezRenderViewContext &renderViewContext)
{
  switch (ezParticleTypeTrail::ComputeTrailPointBucketSize(pRenderData->m_uiMaxTrailPoints))
  {
  case 8:
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_TRAIL_POINTS", "COUNT8");
    m_hActiveTrailPointsDataBuffer = m_hTrailPointsDataBuffer8;
    return true;
  case 16:
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_TRAIL_POINTS", "COUNT16");
    m_hActiveTrailPointsDataBuffer = m_hTrailPointsDataBuffer16;
    return true;
  case 32:
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_TRAIL_POINTS", "COUNT32");
    m_hActiveTrailPointsDataBuffer = m_hTrailPointsDataBuffer32;
    return true;
  case 64:
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_TRAIL_POINTS", "COUNT64");
    m_hActiveTrailPointsDataBuffer = m_hTrailPointsDataBuffer64;
    return true;
  }

  return false;
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Trail_TrailRenderer);

