#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Lights/SSRDataProvider.h>
#include <RendererCore/Pipeline/Passes/SSRPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSSRPass, 2, ezRTTIDefaultAllocator<ezSSRPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    EZ_MEMBER_PROPERTY("GBuffer1", m_PinGBuffer1),
    EZ_MEMBER_PROPERTY("Color", m_PinColor),
    EZ_ACCESSOR_PROPERTY("MaxRaySteps", GetMaxRaySteps, SetMaxRaySteps)->AddAttributes(new ezDefaultValueAttribute(64), new ezClampValueAttribute(16, 256)),
    EZ_MEMBER_PROPERTY("Thickness", m_fThickness)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.01f, 5.0f)),
    EZ_MEMBER_PROPERTY("MaxRayDistance", m_fMaxRayDistance)->AddAttributes(new ezDefaultValueAttribute(100.0f), new ezClampValueAttribute(1.0f, 500.0f)),
    EZ_ACCESSOR_PROPERTY("RoughnessThreshold", GetRoughnessThreshold, SetRoughnessThreshold)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("TemporalBlendWeight", m_fTemporalBlendWeight)->AddAttributes(new ezDefaultValueAttribute(0.05f), new ezClampValueAttribute(0.01f, 0.5f)),
    EZ_MEMBER_PROPERTY("EdgeFadeStart", m_fEdgeFadeStart)->AddAttributes(new ezDefaultValueAttribute(0.05f), new ezClampValueAttribute(0.01f, 0.3f)),
    EZ_MEMBER_PROPERTY("EdgeFadeEnd", m_fEdgeFadeEnd)->AddAttributes(new ezDefaultValueAttribute(0.15f), new ezClampValueAttribute(0.01f, 0.3f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Effects")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSSRPass::ezSSRPass()
  : ezRenderPipelinePass("SSRPass", true)
{
  m_hBuildHiZShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/SSRBuildHiZ.ezShader");
  m_hClassifyShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/SSRClassify.ezShader");
  m_hPrepareArgsShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/SSRPrepareArgs.ezShader");
  m_hTraceShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/SSRTrace.ezShader");
  m_hSpatialBlurShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/SSRSpatialBlur.ezShader");
  m_hTemporalShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/SSRTemporal.ezShader");

  m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezSSRConstants>();
}

ezSSRPass::~ezSSRPass()
{
  DestroyResources();
  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

bool ezSSRPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  if (!inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    ezLog::Error("No depth input connected to '{0}'!", GetName());
    return false;
  }

  // GBuffer1 is optional: when not connected (Forward+ pipeline), normals are reconstructed from depth

  // PassThrough: copy input description to output
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    outputs[m_PinColor.m_uiOutputIndex] = *inputs[m_PinColor.m_uiInputIndex];
  }

  return true;
}

void ezSSRPass::InitRenderPipelinePass(const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  if (auto pDepthInput = inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    EnsureResources(pDepthInput->m_Desc.m_uiWidth, pDepthInput->m_Desc.m_uiHeight);
  }
}

void ezSSRPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  auto pDepthInput = inputs[m_PinDepthStencil.m_uiInputIndex];
  auto pGBuffer1Input = inputs[m_PinGBuffer1.m_uiInputIndex]; // may be null in Forward+
  auto pColorOutput = outputs[m_PinColor.m_uiOutputIndex];

  if (pDepthInput == nullptr || pColorOutput == nullptr)
    return;

  const bool bHasGBuffer = (pGBuffer1Input != nullptr);

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Check if we need to recreate resources (resolution changed)
  const ezUInt32 uiWidth = pDepthInput->m_Desc.m_uiWidth;
  const ezUInt32 uiHeight = pDepthInput->m_Desc.m_uiHeight;
  if (uiWidth != m_uiWidth || uiHeight != m_uiHeight)
  {
    EnsureResources(uiWidth, uiHeight);
  }

  // Determine current / history SSR textures
  ezGALTextureHandle hCurrentSSR = m_bUseResultA ? m_hSSRResultA : m_hSSRResultB;
  ezGALTextureHandle hHistorySSR = m_bUseResultA ? m_hSSRResultB : m_hSSRResultA;

  // Update constant buffer
  {
    ezSSRConstants* cb = ezRenderContext::GetConstantBufferData<ezSSRConstants>(m_hConstantBuffer);
    cb->SSRResolutionX = uiWidth;
    cb->SSRResolutionY = uiHeight;
    cb->SSRThickness = m_fThickness;
    cb->SSRMaxSteps = m_uiMaxRaySteps;
    cb->SSRMaxDistance = m_fMaxRayDistance;
    cb->SSRRoughnessThreshold = m_fRoughnessThreshold;
    cb->SSRTemporalBlendWeight = m_fTemporalBlendWeight;
    cb->SSRBRDFBias = 0.0f;
    cb->SSRInverseResolution = ezVec2(1.0f / uiWidth, 1.0f / uiHeight);
    cb->SSRHiZMipCount = m_uiHiZMipCount;
    cb->SSRHiZResolutionX = uiWidth;
    cb->SSRHiZResolutionY = uiHeight;
    cb->SSRCurrentMipLevel = 0;
    cb->SSRHasGBuffer = bHasGBuffer ? 1 : 0;
    cb->SSRFrameIndex = m_uiFrameIndex;
    cb->SSRPrevWorldToClipMatrix = m_PrevViewProjectionMatrix;
    cb->SSREdgeFadeStart = m_fEdgeFadeStart;
    cb->SSREdgeFadeEnd = m_fEdgeFadeEnd;
    cb->SSRBlurRadius = m_fBlurRadius;
    cb->SSRBlurSharpness = m_fBlurSharpness;
  }

  // ---- Sub-pass 1: Clear indirect args buffer ----
  {
    auto computeScope = renderViewContext.m_pRenderContext->BeginComputeScope(renderViewContext, "SSR Clear Args");

    // Clear to (0, 1, 1) so Y and Z dispatch dimensions are always 1
    ezUInt32 clearData[3] = {0, 1, 1};
    renderViewContext.m_pRenderContext->GetCommandEncoder()->UpdateBuffer(m_hIndirectArgsBuffer, 0, ezMakeArrayPtr(reinterpret_cast<const ezUInt8*>(clearData), sizeof(clearData)));
  }

  // ---- Sub-pass 2: Build Hi-Z mip chain ----
  {
    EZ_PROFILE_SCOPE("SSR Build HiZ");

    ezUInt32 mipWidth = uiWidth;
    ezUInt32 mipHeight = uiHeight;

    for (ezUInt32 mip = 0; mip < m_uiHiZMipCount; ++mip)
    {
      {
        auto computeScope = renderViewContext.m_pRenderContext->BeginComputeScope(renderViewContext, "SSR HiZ Mip");

        ezUInt32 destWidth = ezMath::Max(mipWidth / 2, 1u);
        ezUInt32 destHeight = ezMath::Max(mipHeight / 2, 1u);
        if (mip == 0)
        {
          destWidth = mipWidth;
          destHeight = mipHeight;
        }

        // Update current mip level in constants
        {
          ezSSRConstants* cb = ezRenderContext::GetConstantBufferData<ezSSRConstants>(m_hConstantBuffer);
          cb->SSRCurrentMipLevel = mip;
          cb->SSRHiZResolutionX = destWidth;
          cb->SSRHiZResolutionY = destHeight;
        }

        ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
        bindGroup.BindBuffer("ezSSRConstants", m_hConstantBuffer);

        if (mip == 0)
        {
          // Mip 0: source is the scene depth
          bindGroup.BindTexture("HiZSource", pDepthInput->m_TextureHandle);
        }
        else
        {
          // Subsequent mips: source is the temp texture (contains all previously written mips)
          bindGroup.BindTexture("HiZSource", m_hHiZTemp);
        }
        // Per-mip UAV: writes go to exactly mip N of HiZTexture
        bindGroup.BindTexture("HiZDest", m_hHiZTexture, ezGALTextureRange::MakeFromMipRange(mip, 1));

        renderViewContext.m_pRenderContext->BindShader(m_hBuildHiZShader);

        ezUInt32 dispatchX = (destWidth + SSR_HIZ_THREAD_GROUP_SIZE - 1) / SSR_HIZ_THREAD_GROUP_SIZE;
        ezUInt32 dispatchY = (destHeight + SSR_HIZ_THREAD_GROUP_SIZE - 1) / SSR_HIZ_THREAD_GROUP_SIZE;
        renderViewContext.m_pRenderContext->Dispatch(dispatchX, dispatchY, 1).IgnoreResult();

        mipWidth = destWidth;
        mipHeight = destHeight;
      } // compute scope ends here, resources are unbound

      // Copy HiZTexture -> HiZTemp so the next iteration can read the just-written mip as SRV
      // without SRV/UAV conflict (reading from a different texture than we write to).
      if (mip < m_uiHiZMipCount - 1)
      {
        renderViewContext.m_pRenderContext->GetCommandEncoder()->CopyTexture(m_hHiZTemp, m_hHiZTexture);
      }
    }
  }

  // ---- Sub-pass 3: Classify reflective pixels ----
  {
    EZ_PROFILE_SCOPE("SSR Classify");
    auto computeScope = renderViewContext.m_pRenderContext->BeginComputeScope(renderViewContext, "SSR Classify");

    ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
    bindGroup.BindBuffer("ezSSRConstants", m_hConstantBuffer);
    if (bHasGBuffer)
    {
      bindGroup.BindTexture("GBuffer1Texture", pGBuffer1Input->m_TextureHandle);
    }
    else
    {
      // Bind depth as a dummy so the descriptor slot is not left empty.
      // The shader gates all GBuffer1 reads behind SSRHasGBuffer.
      bindGroup.BindTexture("GBuffer1Texture", pDepthInput->m_TextureHandle);
    }
    bindGroup.BindTexture("SceneDepth", pDepthInput->m_TextureHandle);
    bindGroup.BindBuffer("SSRClassifyBuffer", m_hClassifyBuffer);
    bindGroup.BindBuffer("SSRIndirectArgs", m_hIndirectArgsBuffer);

    renderViewContext.m_pRenderContext->BindShader(m_hClassifyShader);

    ezUInt32 dispatchX = (uiWidth + SSR_CLASSIFY_THREAD_GROUP_SIZE - 1) / SSR_CLASSIFY_THREAD_GROUP_SIZE;
    ezUInt32 dispatchY = (uiHeight + SSR_CLASSIFY_THREAD_GROUP_SIZE - 1) / SSR_CLASSIFY_THREAD_GROUP_SIZE;
    renderViewContext.m_pRenderContext->Dispatch(dispatchX, dispatchY, 1).IgnoreResult();
  }

  // ---- Sub-pass 4: Prepare indirect dispatch args ----
  {
    EZ_PROFILE_SCOPE("SSR Prepare Args");
    auto computeScope = renderViewContext.m_pRenderContext->BeginComputeScope(renderViewContext, "SSR Prepare Args");

    ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
    bindGroup.BindBuffer("SSRIndirectArgs", m_hIndirectArgsBuffer);

    renderViewContext.m_pRenderContext->BindShader(m_hPrepareArgsShader);
    renderViewContext.m_pRenderContext->Dispatch(1, 1, 1).IgnoreResult();
  }

  // ---- Sub-pass 5: Trace (INDIRECT DISPATCH) ----
  {
    EZ_PROFILE_SCOPE("SSR Trace");
    auto computeScope = renderViewContext.m_pRenderContext->BeginComputeScope(renderViewContext, "SSR Trace");

    ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
    bindGroup.BindBuffer("ezSSRConstants", m_hConstantBuffer);
    bindGroup.BindTexture("HiZTexture", m_hHiZTexture);
    bindGroup.BindTexture("PreviousFrameColor", pColorOutput->m_TextureHandle);
    bindGroup.BindTexture("SceneDepth", pDepthInput->m_TextureHandle);
    if (bHasGBuffer)
    {
      bindGroup.BindTexture("GBuffer1Texture", pGBuffer1Input->m_TextureHandle);
    }
    else
    {
      bindGroup.BindTexture("GBuffer1Texture", pDepthInput->m_TextureHandle);
    }
    bindGroup.BindTexture("SSRResult", m_hSSRTraceResult);
    bindGroup.BindBuffer("SSRClassifyBuffer", m_hClassifyBuffer);

    renderViewContext.m_pRenderContext->BindShader(m_hTraceShader);

    // ApplyContextStates must succeed before DispatchIndirect — unlike Dispatch(),
    // there is no ezRenderContext wrapper that checks this automatically.
    if (renderViewContext.m_pRenderContext->ApplyContextStates().Succeeded())
    {
      renderViewContext.m_pRenderContext->GetCommandEncoder()->DispatchIndirect(m_hIndirectArgsBuffer, 0);
    }
  }

  // ---- Sub-pass 6: Spatial pre-filter ----
  {
    EZ_PROFILE_SCOPE("SSR Spatial Blur");
    auto computeScope = renderViewContext.m_pRenderContext->BeginComputeScope(renderViewContext, "SSR Blur");

    ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
    bindGroup.BindBuffer("ezSSRConstants", m_hConstantBuffer);
    bindGroup.BindTexture("SSRInput", m_hSSRTraceResult);
    bindGroup.BindTexture("SceneDepth", pDepthInput->m_TextureHandle);
    if (bHasGBuffer)
    {
      // NOTE(Mikael A.): Ignored if deferred pass isnt enabled, and isnt needed for the OSS backport.
      bindGroup.BindTexture("GBuffer1Texture", pGBuffer1Input->m_TextureHandle);
    }
    else
    {
        // NOTE(Mikael A.): Same as above.
      bindGroup.BindTexture("GBuffer1Texture", pDepthInput->m_TextureHandle);
    }
    bindGroup.BindTexture("SSRBlurOutput", m_hSSRBlurIntermediate);

    renderViewContext.m_pRenderContext->BindShader(m_hSpatialBlurShader);

    ezUInt32 dispatchX = (uiWidth + SSR_BLUR_THREAD_GROUP_SIZE - 1) / SSR_BLUR_THREAD_GROUP_SIZE;
    ezUInt32 dispatchY = (uiHeight + SSR_BLUR_THREAD_GROUP_SIZE - 1) / SSR_BLUR_THREAD_GROUP_SIZE;
    renderViewContext.m_pRenderContext->Dispatch(dispatchX, dispatchY, 1).IgnoreResult();
  }

  // Publish spatial blur result directly (no temporal accumulation).
  auto pSSRProvider = GetPipeline()->GetFrameDataProvider<ezSSRDataProvider>();
  if (pSSRProvider != nullptr)
  {
    auto pSSRData = pSSRProvider->GetData(renderViewContext);
    if (pSSRData != nullptr)
    {
      pSSRProvider->SetSSRTexture(m_hSSRBlurIntermediate);
    }
  }

  // Store current VP matrix for next-frame temporal reprojection
  m_PrevViewProjectionMatrix = renderViewContext.m_pViewData->m_ViewProjectionMatrix[0];

  // Advance frame index for temporal noise jitter
  m_uiFrameIndex++;

  // Swap ping-pong
  m_bUseResultA = !m_bUseResultA;
}

void ezSSRPass::ExecuteInactive(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  // PassThrough: nothing to do, the color buffer passes through unchanged
}

void ezSSRPass::EnsureResources(ezUInt32 uiWidth, ezUInt32 uiHeight)
{
  if (uiWidth == m_uiWidth && uiHeight == m_uiHeight && !m_hHiZTexture.IsInvalidated())
    return;

  DestroyResources();

  m_uiWidth = uiWidth;
  m_uiHeight = uiHeight;

  // Calculate Hi-Z mip count
  m_uiHiZMipCount = 1;
  {
    ezUInt32 w = uiWidth;
    ezUInt32 h = uiHeight;
    while (w > 1 || h > 1)
    {
      w = ezMath::Max(w / 2, 1u);
      h = ezMath::Max(h / 2, 1u);
      m_uiHiZMipCount++;
    }
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  
  // TODO(Mikael A.): High Z should be ignored if history preservation is disabled!!
  // Hi-Z texture (R32Float with full mip chain)
  {
    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = uiWidth;
    desc.m_uiHeight = uiHeight;
    desc.m_Format = ezGALResourceFormat::RFloat;
    desc.m_uiMipLevelCount = m_uiHiZMipCount;
    desc.m_TextureFlags = ezGALTextureUsageFlags::ShaderResource | ezGALTextureUsageFlags::UnorderedAccess;
    desc.m_ResourceAccess.m_bImmutable = false;
    m_hHiZTexture = pDevice->CreateTexture(desc);
    m_hHiZTemp = pDevice->CreateTexture(desc);
  }

  // SSR result textures (temporal ping-pong)
  {
    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = uiWidth;
    desc.m_uiHeight = uiHeight;
    desc.m_Format = ezGALResourceFormat::RGBAHalf;
    desc.m_TextureFlags = ezGALTextureUsageFlags::ShaderResource | ezGALTextureUsageFlags::UnorderedAccess;
    desc.m_ResourceAccess.m_bImmutable = false;
    m_hSSRResultA = pDevice->CreateTexture(desc);
    m_hSSRResultB = pDevice->CreateTexture(desc);
    m_hSSRTraceResult = pDevice->CreateTexture(desc);
    m_hSSRBlurIntermediate = pDevice->CreateTexture(desc);
  }

  // Classify buffer (ByteAddressBuffer: max pixels * 4 bytes)
  {
    const ezUInt32 maxPixels = uiWidth * uiHeight;
    ezGALBufferCreationDescription desc;
    desc.m_uiTotalSize = maxPixels * sizeof(ezUInt32);
    desc.m_BufferFlags = ezGALBufferUsageFlags::UnorderedAccess | ezGALBufferUsageFlags::ShaderResource | ezGALBufferUsageFlags::ByteAddressBuffer;
    desc.m_ResourceAccess.m_bImmutable = false;
    m_hClassifyBuffer = pDevice->CreateBuffer(desc);
  }

  // Indirect args buffer (3 x uint32 = 12 bytes)
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiTotalSize = 12;
    desc.m_BufferFlags = ezGALBufferUsageFlags::DrawIndirect | ezGALBufferUsageFlags::UnorderedAccess | ezGALBufferUsageFlags::ByteAddressBuffer;
    desc.m_ResourceAccess.m_bImmutable = false;
    m_hIndirectArgsBuffer = pDevice->CreateBuffer(desc);
  }
}

void ezSSRPass::DestroyResources()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  if (!m_hHiZTexture.IsInvalidated())
    pDevice->DestroyTexture(m_hHiZTexture);
  if (!m_hHiZTemp.IsInvalidated())
    pDevice->DestroyTexture(m_hHiZTemp);
  if (!m_hSSRResultA.IsInvalidated())
    pDevice->DestroyTexture(m_hSSRResultA);
  if (!m_hSSRResultB.IsInvalidated())
    pDevice->DestroyTexture(m_hSSRResultB);
  if (!m_hSSRTraceResult.IsInvalidated())
    pDevice->DestroyTexture(m_hSSRTraceResult);
  if (!m_hSSRBlurIntermediate.IsInvalidated())
    pDevice->DestroyTexture(m_hSSRBlurIntermediate);
  if (!m_hClassifyBuffer.IsInvalidated())
    pDevice->DestroyBuffer(m_hClassifyBuffer);
  if (!m_hIndirectArgsBuffer.IsInvalidated())
    pDevice->DestroyBuffer(m_hIndirectArgsBuffer);

  m_hHiZTexture.Invalidate();
  m_hHiZTemp.Invalidate();
  m_hSSRResultA.Invalidate();
  m_hSSRResultB.Invalidate();
  m_hSSRTraceResult.Invalidate();
  m_hSSRBlurIntermediate.Invalidate();
  m_hClassifyBuffer.Invalidate();
  m_hIndirectArgsBuffer.Invalidate();

  m_uiWidth = 0;
  m_uiHeight = 0;
}

ezResult ezSSRPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_uiMaxRaySteps;
  inout_stream << m_fThickness;
  inout_stream << m_fMaxRayDistance;
  inout_stream << m_fRoughnessThreshold;
  inout_stream << m_fTemporalBlendWeight;
  inout_stream << m_fEdgeFadeStart;
  inout_stream << m_fEdgeFadeEnd;
  return EZ_SUCCESS;
}

ezResult ezSSRPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_uiMaxRaySteps;
  inout_stream >> m_fThickness;
  inout_stream >> m_fMaxRayDistance;
  inout_stream >> m_fRoughnessThreshold;
  inout_stream >> m_fTemporalBlendWeight;
  if (uiVersion >= 2)
  {
    inout_stream >> m_fEdgeFadeStart;
    inout_stream >> m_fEdgeFadeEnd;
  }
  return EZ_SUCCESS;
}

void ezSSRPass::SetMaxRaySteps(ezUInt32 uiSteps)
{
  m_uiMaxRaySteps = ezMath::Clamp(uiSteps, 16u, 256u);
}

ezUInt32 ezSSRPass::GetMaxRaySteps() const
{
  return m_uiMaxRaySteps;
}

void ezSSRPass::SetRoughnessThreshold(float fThreshold)
{
  m_fRoughnessThreshold = ezMath::Clamp(fThreshold, 0.0f, 1.0f);
}

float ezSSRPass::GetRoughnessThreshold() const
{
  return m_fRoughnessThreshold;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SSRPass);