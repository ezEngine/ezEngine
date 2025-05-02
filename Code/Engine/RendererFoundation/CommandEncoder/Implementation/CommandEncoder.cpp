#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/ReadbackBuffer.h>
#include <RendererFoundation/Resources/ReadbackTexture.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/ResourceView.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Resources/UnorderedAccesView.h>

void ezGALCommandEncoder::SetConstantBuffer(const ezShaderResourceBinding& binding, ezGALBufferHandle hBuffer)
{
  AssertRenderingThread();

  const ezGALBuffer* pBuffer = m_Device.GetBuffer(hBuffer);
  EZ_ASSERT_DEV(pBuffer == nullptr || pBuffer->GetDescription().m_BufferFlags.IsSet(ezGALBufferUsageFlags::ConstantBuffer), "Wrong buffer type");

  m_CommonImpl.SetConstantBufferPlatform(binding, pBuffer);
}

void ezGALCommandEncoder::SetSamplerState(const ezShaderResourceBinding& binding, ezGALSamplerStateHandle hSamplerState)
{
  AssertRenderingThread();

  const ezGALSamplerState* pSamplerState = m_Device.GetSamplerState(hSamplerState);

  m_CommonImpl.SetSamplerStatePlatform(binding, pSamplerState);
}

void ezGALCommandEncoder::SetResourceView(const ezShaderResourceBinding& binding, ezGALTextureResourceViewHandle hResourceView)
{
  AssertRenderingThread();

  const ezGALTextureResourceView* pResourceView = m_Device.GetResourceView(hResourceView);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  if (pResourceView != nullptr)
  {
    const ezGALTexture* pTexture = pResourceView->GetResource();
    const ezGALTextureType::Enum type = (pResourceView->GetDescription().m_OverrideViewType != ezGALTextureType::Invalid) ? pResourceView->GetDescription().m_OverrideViewType : pTexture->GetDescription().m_Type;
    const bool bMSAA = pTexture->GetDescription().m_SampleCount != ezGALMSAASampleCount::None;

    switch (type)
    {
      case ezGALTextureType::Texture2D:
      case ezGALTextureType::Texture2DProxy:
      case ezGALTextureType::Texture2DShared:
        EZ_ASSERT_DEBUG((binding.m_TextureType == ezGALShaderTextureType::Texture2D && !bMSAA) || (binding.m_TextureType == ezGALShaderTextureType::Texture2DMS && bMSAA), "Mismatch between shader resource and bound view.");
        break;

      case ezGALTextureType::TextureCube:
        EZ_ASSERT_DEBUG((binding.m_TextureType == ezGALShaderTextureType::TextureCube && !bMSAA), "Mismatch between shader resource and bound view.");
        break;

      case ezGALTextureType::Texture3D:
        EZ_ASSERT_DEBUG((binding.m_TextureType == ezGALShaderTextureType::Texture3D && !bMSAA), "Mismatch between shader resource and bound view.");
        break;

      case ezGALTextureType::Texture2DArray:
        EZ_ASSERT_DEBUG((binding.m_TextureType == ezGALShaderTextureType::Texture2DArray && !bMSAA) || (binding.m_TextureType == ezGALShaderTextureType::Texture2DMSArray && bMSAA), "Mismatch between shader resource and bound view.");
        break;

      case ezGALTextureType::TextureCubeArray:
        EZ_ASSERT_DEBUG((binding.m_TextureType == ezGALShaderTextureType::TextureCubeArray && !bMSAA), "Mismatch between shader resource and bound view.");
        break;
      default:
        break;
    }
  }
#endif
  m_CommonImpl.SetResourceViewPlatform(binding, pResourceView);
}

void ezGALCommandEncoder::SetResourceView(const ezShaderResourceBinding& binding, ezGALBufferResourceViewHandle hResourceView)
{
  AssertRenderingThread();

  const ezGALBufferResourceView* pResourceView = m_Device.GetResourceView(hResourceView);

  m_CommonImpl.SetResourceViewPlatform(binding, pResourceView);
}

void ezGALCommandEncoder::SetUnorderedAccessView(const ezShaderResourceBinding& binding, ezGALTextureUnorderedAccessViewHandle hUnorderedAccessView)
{
  AssertRenderingThread();

  const ezGALTextureUnorderedAccessView* pUnorderedAccessView = m_Device.GetUnorderedAccessView(hUnorderedAccessView);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  if (pUnorderedAccessView != nullptr)
  {
    const ezGALTexture* pTexture = pUnorderedAccessView->GetResource();
    const ezGALTextureType::Enum type = (pUnorderedAccessView->GetDescription().m_OverrideViewType != ezGALTextureType::Invalid) ? pUnorderedAccessView->GetDescription().m_OverrideViewType : pTexture->GetDescription().m_Type;
    const bool bMSAA = pTexture->GetDescription().m_SampleCount != ezGALMSAASampleCount::None;

    switch (type)
    {
      case ezGALTextureType::Texture2D:
      case ezGALTextureType::Texture2DProxy:
      case ezGALTextureType::Texture2DShared:
        EZ_ASSERT_DEBUG((binding.m_TextureType == ezGALShaderTextureType::Texture2D && !bMSAA) || (binding.m_TextureType == ezGALShaderTextureType::Texture2DMS && bMSAA), "Mismatch between shader resource and bound view.");
        break;

      case ezGALTextureType::Texture3D:
        EZ_ASSERT_DEBUG((binding.m_TextureType == ezGALShaderTextureType::Texture3D && !bMSAA), "Mismatch between shader resource and bound view.");
        break;

      case ezGALTextureType::Texture2DArray:
        EZ_ASSERT_DEBUG((binding.m_TextureType == ezGALShaderTextureType::Texture2DArray && !bMSAA) || (binding.m_TextureType == ezGALShaderTextureType::Texture2DMSArray && bMSAA), "Mismatch between shader resource and bound view.");
        break;

      default:
        EZ_REPORT_FAILURE("Unsupported UAV type");
    }
  }
#endif

  m_CommonImpl.SetUnorderedAccessViewPlatform(binding, pUnorderedAccessView);
}

void ezGALCommandEncoder::SetUnorderedAccessView(const ezShaderResourceBinding& binding, ezGALBufferUnorderedAccessViewHandle hUnorderedAccessView)
{
  AssertRenderingThread();

  const ezGALBufferUnorderedAccessView* pUnorderedAccessView = m_Device.GetUnorderedAccessView(hUnorderedAccessView);
  m_CommonImpl.SetUnorderedAccessViewPlatform(binding, pUnorderedAccessView);
}

void ezGALCommandEncoder::SetPushConstants(ezArrayPtr<const ezUInt8> data)
{
  AssertRenderingThread();
  m_CommonImpl.SetPushConstantsPlatform(data);
}

ezGALTimestampHandle ezGALCommandEncoder::InsertTimestamp()
{
  AssertRenderingThread();
  return m_CommonImpl.InsertTimestampPlatform();
}

ezGALOcclusionHandle ezGALCommandEncoder::BeginOcclusionQuery(ezEnum<ezGALQueryType> type)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Occlusion queries can only be started within a render scope");
  AssertRenderingThread();
  ezGALOcclusionHandle hOcclusion = m_CommonImpl.BeginOcclusionQueryPlatform(type);

  EZ_ASSERT_DEBUG(m_hPendingOcclusionQuery.IsInvalidated(), "Only one occusion query can be active at any give time.");
  m_hPendingOcclusionQuery = hOcclusion;

  return hOcclusion;
}

void ezGALCommandEncoder::EndOcclusionQuery(ezGALOcclusionHandle hOcclusion)
{
  AssertRenderingThread();
  m_CommonImpl.EndOcclusionQueryPlatform(hOcclusion);

  EZ_ASSERT_DEBUG(m_hPendingOcclusionQuery == hOcclusion, "The EndOcclusionQuery parameter does not match the currently started query");
  m_hPendingOcclusionQuery = {};
}

ezGALFenceHandle ezGALCommandEncoder::InsertFence()
{
  return m_CommonImpl.InsertFencePlatform();
}

void ezGALCommandEncoder::ClearUnorderedAccessView(ezGALTextureUnorderedAccessViewHandle hUnorderedAccessView, ezVec4 vClearValues)
{
  AssertRenderingThread();

  const ezGALTextureUnorderedAccessView* pUnorderedAccessView = m_Device.GetUnorderedAccessView(hUnorderedAccessView);
  if (pUnorderedAccessView == nullptr)
  {
    EZ_REPORT_FAILURE("ClearUnorderedAccessView failed, unordered access view handle invalid.");
    return;
  }

  m_CommonImpl.ClearUnorderedAccessViewPlatform(pUnorderedAccessView, vClearValues);
}

void ezGALCommandEncoder::ClearUnorderedAccessView(ezGALBufferUnorderedAccessViewHandle hUnorderedAccessView, ezVec4 vClearValues)
{
  AssertRenderingThread();

  const ezGALBufferUnorderedAccessView* pUnorderedAccessView = m_Device.GetUnorderedAccessView(hUnorderedAccessView);
  if (pUnorderedAccessView == nullptr)
  {
    EZ_REPORT_FAILURE("ClearUnorderedAccessView failed, unordered access view handle invalid.");
    return;
  }

  m_CommonImpl.ClearUnorderedAccessViewPlatform(pUnorderedAccessView, vClearValues);
}

void ezGALCommandEncoder::ClearUnorderedAccessView(ezGALTextureUnorderedAccessViewHandle hUnorderedAccessView, ezVec4U32 vClearValues)
{
  AssertRenderingThread();

  const ezGALTextureUnorderedAccessView* pUnorderedAccessView = m_Device.GetUnorderedAccessView(hUnorderedAccessView);
  if (pUnorderedAccessView == nullptr)
  {
    EZ_REPORT_FAILURE("ClearUnorderedAccessView failed, unordered access view handle invalid.");
    return;
  }

  m_CommonImpl.ClearUnorderedAccessViewPlatform(pUnorderedAccessView, vClearValues);
}

void ezGALCommandEncoder::ClearUnorderedAccessView(ezGALBufferUnorderedAccessViewHandle hUnorderedAccessView, ezVec4U32 vClearValues)
{
  AssertRenderingThread();

  const ezGALBufferUnorderedAccessView* pUnorderedAccessView = m_Device.GetUnorderedAccessView(hUnorderedAccessView);
  if (pUnorderedAccessView == nullptr)
  {
    EZ_REPORT_FAILURE("ClearUnorderedAccessView failed, unordered access view handle invalid.");
    return;
  }

  m_CommonImpl.ClearUnorderedAccessViewPlatform(pUnorderedAccessView, vClearValues);
}

void ezGALCommandEncoder::CopyBuffer(ezGALBufferHandle hDest, ezGALBufferHandle hSource)
{
  AssertRenderingThread();
  AssertOutsideRenderingScope();

  const ezGALBuffer* pDest = m_Device.GetBuffer(hDest);
  const ezGALBuffer* pSource = m_Device.GetBuffer(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    m_CommonImpl.CopyBufferPlatform(pDest, pSource);
  }
  else
  {
    EZ_REPORT_FAILURE("CopyBuffer failed, buffer handle invalid - destination = {0}, source = {1}", ezArgP(pDest), ezArgP(pSource));
  }
}

void ezGALCommandEncoder::CopyBufferRegion(
  ezGALBufferHandle hDest, ezUInt32 uiDestOffset, ezGALBufferHandle hSource, ezUInt32 uiSourceOffset, ezUInt32 uiByteCount)
{
  AssertRenderingThread();
  AssertOutsideRenderingScope();

  const ezGALBuffer* pDest = m_Device.GetBuffer(hDest);
  const ezGALBuffer* pSource = m_Device.GetBuffer(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    const ezUInt32 uiDestSize = pDest->GetSize();
    const ezUInt32 uiSourceSize = pSource->GetSize();

    EZ_IGNORE_UNUSED(uiDestSize);
    EZ_ASSERT_DEV(uiDestSize >= uiDestOffset + uiByteCount, "Destination buffer too small (or offset too big)");
    EZ_IGNORE_UNUSED(uiSourceSize);
    EZ_ASSERT_DEV(uiSourceSize >= uiSourceOffset + uiByteCount, "Source buffer too small (or offset too big)");

    m_CommonImpl.CopyBufferRegionPlatform(pDest, uiDestOffset, pSource, uiSourceOffset, uiByteCount);
  }
  else
  {
    EZ_REPORT_FAILURE("CopyBuffer failed, buffer handle invalid - destination = {0}, source = {1}", ezArgP(pDest), ezArgP(pSource));
  }
}

void ezGALCommandEncoder::GALStaticDeviceEventHandler(const ezGALDeviceEvent& e)
{
  EZ_IGNORE_UNUSED(e);
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  if (e.m_Type == ezGALDeviceEvent::BeforeBeginFrame)
    m_BufferUpdates.Clear();
#endif
}

void ezGALCommandEncoder::UpdateBuffer(ezGALBufferHandle hDest, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> sourceData, ezGALUpdateMode::Enum updateMode)
{
  AssertRenderingThread();
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType != CommandEncoderType::Render || updateMode == ezGALUpdateMode::TransientConstantBuffer || updateMode == ezGALUpdateMode::AheadOfTime, "Only discard updates on dynamic buffers are supported within a render scope");

  EZ_ASSERT_DEV(!sourceData.IsEmpty(), "Source data for buffer update is invalid!");

  const ezGALBuffer* pDest = m_Device.GetBuffer(hDest);

  if (pDest != nullptr)
  {
    EZ_ASSERT_DEV(pDest->GetSize() >= (uiDestOffset + sourceData.GetCount()), "Buffer {} is too small (or offset {} too big) for {} bytes", pDest->GetSize(), uiDestOffset, sourceData.GetCount());

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (updateMode == ezGALUpdateMode::AheadOfTime)
    {
      auto it = m_BufferUpdates.Find(hDest);
      if (!it.IsValid())
      {
        it = m_BufferUpdates.Insert(hDest, ezHybridArray<BufferRange, 1>());
      }
      ezHybridArray<BufferRange, 1>& ranges = it.Value();
      for (const BufferRange& range : ranges)
      {
        EZ_ASSERT_DEBUG(!range.overlapRange(uiDestOffset, sourceData.GetCount()), "A buffer was updated twice in one frame on the same memory range.");
      }
      BufferRange* pLastElement = ranges.IsEmpty() ? nullptr : &ranges.PeekBack();
      if (pLastElement && pLastElement->m_uiOffset + pLastElement->m_uiLength == uiDestOffset)
      {
        // In most cases we update the buffer in order so we can compact the write operations to reduce the number of elements we have to loop through.
        pLastElement->m_uiLength += sourceData.GetCount();
      }
      else
      {
        ranges.PushBack({uiDestOffset, sourceData.GetCount()});
      }
    }
#endif
    m_CommonImpl.UpdateBufferPlatform(pDest, uiDestOffset, sourceData, updateMode);
  }
  else
  {
    ezLog::Error("UpdateBuffer failed, buffer handle invalid");
    // EZ_REPORT_FAILURE("UpdateBuffer failed, buffer handle invalid");
  }
}

void ezGALCommandEncoder::CopyTexture(ezGALTextureHandle hDest, ezGALTextureHandle hSource)
{
  AssertRenderingThread();
  AssertOutsideRenderingScope();

  const ezGALTexture* pDest = m_Device.GetTexture(hDest);
  const ezGALTexture* pSource = m_Device.GetTexture(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    m_CommonImpl.CopyTexturePlatform(pDest, pSource);
  }
  else
  {
    EZ_REPORT_FAILURE("CopyTexture failed, texture handle invalid - destination = {0}, source = {1}", ezArgP(pDest), ezArgP(pSource));
  }
}

void ezGALCommandEncoder::CopyTextureRegion(ezGALTextureHandle hDest, const ezGALTextureSubresource& destinationSubResource,
  const ezVec3U32& vDestinationPoint, ezGALTextureHandle hSource, const ezGALTextureSubresource& sourceSubResource, const ezBoundingBoxu32& box)
{
  AssertRenderingThread();
  AssertOutsideRenderingScope();

  const ezGALTexture* pDest = m_Device.GetTexture(hDest);
  const ezGALTexture* pSource = m_Device.GetTexture(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    m_CommonImpl.CopyTextureRegionPlatform(pDest, destinationSubResource, vDestinationPoint, pSource, sourceSubResource, box);
  }
  else
  {
    EZ_REPORT_FAILURE("CopyTextureRegion failed, texture handle invalid - destination = {0}, source = {1}", ezArgP(pDest), ezArgP(pSource));
  }
}

void ezGALCommandEncoder::UpdateTexture(ezGALTextureHandle hDest, const ezGALTextureSubresource& destinationSubResource,
  const ezBoundingBoxu32& destinationBox, const ezGALSystemMemoryDescription& sourceData)
{
  AssertRenderingThread();
  AssertOutsideRenderingScope();

  const ezGALTexture* pDest = m_Device.GetTexture(hDest);

  if (pDest != nullptr)
  {
    m_CommonImpl.UpdateTexturePlatform(pDest, destinationSubResource, destinationBox, sourceData);
  }
  else
  {
    EZ_REPORT_FAILURE("UpdateTexture failed, texture handle invalid - destination = {0}", ezArgP(pDest));
  }
}

void ezGALCommandEncoder::ResolveTexture(ezGALTextureHandle hDest, const ezGALTextureSubresource& destinationSubResource, ezGALTextureHandle hSource,
  const ezGALTextureSubresource& sourceSubResource)
{
  AssertRenderingThread();
  AssertOutsideRenderingScope();

  const ezGALTexture* pDest = m_Device.GetTexture(hDest);
  const ezGALTexture* pSource = m_Device.GetTexture(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    m_CommonImpl.ResolveTexturePlatform(pDest, destinationSubResource, pSource, sourceSubResource);
  }
  else
  {
    EZ_REPORT_FAILURE("ResolveTexture failed, texture handle invalid - destination = {0}, source = {1}", ezArgP(pDest), ezArgP(pSource));
  }
}

void ezGALCommandEncoder::ReadbackTexture(ezGALReadbackTextureHandle hDestination, ezGALTextureHandle hSource)
{
  AssertRenderingThread();
  AssertOutsideRenderingScope();

  const ezGALReadbackTexture* pDestination = m_Device.GetReadbackTexture(hDestination);
  const ezGALTexture* pSource = m_Device.GetTexture(hSource);

  EZ_ASSERT_DEBUG(pDestination != nullptr && pSource != nullptr, "Invalid handle provided");

  const ezGALTextureCreationDescription& sourceDesc = pSource->GetDescription();
  const ezGALTextureCreationDescription& destinationDesc = pDestination->GetDescription();

  bool bMissmatch = sourceDesc.m_uiWidth != destinationDesc.m_uiWidth || sourceDesc.m_uiHeight != destinationDesc.m_uiHeight || sourceDesc.m_uiDepth != destinationDesc.m_uiDepth || sourceDesc.m_uiMipLevelCount != destinationDesc.m_uiMipLevelCount || sourceDesc.m_uiArraySize != destinationDesc.m_uiArraySize || sourceDesc.m_Format != destinationDesc.m_Format || sourceDesc.m_Type != destinationDesc.m_Type || sourceDesc.m_Format != destinationDesc.m_Format || sourceDesc.m_SampleCount != destinationDesc.m_SampleCount;
  EZ_IGNORE_UNUSED(bMissmatch);
  EZ_ASSERT_DEBUG(!bMissmatch, "Source and destination formats do not match");

  if (pDestination != nullptr && pSource != nullptr)
  {
    m_CommonImpl.ReadbackTexturePlatform(pDestination, pSource);
  }
}


void ezGALCommandEncoder::ReadbackBuffer(ezGALReadbackBufferHandle hDestination, ezGALBufferHandle hSource)
{
  AssertRenderingThread();
  AssertOutsideRenderingScope();

  const ezGALReadbackBuffer* pDestination = m_Device.GetReadbackBuffer(hDestination);
  const ezGALBuffer* pSource = m_Device.GetBuffer(hSource);

  EZ_ASSERT_DEBUG(pDestination != nullptr && pSource != nullptr, "Invalid handle provided");
  EZ_ASSERT_DEBUG(pSource->GetDescription().m_uiTotalSize == pDestination->GetDescription().m_uiTotalSize, "Source and destination size do not match");

  if (pDestination != nullptr && pSource != nullptr)
  {
    m_CommonImpl.ReadbackBufferPlatform(pDestination, pSource);
  }
}

void ezGALCommandEncoder::GenerateMipMaps(ezGALTextureResourceViewHandle hResourceView)
{
  AssertRenderingThread();
  AssertOutsideRenderingScope();

  const ezGALTextureResourceView* pResourceView = m_Device.GetResourceView(hResourceView);
  if (pResourceView != nullptr)
  {
    EZ_ASSERT_DEV(!pResourceView->GetDescription().m_hTexture.IsInvalidated(), "Resource view needs a valid texture to generate mip maps.");
    const ezGALTexture* pTexture = m_Device.GetTexture(pResourceView->GetDescription().m_hTexture);
    EZ_IGNORE_UNUSED(pTexture);
    EZ_ASSERT_DEV(pTexture->GetDescription().m_bAllowDynamicMipGeneration,
      "Dynamic mip map generation needs to be enabled (m_bAllowDynamicMipGeneration = true)!");

    m_CommonImpl.GenerateMipMapsPlatform(pResourceView);
  }
}

void ezGALCommandEncoder::Flush()
{
  AssertRenderingThread();
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType != CommandEncoderType::Render, "Flush can't be called inside a rendering scope");

  m_CommonImpl.FlushPlatform();
}

// Debug helper functions

void ezGALCommandEncoder::PushMarker(const char* szMarker)
{
  AssertRenderingThread();

  EZ_ASSERT_DEV(szMarker != nullptr, "Invalid marker!");

  m_CommonImpl.PushMarkerPlatform(szMarker);
}

void ezGALCommandEncoder::PopMarker()
{
  AssertRenderingThread();

  m_CommonImpl.PopMarkerPlatform();
}

void ezGALCommandEncoder::InsertEventMarker(const char* szMarker)
{
  AssertRenderingThread();

  EZ_ASSERT_DEV(szMarker != nullptr, "Invalid marker!");

  m_CommonImpl.InsertEventMarkerPlatform(szMarker);
}

ezGALCommandEncoder::ezGALCommandEncoder(ezGALDevice& ref_device, ezGALCommandEncoderCommonPlatformInterface& ref_commonImpl)
  : m_Device(ref_device)
  , m_CommonImpl(ref_commonImpl)
{
  ezGALDevice::s_Events.AddEventHandler(ezMakeDelegate(&ezGALCommandEncoder::GALStaticDeviceEventHandler, this));
}

ezGALCommandEncoder::~ezGALCommandEncoder()
{
  ezGALDevice::s_Events.RemoveEventHandler(ezMakeDelegate(&ezGALCommandEncoder::GALStaticDeviceEventHandler, this));
}

void ezGALCommandEncoder::InvalidateState()
{
  m_State.InvalidateState();
}

ezResult ezGALCommandEncoder::Dispatch(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Compute, "Call BeginCompute first");
  AssertRenderingThread();

  EZ_ASSERT_DEBUG(uiThreadGroupCountX > 0 && uiThreadGroupCountY > 0 && uiThreadGroupCountZ > 0, "Thread group counts of zero are not meaningful. Did you mean 1?");

  CountDispatchCall();
  return m_CommonImpl.DispatchPlatform(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

ezResult ezGALCommandEncoder::DispatchIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Compute, "Call BeginCompute first");
  AssertRenderingThread();

  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  EZ_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");

  CountDispatchCall();
  return m_CommonImpl.DispatchIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);
}

void ezGALCommandEncoder::ClearStatisticsCounters()
{
  m_uiDrawCalls = 0;
  m_uiDispatchCalls = 0;
}

void ezGALCommandEncoder::Clear(const ezColor& clearColor, ezUInt32 uiRenderTargetClearMask /*= 0xFFFFFFFFu*/, bool bClearDepth /*= true*/, bool bClearStencil /*= true*/, float fDepthClear /*= 1.0f*/, ezUInt8 uiStencilClear /*= 0x0u*/)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();
  m_CommonImpl.ClearPlatform(clearColor, uiRenderTargetClearMask, bClearDepth, bClearStencil, fDepthClear, uiStencilClear);
}

ezResult ezGALCommandEncoder::Draw(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();
  CountDrawCall();
  return m_CommonImpl.DrawPlatform(uiVertexCount, uiStartVertex);
}

ezResult ezGALCommandEncoder::DrawIndexed(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();
  CountDrawCall();
  return m_CommonImpl.DrawIndexedPlatform(uiIndexCount, uiStartIndex);
}

ezResult ezGALCommandEncoder::DrawIndexedInstanced(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();
  CountDrawCall();
  return m_CommonImpl.DrawIndexedInstancedPlatform(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex);
}

ezResult ezGALCommandEncoder::DrawIndexedInstancedIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();
  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  EZ_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");
  CountDrawCall();
  return m_CommonImpl.DrawIndexedInstancedIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);
}

ezResult ezGALCommandEncoder::DrawInstanced(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();
  CountDrawCall();
  return m_CommonImpl.DrawInstancedPlatform(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex);
}

ezResult ezGALCommandEncoder::DrawInstancedIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();
  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  EZ_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");
  CountDrawCall();
  return m_CommonImpl.DrawInstancedIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);
}

void ezGALCommandEncoder::SetIndexBuffer(ezGALBufferHandle hIndexBuffer)
{
  if (m_State.m_hIndexBuffer == hIndexBuffer)
  {
    return;
  }

  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hIndexBuffer);
  /// \todo Assert on index buffer type (if non nullptr)
  // Note that GL4 can bind arbitrary buffer to arbitrary binding points (index/vertex/transform-feedback/indirect-draw/...)

  m_CommonImpl.SetIndexBufferPlatform(pBuffer);

  m_State.m_hIndexBuffer = hIndexBuffer;
}

void ezGALCommandEncoder::SetVertexBuffer(ezUInt32 uiSlot, ezGALBufferHandle hVertexBuffer, ezUInt32 uiOffset)
{
  if (m_State.m_hVertexBuffers[uiSlot] == hVertexBuffer && m_State.m_hVertexBufferOffsets[uiSlot] == uiOffset)
  {
    return;
  }

  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hVertexBuffer);
  // Assert on vertex buffer type (if non-zero)
  // Note that GL4 can bind arbitrary buffer to arbitrary binding points (index/vertex/transform-feedback/indirect-draw/...)

  m_CommonImpl.SetVertexBufferPlatform(uiSlot, pBuffer, uiOffset);

  m_State.m_hVertexBuffers[uiSlot] = hVertexBuffer;
  m_State.m_hVertexBufferOffsets[uiSlot] = uiOffset;
}

void ezGALCommandEncoder::SetGraphicsPipeline(ezGALGraphicsPipelineHandle hGraphicsPipeline)
{
  AssertRenderingThread();
  m_State.m_hComputePipeline.Invalidate();
  if (m_State.m_hGraphicsPipeline == hGraphicsPipeline)
    return;

  const ezGALGraphicsPipeline* pGraphicsPipeline = GetDevice().GetGraphicsPipeline(hGraphicsPipeline);
  m_CommonImpl.SetGraphicsPipelinePlatform(pGraphicsPipeline);
  m_State.m_hGraphicsPipeline = hGraphicsPipeline;
}

void ezGALCommandEncoder::SetComputePipeline(ezGALComputePipelineHandle hComputePipeline)
{
  AssertRenderingThread();
  m_State.m_hGraphicsPipeline.Invalidate();
  if (m_State.m_hComputePipeline == hComputePipeline)
    return;

  const ezGALComputePipeline* pComputePipeline = GetDevice().GetComputePipeline(hComputePipeline);
  m_CommonImpl.SetComputePipelinePlatform(pComputePipeline);
  m_State.m_hComputePipeline = hComputePipeline;
}

void ezGALCommandEncoder::SetViewport(const ezRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  AssertRenderingThread();

  if (m_State.m_ViewPortRect == rect && m_State.m_fViewPortMinDepth == fMinDepth && m_State.m_fViewPortMaxDepth == fMaxDepth)
  {
    return;
  }

  m_CommonImpl.SetViewportPlatform(rect, fMinDepth, fMaxDepth);

  m_State.m_ViewPortRect = rect;
  m_State.m_fViewPortMinDepth = fMinDepth;
  m_State.m_fViewPortMaxDepth = fMaxDepth;
}

void ezGALCommandEncoder::SetScissorRect(const ezRectU32& rect)
{
  AssertRenderingThread();

  if (m_State.m_ScissorRect == rect)
  {
    return;
  }

  m_CommonImpl.SetScissorRectPlatform(rect);

  m_State.m_ScissorRect = rect;
}

void ezGALCommandEncoder::SetStencilReference(ezUInt8 uiStencilRefValue)
{
  AssertRenderingThread();

  if (m_State.m_uiStencilRefValue == uiStencilRefValue)
    return;

  m_CommonImpl.SetStencilReferencePlatform(uiStencilRefValue);

  m_State.m_uiStencilRefValue = uiStencilRefValue;
}

void ezGALCommandEncoder::BeginCompute(const char* szName)
{
  EZ_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Invalid, "Nested Command Encoder are not allowed");
  m_CurrentCommandEncoderType = CommandEncoderType::Compute;

  m_CommonImpl.BeginComputePlatform();

  m_bMarker = !ezStringUtils::IsNullOrEmpty(szName);
  if (m_bMarker)
  {
    PushMarker(szName);
  }
}

void ezGALCommandEncoder::EndCompute()
{
  EZ_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Compute, "BeginCompute has not been called");
  m_CurrentCommandEncoderType = CommandEncoderType::Invalid;

  if (m_bMarker)
  {
    PopMarker();
    m_bMarker = false;
  }

  m_CommonImpl.EndComputePlatform();
}

void ezGALCommandEncoder::BeginRendering(const ezGALRenderingSetup& renderingSetup, const char* szName)
{
  EZ_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Invalid, "Nested Command Encoder are not allowed");
  m_CurrentCommandEncoderType = CommandEncoderType::Render;

  m_CommonImpl.BeginRenderingPlatform(renderingSetup);

  m_bMarker = !ezStringUtils::IsNullOrEmpty(szName);
  if (m_bMarker)
  {
    PushMarker(szName);
  }
}

void ezGALCommandEncoder::EndRendering()
{
  EZ_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Render, "BeginRendering has not been called");
  m_CurrentCommandEncoderType = CommandEncoderType::Invalid;

  if (m_bMarker)
  {
    PopMarker();
    m_bMarker = false;
  }

  EZ_ASSERT_DEBUG(m_hPendingOcclusionQuery.IsInvalidated(), "An occlusion query was started and not stopped within this render scope.");

  m_CommonImpl.EndRenderingPlatform();
}

bool ezGALCommandEncoder::IsInRenderingScope() const
{
  return m_CurrentCommandEncoderType == CommandEncoderType::Render;
}
