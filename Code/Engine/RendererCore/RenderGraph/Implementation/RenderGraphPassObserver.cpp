#include <RendererCore/RendererCorePCH.h>

#include <Core/ResourceManager/Implementation/ResourceLock.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/RenderGraph/RenderGraphPassBuilder.h>
#include <RendererCore/RenderGraph/RenderGraphPassObserver.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/RenderGraph/RenderGraphHistogramConstants.h>
#include <RendererCore/../../../Data/Base/Shaders/RenderGraph/RenderGraphMinMaxConstants.h>
#include <RendererCore/../../../Data/Base/Shaders/RenderGraph/RenderGraphPreviewConstants.h>
#include <RendererCore/../../../Data/Base/Shaders/RenderGraph/RenderGraphReadbackPixelConstants.h>

static ezAtomicInteger32 s_uiObserverTextureCounter;

namespace
{
  constexpr ezUInt32 s_uiHistogramBinCount = 256;
  constexpr ezUInt32 s_uiHistogramChannelCount = 4;
  constexpr ezUInt32 s_uiMinMaxValueCount = 2;

  float GetAdjustedHistogramMax(float fMin, float fMax)
  {
    const float fRange = ezMath::Max(fMax - fMin, 0.0000001f);
    return fMax + fRange * 0.000001f;
  }

  float OrderedUintToFloat(ezUInt32 uiOrderedValue)
  {
    // Inverse of FloatToOrderedUint in RenderGraphBuildMinMax.ezShader.
    // The shader stores min/max floats in an integer domain that preserves float ordering for atomic operations.
    const ezUInt32 uiRawValue = (uiOrderedValue & 0x80000000u) != 0u ? (uiOrderedValue ^ 0x80000000u) : ~uiOrderedValue;
    return *reinterpret_cast<const float*>(&uiRawValue);
  }
} // namespace

ezRenderGraphPassObserver::ezRenderGraphPassObserver(ezGALDevice* pDevice)
  : m_pDevice(pDevice)
{
  m_Response.m_Histogram.SetCount(s_uiHistogramBinCount * s_uiHistogramChannelCount);
  ezMemoryUtils::ZeroFill(m_Response.m_Histogram.GetData(), m_Response.m_Histogram.GetCount());
}

ezRenderGraphPassObserver::~ezRenderGraphPassObserver()
{
  DestroyInspectionResources();
  m_hCopyTextureResource.Invalidate();
  m_hCopyTexture.Invalidate();
}

ezRenderGraphObserverResponse ezRenderGraphPassObserver::GetResponse() const
{
  EZ_LOCK(m_Mutex);
  return m_Response;
}

void ezRenderGraphPassObserver::SetRequest(const ezRenderGraphObserverRequest& request)
{
  EZ_LOCK(m_Mutex);
  m_PendingRequest = request;
  m_bPendingRequestDirty = true;
}

void ezRenderGraphPassObserver::ApplyPendingRequest()
{
  EZ_LOCK(m_Mutex);
  if (m_bPendingRequestDirty)
  {
    m_Request = m_PendingRequest;
    m_pGraph = ezRenderGraphManager::GetRenderGraphById(m_Request.m_uiRenderGraphId);
    m_hSwapChain = ezRenderGraphManager::GetSwapChainById(m_Request.m_uiSwapChainId);
    m_bPendingRequestDirty = false;
  }
}

void ezRenderGraphPassObserver::EnsureInspectionResources()
{
  if (m_hPixelReadbackBuffer.IsInvalidated())
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ezVec4);
    desc.m_uiTotalSize = desc.m_uiStructSize;
    desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource | ezGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceAccess.m_bImmutable = false;
    m_hPixelReadbackBuffer = m_pDevice->CreateBuffer(desc);
  }

  if (m_hHistogramBuffer.IsInvalidated())
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ezUInt32);
    desc.m_uiTotalSize = s_uiHistogramBinCount * s_uiHistogramChannelCount * desc.m_uiStructSize;
    desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource | ezGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceAccess.m_bImmutable = false;
    m_hHistogramBuffer = m_pDevice->CreateBuffer(desc);
  }

  if (m_hMinMaxBuffer.IsInvalidated())
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ezUInt32);
    desc.m_uiTotalSize = s_uiMinMaxValueCount * desc.m_uiStructSize;
    desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource | ezGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceAccess.m_bImmutable = false;
    m_hMinMaxBuffer = m_pDevice->CreateBuffer(desc);
  }

  if (!m_hReadbackPixelShader.IsValid())
  {
    m_hReadbackPixelShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/RenderGraph/RenderGraphReadbackPixel.ezShader");
  }

  if (!m_hClearHistogramShader.IsValid())
  {
    m_hClearHistogramShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/RenderGraph/RenderGraphClearHistogram.ezShader");
  }

  if (!m_hBuildHistogramShader.IsValid())
  {
    m_hBuildHistogramShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/RenderGraph/RenderGraphBuildHistogram.ezShader");
  }

  if (!m_hClearMinMaxShader.IsValid())
  {
    m_hClearMinMaxShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/RenderGraph/RenderGraphClearMinMax.ezShader");
  }

  if (!m_hBuildMinMaxShader.IsValid())
  {
    m_hBuildMinMaxShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/RenderGraph/RenderGraphBuildMinMax.ezShader");
  }

  if (!m_hPreviewShader.IsValid())
  {
    m_hPreviewShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/RenderGraph/RenderGraphPreview.ezShader");
  }
}

void ezRenderGraphPassObserver::DestroyInspectionResources()
{
  m_PixelReadback.Reset();
  m_HistogramReadback.Reset();
  m_MinMaxReadback.Reset();
  m_pDevice->DestroyBuffer(m_hPixelReadbackBuffer);
  m_pDevice->DestroyBuffer(m_hHistogramBuffer);
  m_pDevice->DestroyBuffer(m_hMinMaxBuffer);
  m_hReadbackPixelShader.Invalidate();
  m_hClearHistogramShader.Invalidate();
  m_hBuildHistogramShader.Invalidate();
  m_hClearMinMaxShader.Invalidate();
  m_hBuildMinMaxShader.Invalidate();
  m_hPreviewShader.Invalidate();
}

void ezRenderGraphPassObserver::PollReadbacks()
{
  if (m_bPixelReadbackInFlight)
  {
    const ezEnum<ezGALAsyncResult> result = m_PixelReadback.GetReadbackResult(ezTime::MakeZero());
    if (result == ezGALAsyncResult::Ready)
    {
      ezArrayPtr<const ezUInt8> memory;
      if (ezReadbackBufferLock lock = m_PixelReadback.LockBuffer(memory))
      {
        ProcessPixelReadback(memory);
      }
      m_bPixelReadbackInFlight = false;
    }
    else if (result == ezGALAsyncResult::Expired)
    {
      m_bPixelReadbackInFlight = false;
    }
  }

  if (m_bHistogramReadbackInFlight)
  {
    const ezEnum<ezGALAsyncResult> result = m_HistogramReadback.GetReadbackResult(ezTime::MakeZero());
    if (result == ezGALAsyncResult::Ready)
    {
      ezArrayPtr<const ezUInt8> memory;
      if (ezReadbackBufferLock lock = m_HistogramReadback.LockBuffer(memory))
      {
        ProcessHistogramReadback(memory);
      }
      m_bHistogramReadbackInFlight = false;
    }
    else if (result == ezGALAsyncResult::Expired)
    {
      m_bHistogramReadbackInFlight = false;
    }
  }

  if (m_bMinMaxReadbackInFlight)
  {
    const ezEnum<ezGALAsyncResult> result = m_MinMaxReadback.GetReadbackResult(ezTime::MakeZero());
    if (result == ezGALAsyncResult::Ready)
    {
      ezArrayPtr<const ezUInt8> memory;
      if (ezReadbackBufferLock lock = m_MinMaxReadback.LockBuffer(memory))
      {
        ProcessMinMaxReadback(memory);
      }
      m_bMinMaxReadbackInFlight = false;
    }
    else if (result == ezGALAsyncResult::Expired)
    {
      m_bMinMaxReadbackInFlight = false;
    }
  }
}

void ezRenderGraphPassObserver::ProcessPixelReadback(ezArrayPtr<const ezUInt8> memory)
{
  EZ_ASSERT_DEBUG(memory.GetCount() >= sizeof(ezVec4), "Readback size missmatch");

  ezVec4 pixelValue;
  ezMemoryUtils::RawByteCopy(&pixelValue, memory.GetPtr(), sizeof(ezVec4));

  EZ_LOCK(m_Mutex);
  m_Response.m_PixelValue = pixelValue;
}

void ezRenderGraphPassObserver::ProcessHistogramReadback(ezArrayPtr<const ezUInt8> memory)
{
  constexpr ezUInt32 uiRequiredSize = s_uiHistogramBinCount * s_uiHistogramChannelCount * sizeof(ezUInt32);
  EZ_ASSERT_DEBUG(memory.GetCount() >= uiRequiredSize, "Readback size missmatch");

  const ezUInt32* pHistogram = reinterpret_cast<const ezUInt32*>(memory.GetPtr());
  bool bIgnoreChannel[s_uiHistogramChannelCount] = {};
  ezUInt32 uiMaxCount = 0;

  for (ezUInt32 uiChannel = 0; uiChannel < s_uiHistogramChannelCount; ++uiChannel)
  {
    if ((m_Request.m_uiChannelMask & EZ_BIT(uiChannel)) == 0)
    {
      bIgnoreChannel[uiChannel] = true;
      continue;
    }

    ezUInt32 uiNonZeroBuckets = 0;
    ezUInt32 uiChannelMaxCount = 0;

    for (ezUInt32 uiBucket = 0; uiBucket < s_uiHistogramBinCount; ++uiBucket)
    {
      const ezUInt32 uiCount = pHistogram[uiChannel * s_uiHistogramBinCount + uiBucket];
      if (uiCount > 0)
      {
        ++uiNonZeroBuckets;
        uiChannelMaxCount = ezMath::Max(uiChannelMaxCount, uiCount);
      }
    }

    bIgnoreChannel[uiChannel] = uiNonZeroBuckets <= 1;
    if (!bIgnoreChannel[uiChannel])
    {
      uiMaxCount = ezMath::Max(uiMaxCount, uiChannelMaxCount);
    }
  }

  EZ_LOCK(m_Mutex);
  m_Response.m_bHistogramValid = true;

  if (uiMaxCount == 0)
  {
    ezMemoryUtils::ZeroFill(m_Response.m_Histogram.GetData(), m_Response.m_Histogram.GetCount());
    return;
  }

  for (ezUInt32 uiChannel = 0; uiChannel < s_uiHistogramChannelCount; ++uiChannel)
  {
    const ezUInt32 uiChannelOffset = uiChannel * s_uiHistogramBinCount;
    if (bIgnoreChannel[uiChannel])
    {
      ezMemoryUtils::ZeroFill(m_Response.m_Histogram.GetData() + uiChannelOffset, s_uiHistogramBinCount);
      continue;
    }

    for (ezUInt32 uiBucket = 0; uiBucket < s_uiHistogramBinCount; ++uiBucket)
    {
      const ezUInt64 uiNormalizedValue = (static_cast<ezUInt64>(pHistogram[uiChannelOffset + uiBucket]) * 255ull) / uiMaxCount;
      m_Response.m_Histogram[uiChannelOffset + uiBucket] = static_cast<ezUInt8>(ezMath::Min<ezUInt64>(255, uiNormalizedValue));
    }
  }
}

void ezRenderGraphPassObserver::ProcessMinMaxReadback(ezArrayPtr<const ezUInt8> memory)
{
  constexpr ezUInt32 uiRequiredSize = s_uiMinMaxValueCount * sizeof(ezUInt32);
  EZ_ASSERT_DEBUG(memory.GetCount() >= uiRequiredSize, "Readback size missmatch");

  const ezUInt32* pMinMax = reinterpret_cast<const ezUInt32*>(memory.GetPtr());
  float fMin = 0.0f;
  float fMax = 1.0f;

  if (pMinMax[0] != ezMath::MaxValue<ezUInt32>() || pMinMax[1] != 0)
  {
    fMin = OrderedUintToFloat(pMinMax[0]);
    fMax = OrderedUintToFloat(pMinMax[1]);
  }

  EZ_LOCK(m_Mutex);
  m_Response.m_fImageMin = fMin;
  m_Response.m_fImageMax = fMax;
}

void ezRenderGraphPassObserver::Reset()
{
  m_bValid = false;
  m_uiSortedPassIndex = 0xFFFFFFFF;
  m_hResolvedSourceTexture.Invalidate();
  m_PreCopyBarriers.Clear();
  m_PostCopyBarriers.Clear();
}

void ezRenderGraphPassObserver::EnsureCopyTexture(const ezGALTextureCreationDescription& srcDesc)
{
  ezGALTextureCreationDescription newDesc = srcDesc;
  newDesc.m_TextureFlags = ezGALTextureUsageFlags::ShaderResource;
  newDesc.m_ResourceAccess.m_bImmutable = false;
  newDesc.m_pExisitingNativeObject = nullptr;

  if (!m_hCopyTexture.IsInvalidated())
  {
    const ezGALTexture* pExisting = m_pDevice->GetTexture(m_hCopyTexture);
    if (pExisting != nullptr && newDesc.CalculateHash() == pExisting->GetDescription().CalculateHash())
    {
      return; // Already matches.
    }

    // Release old resource. The resource manager will clean up the GAL texture.
    m_hCopyTextureResource.Invalidate();
    m_hCopyTexture.Invalidate();
  }

  ezStringBuilder resourceName;
  resourceName.SetFormat("RGObserverCopy_{}", s_uiObserverTextureCounter.Increment());

  ezTexture2DResourceDescriptor texDesc;
  texDesc.m_DescGAL = newDesc;
  texDesc.m_SamplerDesc.m_MinFilter = ezGALTextureFilterMode::Point;
  texDesc.m_SamplerDesc.m_MagFilter = ezGALTextureFilterMode::Point;
  texDesc.m_SamplerDesc.m_MipFilter = ezGALTextureFilterMode::Point;

  m_hCopyTextureResource = ezResourceManager::CreateResource<ezTexture2DResource>(resourceName, std::move(texDesc), "Render Graph Observer Copy");

  ezResourceLock<ezTexture2DResource> pResource(m_hCopyTextureResource, ezResourceAcquireMode::BlockTillLoaded);
  m_hCopyTexture = pResource->GetGALTexture();
}

void ezRenderGraphPassObserver::RecordPreview(ezRenderGraph& ref_graph)
{
  if (m_pGraph == nullptr || m_Request.m_sPassName.IsEmpty())
    return;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALTextureHandle hCopyTexture = GetCopyTexture();
  if (hCopyTexture.IsInvalidated())
    return;
  const ezGALTexture* pCopyTexture = pDevice->GetTexture(hCopyTexture);
  if (pCopyTexture == nullptr)
    return;

  PollReadbacks();
  EnsureInspectionResources();

  const ezGALTextureCreationDescription& copyTextureDesc = pCopyTexture->GetDescription();
  const ezUInt32 uiMipLevel = ezMath::Min<ezUInt32>(m_Request.m_uiMipLevel, copyTextureDesc.m_uiMipLevelCount - 1);
  const ezUInt32 uiArraySlice = ezMath::Min<ezUInt32>(m_Request.m_uiArraySlice, copyTextureDesc.GetNumberOfSlices() - 1);
  const ezVec3U32 mipSize = copyTextureDesc.GetMipMapSize(uiMipLevel);
  const bool bIsMSAA = copyTextureDesc.m_SampleCount != ezGALMSAASampleCount::None;
  const ezUInt32 uiSampleCount = (ezUInt32)copyTextureDesc.m_SampleCount;
  const ezInt32 iSampleIndex = m_Request.m_iSampleIndex < 0 ? -1 : ezMath::Min<ezInt32>(m_Request.m_iSampleIndex, uiSampleCount - 1u);

  ezGALTextureRange sourceRange;
  sourceRange.m_uiBaseMipLevel = uiMipLevel;
  sourceRange.m_uiMipLevels = 1;
  sourceRange.m_uiBaseArraySlice = uiArraySlice;
  sourceRange.m_uiArraySlices = 1;

  ezRenderGraphTextureHandle hSource = ref_graph.ImportTexture(hCopyTexture);
  ezRenderGraphBufferHandle hPixelBuffer = ref_graph.ImportBuffer(m_hPixelReadbackBuffer);
  ezRenderGraphBufferHandle hHistogramBuffer = ref_graph.ImportBuffer(m_hHistogramBuffer);
  ezRenderGraphBufferHandle hMinMaxBuffer = ref_graph.ImportBuffer(m_hMinMaxBuffer);

  if (!m_bPixelReadbackInFlight && m_Request.m_vPixelPosition.x >= 0 && m_Request.m_vPixelPosition.y >= 0)
  {
    ezRenderGraphReadbackPixelConstants constants;
    constants.PixelPosition = m_Request.m_vPixelPosition;
    constants.SampleIndex = iSampleIndex;
    constants.SampleCount = uiSampleCount;
    constants.TextureSize = ezVec2U32(mipSize.x, mipSize.y);

    {
      auto pass = ref_graph.AddComputePass("RenderGraphObserver Read Pixel");
      pass.ReadTexture(hSource, sourceRange);
      pass.WriteBuffer(hPixelBuffer);
      pass.SetExecuteCallback([this, hSource, hPixelBuffer, sourceRange, constants, bIsMSAA](const ezRenderGraphContext& ctx)
        {
        ezRenderContext* pContext = ctx.GetRenderContext();
        pContext->SetShaderPermutationVariable("MSAA", bIsMSAA ? ezTempHashedString("TRUE") : ezTempHashedString("FALSE"));
        pContext->BindShader(m_hReadbackPixelShader);
        pContext->SetPushConstants("ezRenderGraphReadbackPixelConstants", constants);
        ezBindGroupBuilder& bindGroup = pContext->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
        bindGroup.BindTexture("SourceTexture", ctx.ResolveTexture(hSource), sourceRange, {}, ezGALTextureType::Texture2D);
        bindGroup.BindBuffer("PixelOutput", ctx.ResolveBuffer(hPixelBuffer));
        pContext->Dispatch(1, 1, 1).AssertSuccess(); });
    }
    {
      auto readbackPass = ref_graph.AddTransferPass("RenderGraphObserver Pixel Readback");
      readbackPass.ReadBuffer(hPixelBuffer, ezGALResourceState::CopySource);
      readbackPass.HasSideEffects();
      readbackPass.SetExecuteCallback([this, hPixelBuffer](const ezRenderGraphContext& ctx)
        { m_PixelReadback.ReadbackBuffer(*ctx.GetCommandEncoder(), ctx.ResolveBuffer(hPixelBuffer)); });
    }

    m_bPixelReadbackInFlight = true;
  }

  if (!m_bMinMaxReadbackInFlight)
  {
    ezRenderGraphMinMaxConstants constants;
    constants.TextureSize = ezVec2U32(mipSize.x, mipSize.y);
    constants.SampleIndex = iSampleIndex;
    constants.SampleCount = uiSampleCount;
    constants.ChannelMask = m_Request.m_uiChannelMask;

    {
      auto pass = ref_graph.AddComputePass("RenderGraphObserver Clear MinMax");
      pass.WriteBuffer(hMinMaxBuffer);
      pass.HasSideEffects();
      pass.SetExecuteCallback([this, hMinMaxBuffer](const ezRenderGraphContext& ctx)
        {
        ezRenderContext* pContext = ctx.GetRenderContext();
        pContext->BindShader(m_hClearMinMaxShader);
        ezBindGroupBuilder& bindGroup = pContext->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
        bindGroup.BindBuffer("MinMaxOutput", ctx.ResolveBuffer(hMinMaxBuffer));
        pContext->Dispatch(1, 1, 1).AssertSuccess(); });
    }

    {
      auto pass = ref_graph.AddComputePass("RenderGraphObserver Build MinMax");
      pass.ReadTexture(hSource, sourceRange);
      pass.WriteBuffer(hMinMaxBuffer);
      pass.SetExecuteCallback([this, hSource, hMinMaxBuffer, sourceRange, constants, bIsMSAA](const ezRenderGraphContext& ctx)
        {
        ezRenderContext* pContext = ctx.GetRenderContext();
        pContext->SetShaderPermutationVariable("MSAA", bIsMSAA ? ezTempHashedString("TRUE") : ezTempHashedString("FALSE"));
        pContext->BindShader(m_hBuildMinMaxShader);
        pContext->SetPushConstants("ezRenderGraphMinMaxConstants", constants);
        ezBindGroupBuilder& bindGroup = pContext->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
        bindGroup.BindTexture("SourceTexture", ctx.ResolveTexture(hSource), sourceRange, ezGALResourceFormat::Invalid, ezGALTextureType::Texture2D);
        bindGroup.BindBuffer("MinMaxOutput", ctx.ResolveBuffer(hMinMaxBuffer));
        pContext->Dispatch((constants.TextureSize.x + 7u) / 8u, (constants.TextureSize.y + 7u) / 8u, 1).AssertSuccess(); });
    }

    {
      auto readbackPass = ref_graph.AddTransferPass("RenderGraphObserver MinMax Readback");
      readbackPass.ReadBuffer(hMinMaxBuffer, ezGALResourceState::CopySource);
      readbackPass.HasSideEffects();
      readbackPass.SetExecuteCallback([this, hMinMaxBuffer](const ezRenderGraphContext& ctx)
        { m_MinMaxReadback.ReadbackBuffer(*ctx.GetCommandEncoder(), ctx.ResolveBuffer(hMinMaxBuffer)); });
    }
    m_bMinMaxReadbackInFlight = true;
  }

  if (!m_bHistogramReadbackInFlight)
  {
    ezRenderGraphHistogramConstants constants;
    constants.TextureSize = ezVec2U32(mipSize.x, mipSize.y);
    constants.SampleIndex = iSampleIndex;
    constants.SampleCount = uiSampleCount;
    constants.ChannelMask = m_Request.m_uiChannelMask;
    constants.RangeMin = m_Request.m_fRangeMin;
    constants.RangeMax = GetAdjustedHistogramMax(m_Request.m_fRangeMin, m_Request.m_fRangeMax);

    {
      auto pass = ref_graph.AddComputePass("RenderGraphObserver Clear Histogram");
      pass.WriteBuffer(hHistogramBuffer);
      pass.HasSideEffects();
      pass.SetExecuteCallback([this, hHistogramBuffer](const ezRenderGraphContext& ctx)
        {
        ezRenderContext* pContext = ctx.GetRenderContext();
        pContext->BindShader(m_hClearHistogramShader);
        ezBindGroupBuilder& bindGroup = pContext->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
        bindGroup.BindBuffer("HistogramOutput", ctx.ResolveBuffer(hHistogramBuffer));
        pContext->Dispatch(1, 1, 1).AssertSuccess(); });
    }

    {
      auto pass = ref_graph.AddComputePass("RenderGraphObserver Build Histogram");
      pass.ReadTexture(hSource, sourceRange);
      pass.WriteBuffer(hHistogramBuffer);
      pass.SetExecuteCallback([this, hSource, hHistogramBuffer, sourceRange, constants, bIsMSAA](const ezRenderGraphContext& ctx)
        {
        ezRenderContext* pContext = ctx.GetRenderContext();
        pContext->SetShaderPermutationVariable("MSAA", bIsMSAA ? ezTempHashedString("TRUE") : ezTempHashedString("FALSE"));
        pContext->BindShader(m_hBuildHistogramShader);
        pContext->SetPushConstants("ezRenderGraphHistogramConstants", constants);
        ezBindGroupBuilder& bindGroup = pContext->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
        bindGroup.BindTexture("SourceTexture", ctx.ResolveTexture(hSource), sourceRange, ezGALResourceFormat::Invalid, ezGALTextureType::Texture2D);
        bindGroup.BindBuffer("HistogramOutput", ctx.ResolveBuffer(hHistogramBuffer));
        pContext->Dispatch((constants.TextureSize.x + 7u) / 8u, (constants.TextureSize.y + 7u) / 8u, 1).AssertSuccess(); });
    }

    {
      auto readbackPass = ref_graph.AddTransferPass("RenderGraphObserver Histogram Readback");
      readbackPass.ReadBuffer(hHistogramBuffer, ezGALResourceState::CopySource);
      readbackPass.HasSideEffects();
      readbackPass.SetExecuteCallback([this, hHistogramBuffer](const ezRenderGraphContext& ctx)
        { m_HistogramReadback.ReadbackBuffer(*ctx.GetCommandEncoder(), ctx.ResolveBuffer(hHistogramBuffer)); });
    }
    m_bHistogramReadbackInFlight = true;
  }

  const ezGALSwapChain* pSwapChain = pDevice->GetSwapChain(m_hSwapChain);
  if (!pSwapChain)
    return;

  ezGALTextureHandle hBackbuffer = pSwapChain->GetBackBufferTexture();
  const ezGALTexture* pBackbuffer = pDevice->GetTexture(hBackbuffer);
  if (!pBackbuffer)
    return;

  const ezGALTextureCreationDescription& desc = pBackbuffer->GetDescription();
  ezRenderGraphTextureHandle hTarget = ref_graph.ImportTexture(hBackbuffer);

  const float fZoom = ezMath::Clamp(m_Request.m_fZoom, 0.1f, 64.0f);
  const float fTexAspect = (float)mipSize.x / (float)ezMath::Max(mipSize.y, 1u);
  const float fViewAspect = (float)desc.m_uiWidth / (float)ezMath::Max(desc.m_uiHeight, 1u);

  float fHalfExtentU = 0.5f;
  float fHalfExtentV = 0.5f;
  if (fTexAspect > fViewAspect)
  {
    fHalfExtentU = 0.5f / fZoom;
    fHalfExtentV = 0.5f / fZoom * (fTexAspect / fViewAspect);
  }
  else
  {
    fHalfExtentU = 0.5f / fZoom * (fViewAspect / fTexAspect);
    fHalfExtentV = 0.5f / fZoom;
  }

  const ezVec2 uv0(m_Request.m_vPanCenter.x - fHalfExtentU, m_Request.m_vPanCenter.y - fHalfExtentV);
  const ezVec2 uv1(m_Request.m_vPanCenter.x + fHalfExtentU, m_Request.m_vPanCenter.y + fHalfExtentV);

  ezRenderGraphPreviewConstants previewConstants;
  previewConstants.UVTransform = ezVec4(uv1.x - uv0.x, uv1.y - uv0.y, uv0.x, uv0.y);
  previewConstants.ValueRange = ezVec2(m_Request.m_fRangeMin, m_Request.m_fRangeMax);
  previewConstants.ChannelMask = m_Request.m_uiChannelMask;
  previewConstants.SampleIndex = iSampleIndex;
  previewConstants.PixelPosition = m_Request.m_vPixelPosition;
  previewConstants.HighlightPixel = m_Request.m_bHighlightPixel ? 1 : 0;

  {
    auto pass = ref_graph.AddGraphicsPass("RenderGraphObserver Preview");
    pass.AddColorTarget(hTarget);
    pass.ReadTexture(hSource, sourceRange, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
    pass.SetExecuteCallback([this, hSource, sourceRange, previewConstants, bIsMSAA](const ezRenderGraphContext& ctx)
      {
        ezRenderContext* pContext = ctx.GetRenderContext();
        pContext->SetShaderPermutationVariable("MSAA", bIsMSAA ? ezTempHashedString("TRUE") : ezTempHashedString("FALSE"));

        pContext->BindShader(m_hPreviewShader);
        pContext->SetPushConstants("ezRenderGraphPreviewConstants", previewConstants);
        pContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);

        ezBindGroupBuilder& bindGroup = pContext->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
        bindGroup.BindTexture("SourceTexture", ctx.ResolveTexture(hSource), sourceRange, ezGALResourceFormat::Invalid, ezGALTextureType::Texture2D);

        pContext->DrawMeshBuffer().AssertSuccess(); //
      });
  }
}
