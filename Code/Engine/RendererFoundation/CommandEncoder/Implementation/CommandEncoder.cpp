#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/ProxyTexture.h>
#include <RendererFoundation/Resources/ReadbackBuffer.h>
#include <RendererFoundation/Resources/ReadbackTexture.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Shader/BindGroup.h>
#include <RendererFoundation/Shader/BindGroupLayout.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/State/ComputePipeline.h>
#include <RendererFoundation/State/GraphicsPipeline.h>

#if EZ_ENABLED(EZ_BARRIER_VALIDATION)

namespace
{
  void ResolveProxyTexture(ezGALDevice* pDevice, ezGALTextureHandle& ref_hTexture, ezGALTextureRange& ref_range)
  {
    const ezGALTexture* pTexture = pDevice->GetTexture(ref_hTexture);
    EZ_ASSERT_DEBUG(pTexture != nullptr, "Invalid texture handle.");
    // Resolve proxy texture as they only cause pain down the pipeline.
    if (pTexture->GetDescription().m_Type == ezGALTextureType::Texture2DProxy)
    {
      const auto pProxy = static_cast<const ezGALProxyTexture*>(pTexture);
      ref_hTexture = pProxy->GetParentTextureHandle();
      ref_range = {pProxy->GetSlice(), 1, ref_range.m_uiBaseMipLevel, ref_range.m_uiMipLevels};
    }
  }

  /// Maps a shader resource type to the expected ezGALResourceState for validation.
  /// For texture SRVs, depth textures expect DepthStencilRead instead of ShaderResource.
  ezBitflags<ezGALResourceState> GetExpectedResourceState(ezGALShaderResourceType::Enum resourceType, const ezGALDevice& device, ezGALTextureHandle hTexture = {})
  {
    switch (resourceType)
    {
      case ezGALShaderResourceType::Texture:
      case ezGALShaderResourceType::TextureAndSampler:
      {
        if (!hTexture.IsInvalidated())
        {
          const ezGALTexture* pTexture = device.GetTexture(hTexture);
          if (pTexture != nullptr && ezGALResourceFormat::IsDepthFormat(pTexture->GetDescription().m_Format))
            return ezGALResourceState::DepthStencilRead;
        }
        return ezGALResourceState::ShaderResource;
      }
      case ezGALShaderResourceType::TextureRW:
        return ezGALResourceState::UnorderedAccess;
      case ezGALShaderResourceType::ConstantBuffer:
        return ezGALResourceState::ConstantBuffer;
      case ezGALShaderResourceType::TexelBuffer:
      case ezGALShaderResourceType::StructuredBuffer:
      case ezGALShaderResourceType::ByteAddressBuffer:
        return ezGALResourceState::ShaderResource;
      case ezGALShaderResourceType::TexelBufferRW:
      case ezGALShaderResourceType::StructuredBufferRW:
      case ezGALShaderResourceType::ByteAddressBufferRW:
        return ezGALResourceState::UnorderedAccess;
      default:
        return {};
    }
  }

  ezResult CheckTextureSubResourceState(
    const ezGALResourceStateTracker::TextureState& textureState,
    const ezGALTextureRange& range,
    ezTextureValidationError& ref_error)
  {
    auto checkSubResource = [&](const ezGALResourceStateTracker::SubResourceState& sr, const ezGALTextureSubresource& subResource) -> ezResult
    {
      if (ezGALResourceStateTracker::IsTextureBarrierNeeded(sr, {ref_error.m_expectedState, ref_error.m_expectedStages}, false))
      {
        ref_error.m_failedSubResource = subResource;
        ref_error.m_actualState = sr.m_State;
        ref_error.m_actualStages = sr.m_Stages;
        return EZ_FAILURE;
      }
      return EZ_SUCCESS;
    };

    if (textureState.m_SubResourceStates.GetCount() == 1)
    {
      ezGALTextureSubresource subResource = {0, 0};
      return checkSubResource(textureState.m_SubResourceStates[0], subResource);
    }

    const ezUInt32 uiEndSlice = ezMath::Min(static_cast<ezUInt32>(range.m_uiBaseArraySlice) + range.m_uiArraySlices, static_cast<ezUInt32>(textureState.m_FullRange.m_uiArraySlices));
    const ezUInt32 uiEndMip = ezMath::Min(static_cast<ezUInt32>(range.m_uiBaseMipLevel) + range.m_uiMipLevels, static_cast<ezUInt32>(textureState.m_FullRange.m_uiMipLevels));

    for (ezUInt32 uiSlice = range.m_uiBaseArraySlice; uiSlice < uiEndSlice; ++uiSlice)
    {
      for (ezUInt32 uiMip = range.m_uiBaseMipLevel; uiMip < uiEndMip; ++uiMip)
      {
        const ezUInt32 uiSubresourceIndex = uiMip + uiSlice * textureState.m_FullRange.m_uiMipLevels;
        ezGALTextureSubresource subResource = {uiMip, uiSlice};
        if (uiSubresourceIndex < textureState.m_SubResourceStates.GetCount())
        {
          EZ_SUCCEED_OR_RETURN(checkSubResource(textureState.m_SubResourceStates[uiSubresourceIndex], subResource));
        }
      }
    }
    return EZ_SUCCESS;
  }

  ezGALTextureRange MakeSingleSubresourceRange(const ezGALTextureSubresource& subResource)
  {
    return {static_cast<ezUInt16>(subResource.m_uiArraySlice), 1, static_cast<ezUInt8>(subResource.m_uiMipLevel), 1};
  }
} // namespace


ezUInt32 ezGALCommandEncoder::ValidationHash::Hash(const ezTextureValidationError& a)
{
  return a.CalculateHash();
}

bool ezGALCommandEncoder::ValidationHash::Equal(const ezTextureValidationError& a, const ezTextureValidationError& b)
{
  return a == b;
}

ezUInt32 ezGALCommandEncoder::ValidationHash::Hash(const ezBufferValidationError& a)
{
  return a.CalculateHash();
}

bool ezGALCommandEncoder::ValidationHash::Equal(const ezBufferValidationError& a, const ezBufferValidationError& b)
{
  return a == b;
}

#endif

ezEvent<const ezTextureValidationError&> ezGALCommandEncoder::s_TextureBarrierValidationFailed;
ezEvent<const ezBufferValidationError&> ezGALCommandEncoder::s_BufferBarrierValidationFailed;

void ezGALCommandEncoder::SetBindGroup(ezUInt32 uiBindGroup, const ezGALBindGroupCreationDescription& bindGroup)
{
  AssertRenderingThread();

  // Validation
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  {
    EZ_LOG_BLOCK("SetBindGroup");
    bindGroup.AssertValidDescription(m_Device);
  }
#endif

  EZ_ASSERT_DEBUG(uiBindGroup < EZ_GAL_MAX_BIND_GROUPS, "Bind group index {} out of range", uiBindGroup);
#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
  m_BindGroups[uiBindGroup] = bindGroup;
  m_uiBindGroupsMask |= static_cast<ezUInt8>(EZ_BIT(uiBindGroup));
#endif

  m_CommonImpl.SetBindGroupPlatform(uiBindGroup, bindGroup);
}

void ezGALCommandEncoder::SetBindGroup(ezUInt32 uiBindGroup, ezGALBindGroupHandle hBindGroup)
{
  AssertRenderingThread();

  const ezGALBindGroup* pBindGroup = m_Device.GetBindGroup(hBindGroup);
  EZ_ASSERT_DEBUG(pBindGroup != nullptr, "Bind group does not exist");
  EZ_ASSERT_DEBUG(!pBindGroup->IsInvalidated(), "Bind group is invalidated. One of its dependencies was destroyed.");

  EZ_ASSERT_DEBUG(uiBindGroup < EZ_GAL_MAX_BIND_GROUPS, "Bind group index {} out of range", uiBindGroup);
#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
  m_BindGroups[uiBindGroup] = pBindGroup->GetDescription();
  m_uiBindGroupsMask |= static_cast<ezUInt8>(EZ_BIT(uiBindGroup));
#endif

  m_CommonImpl.SetBindGroupPlatform(uiBindGroup, pBindGroup);
}

void ezGALCommandEncoder::SetPushConstants(ezArrayPtr<const ezUInt8> data)
{
  AssertRenderingThread();
  m_CommonImpl.SetPushConstantsPlatform(data);
}

ezGALTimestampHandle ezGALCommandEncoder::InsertTimestamp()
{
  AssertRenderingThread();
  m_Stats.m_uiInsertTimestamp++;
  return m_CommonImpl.InsertTimestampPlatform();
}

ezGALOcclusionHandle ezGALCommandEncoder::BeginOcclusionQuery(ezEnum<ezGALQueryType> type)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Occlusion queries can only be started within a render scope");
  AssertRenderingThread();
  m_Stats.m_uiBeginOcclusionQuery++;
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
  m_Stats.m_uiInsertFence++;
  return m_CommonImpl.InsertFencePlatform();
}

void ezGALCommandEncoder::CopyBuffer(ezGALBufferHandle hDest, ezGALBufferHandle hSource)
{
  AssertRenderingThread();
  AssertOutsideRenderingScope();

  const ezGALBuffer* pDest = m_Device.GetBuffer(hDest);
  const ezGALBuffer* pSource = m_Device.GetBuffer(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    EZ_ASSERT_DEBUG(!pDest->GetDescription().m_ResourceAccess.m_bImmutable, "Can't update immutable textures");
#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
    ValidateBufferState(hDest, ezGALResourceState::CopyDestination, ezGALShaderStageFlags::Auto).IgnoreResult();
    ValidateBufferState(hSource, ezGALResourceState::CopySource, ezGALShaderStageFlags::Auto).IgnoreResult();
#endif
    m_Stats.m_uiCopyBuffer++;
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
    EZ_ASSERT_DEBUG(!pDest->GetDescription().m_ResourceAccess.m_bImmutable, "Can't update immutable buffers");
    const ezUInt32 uiDestSize = pDest->GetSize();
    const ezUInt32 uiSourceSize = pSource->GetSize();

    EZ_IGNORE_UNUSED(uiDestSize);
    EZ_ASSERT_DEV(uiDestSize >= uiDestOffset + uiByteCount, "Destination buffer too small (or offset too big)");
    EZ_IGNORE_UNUSED(uiSourceSize);
    EZ_ASSERT_DEV(uiSourceSize >= uiSourceOffset + uiByteCount, "Source buffer too small (or offset too big)");

#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
    ValidateBufferState(hDest, ezGALResourceState::CopyDestination, ezGALShaderStageFlags::Auto).IgnoreResult();
    ValidateBufferState(hSource, ezGALResourceState::CopySource, ezGALShaderStageFlags::Auto).IgnoreResult();
#endif
    m_Stats.m_uiCopyBuffer++;
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
    EZ_ASSERT_DEBUG(!pDest->GetDescription().m_ResourceAccess.m_bImmutable, "Can't update immutable buffers");
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
    m_Stats.m_uiUpdateBuffer++;
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
    EZ_ASSERT_DEBUG(!pDest->GetDescription().m_ResourceAccess.m_bImmutable, "Can't update immutable textures");
    EZ_ASSERT_DEBUG(pDest->GetDescription().m_Format == pSource->GetDescription().m_Format, "CopyTexture formats must match");
    EZ_ASSERT_DEBUG(pDest->GetDescription().m_SampleCount == pSource->GetDescription().m_SampleCount, "CopyTexture sample count must match");
    EZ_ASSERT_DEBUG(pDest->GetDescription().m_Type == pSource->GetDescription().m_Type, "CopyTexture type must match");
    EZ_ASSERT_DEBUG(pDest->GetDescription().m_uiHeight == pSource->GetDescription().m_uiHeight, "CopyTexture height must match");
    EZ_ASSERT_DEBUG(pDest->GetDescription().m_uiWidth == pSource->GetDescription().m_uiWidth, "CopyTexture width must match");
    EZ_ASSERT_DEBUG(pDest->GetDescription().m_uiDepth == pSource->GetDescription().m_uiDepth, "CopyTexture depth must match");
    EZ_ASSERT_DEBUG(pDest->GetDescription().m_uiMipLevelCount == pSource->GetDescription().m_uiMipLevelCount, "CopyTexture mip levels must match");
    EZ_ASSERT_DEBUG(pDest->GetDescription().m_uiArraySize == pSource->GetDescription().m_uiArraySize, "CopyTexture array size must match");

#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
    ValidateTextureState(hDest, {}, ezGALResourceState::CopyDestination, ezGALShaderStageFlags::Auto).IgnoreResult();
    ValidateTextureState(hSource, {}, ezGALResourceState::CopySource, ezGALShaderStageFlags::Auto).IgnoreResult();
#endif
    m_Stats.m_uiCopyTexture++;
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
    EZ_ASSERT_DEBUG(!pDest->GetDescription().m_ResourceAccess.m_bImmutable, "Can't update immutable textures");
#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
    ValidateTextureState(hDest, MakeSingleSubresourceRange(destinationSubResource), ezGALResourceState::CopyDestination, ezGALShaderStageFlags::Auto).IgnoreResult();
    ValidateTextureState(hSource, MakeSingleSubresourceRange(sourceSubResource), ezGALResourceState::CopySource, ezGALShaderStageFlags::Auto).IgnoreResult();
#endif
    m_Stats.m_uiCopyTexture++;
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
    EZ_ASSERT_DEBUG(!pDest->GetDescription().m_ResourceAccess.m_bImmutable, "Can't update immutable textures");
    m_Stats.m_uiUpdateTexture++;
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
    EZ_ASSERT_DEBUG(!pDest->GetDescription().m_ResourceAccess.m_bImmutable, "Can't update immutable textures");
#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
    ValidateTextureState(hDest, MakeSingleSubresourceRange(destinationSubResource), ezGALResourceState::ResolveDestination, ezGALShaderStageFlags::Auto).IgnoreResult();
    ValidateTextureState(hSource, MakeSingleSubresourceRange(sourceSubResource), ezGALResourceState::ResolveSource, ezGALShaderStageFlags::Auto).IgnoreResult();
#endif
    m_Stats.m_uiResolveTexture++;
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
#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
    ValidateTextureState(hSource, {}, ezGALResourceState::CopySource, ezGALShaderStageFlags::Auto).IgnoreResult();
#endif
    m_Stats.m_uiReadbackTexture++;
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
#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
    ValidateBufferState(hSource, ezGALResourceState::CopySource, ezGALShaderStageFlags::Auto).IgnoreResult();
#endif
    m_Stats.m_uiReadbackBuffer++;
    m_CommonImpl.ReadbackBufferPlatform(pDestination, pSource);
  }
}

void ezGALCommandEncoder::Flush()
{
  AssertRenderingThread();
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType != CommandEncoderType::Render, "Flush can't be called inside a rendering scope");

  m_Stats.m_uiFlush++;
  m_CommonImpl.FlushPlatform();
}

void ezGALCommandEncoder::TextureBarrier(ezArrayPtr<const ezGALTextureBarrier> barriers)
{
  AssertRenderingThread();
  AssertOutsideRenderingScope();

#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
  ValidateTextureBarriers(barriers);
#endif

  m_CommonImpl.TextureBarrierPlatform(barriers);
}

void ezGALCommandEncoder::TextureBarrier(
  ezGALTextureHandle hTexture,
  ezGALTextureRange range,
  ezBitflags<ezGALResourceState> stateBefore,
  ezBitflags<ezGALResourceState> stateAfter,
  ezBitflags<ezGALShaderStageFlags> stagesBefore,
  ezBitflags<ezGALShaderStageFlags> stagesAfter)
{
  if (range == ezGALTextureRange{})
  {
    const ezGALTextureBarrier barrier{
      hTexture,
      stateBefore,
      stateAfter,
      stagesBefore,
      stagesAfter,
      {},
      true};

    TextureBarrier(ezArrayPtr<const ezGALTextureBarrier>(&barrier, 1));
    return;
  }

  const ezGALTexture* pTexture = m_Device.GetTexture(hTexture);
  EZ_ASSERT_DEBUG(pTexture != nullptr, "Invalid handle provided");
  if (pTexture == nullptr)
    return;

  range = pTexture->ClampRange(range);

  ezHybridArray<ezGALTextureBarrier, 16> barriers;
  barriers.Reserve(static_cast<ezUInt32>(range.m_uiMipLevels) * static_cast<ezUInt32>(range.m_uiArraySlices));

  for (ezUInt32 uiArraySlice = range.m_uiBaseArraySlice; uiArraySlice < static_cast<ezUInt32>(range.m_uiBaseArraySlice + range.m_uiArraySlices); ++uiArraySlice)
  {
    for (ezUInt32 uiMipLevel = range.m_uiBaseMipLevel; uiMipLevel < static_cast<ezUInt32>(range.m_uiBaseMipLevel + range.m_uiMipLevels); ++uiMipLevel)
    {
      barriers.PushBack({hTexture,
        stateBefore,
        stateAfter,
        stagesBefore,
        stagesAfter,
        {uiMipLevel, uiArraySlice},
        false});
    }
  }

  TextureBarrier(barriers);
}

void ezGALCommandEncoder::BufferBarrier(ezArrayPtr<const ezGALBufferBarrier> barriers)
{
  AssertRenderingThread();
  AssertOutsideRenderingScope();

#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
  ValidateBufferBarriers(barriers);
#endif

  m_CommonImpl.BufferBarrierPlatform(barriers);
}

void ezGALCommandEncoder::BufferBarrier(
  ezGALBufferHandle hBuffer,
  ezBitflags<ezGALResourceState> stateBefore,
  ezBitflags<ezGALResourceState> stateAfter,
  ezBitflags<ezGALShaderStageFlags> stagesBefore,
  ezBitflags<ezGALShaderStageFlags> stagesAfter)
{
  const ezGALBufferBarrier barrier{
    hBuffer,
    stateBefore,
    stateAfter,
    stagesBefore,
    stagesAfter};

  BufferBarrier(ezArrayPtr<const ezGALBufferBarrier>(&barrier, 1));
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
#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
  , m_ResourceStateTracker(&ref_device)
#endif
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
#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
  m_ResourceStateTracker.Clear();
  m_uiBindGroupsMask = 0;
  m_bVertexBufferStatesDirty = true;
  m_bIndexBufferStateDirty = true;
#endif
}

ezResult ezGALCommandEncoder::Dispatch(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Compute, "Call BeginCompute first");
  AssertRenderingThread();

  EZ_ASSERT_DEBUG(uiThreadGroupCountX > 0 && uiThreadGroupCountY > 0 && uiThreadGroupCountZ > 0, "Thread group counts of zero are not meaningful. Did you mean 1?");

#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
  EZ_SUCCEED_OR_RETURN(ValidateComputePipelineResources());
#endif

  m_Stats.m_uiDispatch++;
  return m_CommonImpl.DispatchPlatform(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

ezResult ezGALCommandEncoder::DispatchIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Compute, "Call BeginCompute first");
  AssertRenderingThread();

  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  EZ_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");

#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
  EZ_SUCCEED_OR_RETURN(ValidateComputePipelineResources());
  EZ_SUCCEED_OR_RETURN(ValidateBufferState(hIndirectArgumentBuffer, ezGALResourceState::DrawIndirect, ezGALShaderStageFlags::Auto));
#endif

  m_Stats.m_uiDispatch++;
  return m_CommonImpl.DispatchIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);
}

void ezGALCommandEncoder::Clear(const ezColor& clearColor, ezUInt32 uiRenderTargetClearMask /*= 0xFFFFFFFFu*/, bool bClearDepth /*= true*/, bool bClearStencil /*= true*/, float fDepthClear /*= 1.0f*/, ezUInt8 uiStencilClear /*= 0x0u*/)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();
  m_Stats.m_uiClear++;
  m_CommonImpl.ClearPlatform(clearColor, uiRenderTargetClearMask, bClearDepth, bClearStencil, fDepthClear, uiStencilClear);
}

ezResult ezGALCommandEncoder::Draw(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();

#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
  EZ_SUCCEED_OR_RETURN(ValidateGraphicsPipelineResources());
  EZ_SUCCEED_OR_RETURN(ValidateVertexBufferState());
#endif

  m_Stats.m_uiDraw++;
  return m_CommonImpl.DrawPlatform(uiVertexCount, uiStartVertex);
}

ezResult ezGALCommandEncoder::DrawIndexed(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();

#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
  EZ_SUCCEED_OR_RETURN(ValidateGraphicsPipelineResources());
  EZ_SUCCEED_OR_RETURN(ValidateVertexBufferState());
  EZ_SUCCEED_OR_RETURN(ValidateIndexBufferState());
#endif

  m_Stats.m_uiDraw++;
  return m_CommonImpl.DrawIndexedPlatform(uiIndexCount, uiStartIndex);
}

ezResult ezGALCommandEncoder::DrawIndexedInstanced(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();

#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
  EZ_SUCCEED_OR_RETURN(ValidateGraphicsPipelineResources());
  EZ_SUCCEED_OR_RETURN(ValidateVertexBufferState());
  EZ_SUCCEED_OR_RETURN(ValidateIndexBufferState());
#endif

  m_Stats.m_uiDraw++;
  return m_CommonImpl.DrawIndexedInstancedPlatform(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex);
}

ezResult ezGALCommandEncoder::DrawIndexedInstancedIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();
  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  EZ_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");

#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
  EZ_SUCCEED_OR_RETURN(ValidateGraphicsPipelineResources());
  EZ_SUCCEED_OR_RETURN(ValidateVertexBufferState());
  EZ_SUCCEED_OR_RETURN(ValidateIndexBufferState());
  EZ_SUCCEED_OR_RETURN(ValidateBufferState(hIndirectArgumentBuffer, ezGALResourceState::DrawIndirect, ezGALShaderStageFlags::Auto));
#endif

  m_Stats.m_uiDraw++;
  return m_CommonImpl.DrawIndexedInstancedIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);
}

ezResult ezGALCommandEncoder::DrawInstanced(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();

#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
  EZ_SUCCEED_OR_RETURN(ValidateGraphicsPipelineResources());
  EZ_SUCCEED_OR_RETURN(ValidateVertexBufferState());
#endif

  m_Stats.m_uiDraw++;
  return m_CommonImpl.DrawInstancedPlatform(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex);
}

ezResult ezGALCommandEncoder::DrawInstancedIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType == CommandEncoderType::Render, "Call BeginRendering first");
  AssertRenderingThread();
  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  EZ_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");

#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
  EZ_SUCCEED_OR_RETURN(ValidateGraphicsPipelineResources());
  EZ_SUCCEED_OR_RETURN(ValidateVertexBufferState());
  EZ_SUCCEED_OR_RETURN(ValidateBufferState(hIndirectArgumentBuffer, ezGALResourceState::DrawIndirect, ezGALShaderStageFlags::Auto));
#endif

  m_Stats.m_uiDraw++;
  return m_CommonImpl.DrawInstancedIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);
}

void ezGALCommandEncoder::SetIndexBuffer(ezGALBufferHandle hIndexBuffer)
{
  if (m_State.m_hIndexBuffer == hIndexBuffer)
  {
    return;
  }

  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hIndexBuffer);

#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
  m_bIndexBufferStateDirty = true;
#endif

  m_Stats.m_uiSetIndexBuffer++;
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

#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
  m_bVertexBufferStatesDirty = true;
#endif

  m_Stats.m_uiSetVertexBuffer++;
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
  m_Stats.m_uiSetGraphicsPipeline++;
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
  m_Stats.m_uiSetComputePipeline++;
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

  m_Stats.m_uiSetViewport++;
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

  m_Stats.m_uiSetScissorRect++;
  m_CommonImpl.SetScissorRectPlatform(rect);

  m_State.m_ScissorRect = rect;
}

void ezGALCommandEncoder::SetStencilReference(ezUInt8 uiStencilRefValue)
{
  AssertRenderingThread();

  if (m_State.m_uiStencilRefValue == uiStencilRefValue)
    return;

  m_Stats.m_uiSetStencilReference++;
  m_CommonImpl.SetStencilReferencePlatform(uiStencilRefValue);

  m_State.m_uiStencilRefValue = uiStencilRefValue;
}

void ezGALCommandEncoder::BeginCompute(const char* szName)
{
  EZ_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Invalid, "Nested Command Encoder are not allowed");
  m_CurrentCommandEncoderType = CommandEncoderType::Compute;

  m_Stats.m_uiBeginCompute++;
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

#if EZ_ENABLED(EZ_BARRIER_VALIDATION)
  ValidateRenderTargetStates(renderingSetup);
  m_bVertexBufferStatesDirty = true;
  m_bIndexBufferStateDirty = true;
#endif

  m_Stats.m_uiBeginRendering++;
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

void ezGALCommandEncoder::ResetStats()
{
  m_Stats = ezGALCommandEncoderStats();
}

#if EZ_ENABLED(EZ_BARRIER_VALIDATION)

ezResult ezGALCommandEncoder::ValidateGraphicsPipelineResources()
{
  if (const ezGALGraphicsPipeline* pPipeline = m_Device.GetGraphicsPipeline(m_State.m_hGraphicsPipeline))
    return ValidateBindGroupResourceStates(m_Device.GetShader(pPipeline->GetDescription().m_hShader));

  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoder::ValidateComputePipelineResources()
{
  if (const ezGALComputePipeline* pPipeline = m_Device.GetComputePipeline(m_State.m_hComputePipeline))
    return ValidateBindGroupResourceStates(m_Device.GetShader(pPipeline->GetDescription().m_hShader));

  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoder::ValidateVertexBufferState()
{
  if (m_bVertexBufferStatesDirty)
  {
    for (ezGALBufferHandle hVertexBuffer : m_State.m_hVertexBuffers)
    {
      if (!hVertexBuffer.IsInvalidated())
      {
        EZ_SUCCEED_OR_RETURN(ValidateBufferState(hVertexBuffer, ezGALResourceState::VertexBuffer, ezGALShaderStageFlags::Auto));
      }
    }

    m_bVertexBufferStatesDirty = false;
  }

  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoder::ValidateIndexBufferState()
{
  if (m_bIndexBufferStateDirty)
  {
    if (!m_State.m_hIndexBuffer.IsInvalidated())
    {
      EZ_SUCCEED_OR_RETURN(ValidateBufferState(m_State.m_hIndexBuffer, ezGALResourceState::IndexBuffer, ezGALShaderStageFlags::Auto));
    }

    m_bIndexBufferStateDirty = false;
  }

  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoder::ValidateBindGroupResourceStates(const ezGALShader* pShader)
{
  if (pShader == nullptr)
  {
    ezLog::Error("Can't validate bind groups, shader is invalid.");
    return EZ_FAILURE;
  }
  const ezUInt32 uiBindGroupCount = pShader->GetBindGroupCount();
  for (ezUInt32 uiBindGroup = 0; uiBindGroup < uiBindGroupCount; ++uiBindGroup)
  {
    if ((m_uiBindGroupsMask & EZ_BIT(uiBindGroup)) == 0)
      continue;
    // Only check once per bind group
    m_uiBindGroupsMask &= ~static_cast<ezUInt8>(EZ_BIT(uiBindGroup));

    const ezGALBindGroupLayout* pLayout = m_Device.GetBindGroupLayout(pShader->GetBindGroupLayout(uiBindGroup));
    if (pLayout == nullptr)
    {
      ezLog::Error("Can't validate bind group {}, bind group layout is invalid.", uiBindGroup);
      continue;
    }
    ezArrayPtr<const ezShaderResourceBinding> bindings = pLayout->GetDescription().m_ResourceBindings;
    const ezGALBindGroupCreationDescription& bindGroupDesc = m_BindGroups[uiBindGroup];

    if (bindGroupDesc.m_BindGroupItems.GetCount() != bindings.GetCount())
    {
      ezLog::Error("Can't validate bind group {}, binding count {} does not match bind group item count {}", uiBindGroup, bindings.GetCount(), bindGroupDesc.m_BindGroupItems.GetCount());
      continue;
    }

    for (ezUInt32 i = 0; i < bindings.GetCount(); ++i)
    {
      const ezShaderResourceBinding& binding = bindings[i];
      const ezGALBindGroupItem& item = bindGroupDesc.m_BindGroupItems[i];
      if (ValidateBindGroupItemResourceState(uiBindGroup, binding, item).Failed())
      {

        return EZ_FAILURE;
      }
    }
  }
  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoder::ValidateBindGroupItemResourceState(ezUInt32 uiBindGroup, const ezShaderResourceBinding& binding, const ezGALBindGroupItem& item)
{
  const ezBitflags<ezGALBindGroupItemFlags> typeFlags = item.m_Flags & ezGALBindGroupItemFlags::TypeFlags;
  if (typeFlags.IsSet(ezGALBindGroupItemFlags::Texture))
  {
    const ezBitflags<ezGALResourceState> expectedState = GetExpectedResourceState(binding.m_ResourceType, m_Device, item.m_Texture.m_hTexture);
    EZ_ASSERT_DEBUG(!expectedState.IsNoFlagSet(), "At least one flag should e set in the expected resource state.");
    return ValidateTextureState(item.m_Texture.m_hTexture, item.m_Texture.m_TextureRange, expectedState, binding.m_Stages, uiBindGroup, binding.m_sName);
  }
  else if (typeFlags.IsSet(ezGALBindGroupItemFlags::Buffer))
  {
    const ezBitflags<ezGALResourceState> expectedState = GetExpectedResourceState(binding.m_ResourceType, m_Device);
    return ValidateBufferState(item.m_Buffer.m_hBuffer, expectedState, binding.m_Stages, uiBindGroup, binding.m_sName);
  }
  // Samplers and push constants have no resource state to validate.
  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoder::ValidateTextureState(ezGALTextureHandle hTexture, ezGALTextureRange range, ezBitflags<ezGALResourceState> expectedState, ezBitflags<ezGALShaderStageFlags> expectedStages, ezUInt32 uiBindGroup, const ezHashedString& sBinding)
{
  if (expectedState.IsNoFlagSet())
    return EZ_SUCCESS;

  ResolveProxyTexture(&m_Device, hTexture, range);

  const ezGALTexture* pTexture = m_Device.GetTexture(hTexture);
  if (pTexture == nullptr)
  {
    if (sBinding.IsEmpty())
      ezLog::Error("Can't validate texture state as the texture is invalid.");
    else
      ezLog::Error("Can't validate bind group {} binding '{}' as the texture is invalid.", uiBindGroup, sBinding);
    return EZ_FAILURE;
  }

  range = pTexture->ClampRange(range);

  const ezGALResourceStateTracker::TextureState* pState = m_ResourceStateTracker.GetTextureState(hTexture);
  ezGALResourceStateTracker::TextureState dummy;
  if (pState == nullptr)
  {
    // Nothing is tracked, compare to default state.
    dummy.m_FullRange = pTexture->ClampRange({});
    dummy.m_SubResourceStates.PushBack({pTexture->GetDescription().GetDefaultState(), ezGALShaderStageFlags::Auto});
    pState = &dummy;
  }

  ezTextureValidationError error;
  error.m_uiBindGroup = uiBindGroup;
  error.m_sBinding = sBinding;
  error.m_hTexture = hTexture;
  error.m_expectedState = expectedState;
  error.m_expectedStages = expectedStages;

  if (CheckTextureSubResourceState(*pState, range, error).Failed())
  {
    if (!m_TextureErrors.Insert(error))
      s_TextureBarrierValidationFailed.Broadcast(error);
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoder::ValidateBufferState(ezGALBufferHandle hBuffer, ezBitflags<ezGALResourceState> expectedState, ezBitflags<ezGALShaderStageFlags> expectedStages, ezUInt32 uiBindGroup, const ezHashedString& sBinding)
{
  if (expectedState.IsNoFlagSet())
    return EZ_SUCCESS;

  const ezGALResourceStateTracker::SubResourceState* pState = m_ResourceStateTracker.GetBufferState(hBuffer);
  ezGALResourceStateTracker::SubResourceState dummy;
  if (pState == nullptr)
  {
    // Nothing is tracked, compare to default state.
    const ezGALBuffer* pBuffer = m_Device.GetBuffer(hBuffer);
    if (pBuffer == nullptr)
    {
      if (sBinding.IsEmpty())
        ezLog::Error("Can't validate buffer state as the buffer is invalid.");
      else
        ezLog::Error("Can't validate bind group {} binding '{}' as the buffer is invalid.", uiBindGroup, sBinding);
      return EZ_FAILURE;
    }
    dummy.m_State = pBuffer->GetDescription().GetDefaultState();
    dummy.m_Stages = ezGALShaderStageFlags::Auto;
    pState = &dummy;
  }

  if (ezGALResourceStateTracker::IsBufferBarrierNeeded(*pState, {expectedState, expectedStages}, false))
  {
    ezBufferValidationError error;
    error.m_uiBindGroup = uiBindGroup;
    error.m_sBinding = sBinding;
    error.m_hBuffer = hBuffer;
    error.m_expectedState = expectedState;
    error.m_expectedStages = expectedStages;
    error.m_actualState = pState->m_State;
    error.m_actualStages = pState->m_Stages;

    if (!m_BufferErrors.Insert(error))
      s_BufferBarrierValidationFailed.Broadcast(error);
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezGALCommandEncoder::ValidateTextureBarriers(ezArrayPtr<const ezGALTextureBarrier> barriers)
{
  for (const ezGALTextureBarrier& barrier : barriers)
  {
    const ezGALResourceStateTracker::TextureState* pState = m_ResourceStateTracker.GetTextureState(barrier.m_hTexture);
    if (pState != nullptr)
    {
      if (pState->m_SubResourceStates.GetCount() == 1)
      {
        EZ_ASSERT_DEV(pState->m_SubResourceStates[0].m_State == barrier.m_StateBefore,
          "Texture barrier before-state mismatch. Tracked: {}, Barrier: {}",
          ezArgEnum(pState->m_SubResourceStates[0].m_State), ezArgEnum(barrier.m_StateBefore));
      }
      else if (!barrier.m_bAllSubresources)
      {
        const ezUInt32 uiSubresourceIndex = barrier.m_Subresource.m_uiMipLevel + barrier.m_Subresource.m_uiArraySlice * pState->m_FullRange.m_uiMipLevels;
        if (uiSubresourceIndex < pState->m_SubResourceStates.GetCount())
        {
          EZ_ASSERT_DEV(pState->m_SubResourceStates[uiSubresourceIndex].m_State == barrier.m_StateBefore,
            "Texture barrier before-state mismatch at subresource (mip={}, slice={}). Tracked: {}, Barrier: {}",
            barrier.m_Subresource.m_uiMipLevel, barrier.m_Subresource.m_uiArraySlice,
            ezArgEnum(pState->m_SubResourceStates[uiSubresourceIndex].m_State), ezArgEnum(barrier.m_StateBefore));
        }
      }
    }

    ezGALTextureRange range;
    if (barrier.m_bAllSubresources)
    {
      range = {};
    }
    else
    {
      range.m_uiBaseMipLevel = static_cast<ezUInt8>(barrier.m_Subresource.m_uiMipLevel);
      range.m_uiMipLevels = 1;
      range.m_uiBaseArraySlice = static_cast<ezUInt16>(barrier.m_Subresource.m_uiArraySlice);
      range.m_uiArraySlices = 1;
    }
    m_ResourceStateTracker.ChangeState(barrier.m_hTexture, range, barrier.m_StateAfter, barrier.m_StagesAfter, [](const ezGALTextureBarrier&) {});
  }
}

void ezGALCommandEncoder::ValidateBufferBarriers(ezArrayPtr<const ezGALBufferBarrier> barriers)
{
  for (const ezGALBufferBarrier& barrier : barriers)
  {
    const ezGALResourceStateTracker::SubResourceState* pState = m_ResourceStateTracker.GetBufferState(barrier.m_hBuffer);
    if (pState != nullptr)
    {
      EZ_ASSERT_DEV(pState->m_State == barrier.m_StateBefore,
        "Buffer barrier before-state mismatch. Tracked: {}, Barrier: {}",
        ezArgEnum(pState->m_State), ezArgEnum(barrier.m_StateBefore));
    }

    m_ResourceStateTracker.ChangeState(barrier.m_hBuffer, barrier.m_StateAfter, barrier.m_StagesAfter, [](const ezGALBufferBarrier&) {});
  }
}

void ezGALCommandEncoder::ValidateRenderTargetStates(const ezGALRenderingSetup& renderingSetup)
{
  const ezGALFrameBufferDescriptor& fb = renderingSetup.GetFrameBuffer();

  for (ezUInt8 i = 0; i < renderingSetup.GetColorTargetCount(); ++i)
  {
    if (fb.m_hColorTarget[i].IsInvalidated())
      continue;

    const ezGALRenderTargetView* pView = m_Device.GetRenderTargetView(fb.m_hColorTarget[i]);
    if (pView == nullptr)
      continue;

    const auto& viewDesc = pView->GetDescription();
    const ezGALTextureRange colorRange = ezGALTextureRange::MakeFromRenderTargetRange({static_cast<ezUInt16>(viewDesc.m_uiFirstSlice), static_cast<ezUInt16>(viewDesc.m_uiSliceCount), static_cast<ezUInt8>(viewDesc.m_uiMipLevel)});
    ValidateTextureState(viewDesc.m_hTexture, colorRange, ezGALResourceState::RenderTarget, ezGALShaderStageFlags::Auto).IgnoreResult();
  }

  if (!fb.m_hDepthTarget.IsInvalidated())
  {
    const ezGALRenderTargetView* pView = m_Device.GetRenderTargetView(fb.m_hDepthTarget);
    if (pView != nullptr)
    {
      const auto& viewDesc = pView->GetDescription();
      const ezBitflags<ezGALResourceState> expectedState = viewDesc.m_bReadOnly ? ezGALResourceState::DepthStencilRead : ezGALResourceState::DepthStencilWrite;
      const ezGALTextureRange depthRange = ezGALTextureRange::MakeFromRenderTargetRange({static_cast<ezUInt16>(viewDesc.m_uiFirstSlice), static_cast<ezUInt16>(viewDesc.m_uiSliceCount), static_cast<ezUInt8>(viewDesc.m_uiMipLevel)});
      ValidateTextureState(viewDesc.m_hTexture, depthRange, expectedState, ezGALShaderStageFlags::Auto).IgnoreResult();
    }
  }
}

#endif // EZ_BARRIER_VALIDATION
