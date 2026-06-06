#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/RendererFallbackResources.h>
#include <RendererFoundation/Shader/BindGroupLayout.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/CommandEncoder/CommandEncoderImplVulkan.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/Pools/DescriptorWritePoolVulkan.h>
#include <RendererVulkan/Pools/FencePoolVulkan.h>
#include <RendererVulkan/Pools/QueryPoolVulkan.h>
#include <RendererVulkan/Pools/StagingBufferPoolVulkan.h>
#include <RendererVulkan/Pools/TransientDescriptorSetPoolVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/ReadbackBufferVulkan.h>
#include <RendererVulkan/Resources/ReadbackTextureVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Shader/BindGroupLayoutVulkan.h>
#include <RendererVulkan/Shader/BindGroupVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/State/ComputePipelineVulkan.h>
#include <RendererVulkan/State/GraphicsPipelineVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/BarrierUtilsVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

ezGALCommandEncoderImplVulkan::ezGALCommandEncoderImplVulkan(ezGALDeviceVulkan& device)
  : m_GALDeviceVulkan(device)
{
  m_vkDevice = device.GetVulkanDevice();
  m_pUniformBufferPool = EZ_NEW(device.GetAllocator(), ezUniformBufferPoolVulkan, &device);
  m_pUniformBufferPool->Initialize();
  m_pWritePool = EZ_NEW(device.GetAllocator(), ezDescriptorWritePoolVulkan, &device);
}

ezGALCommandEncoderImplVulkan::~ezGALCommandEncoderImplVulkan()
{
  m_pUniformBufferPool->DeInitialize();
  m_pUniformBufferPool = nullptr;
}

void ezGALCommandEncoderImplVulkan::Reset()
{
  MarkAllStateDirty();
  m_BoundVertexBuffersRange.Reset();

  m_pShader = nullptr;
  m_pGraphicsPipeline = nullptr;
  m_pComputePipeline = nullptr;

  m_viewport = vk::Viewport();
  m_scissor = vk::Rect2D();
  m_uiStencilRefValue = 0;

  m_pIndexBuffer = nullptr;
  for (ezUInt32 i = 0; i < EZ_GAL_MAX_VERTEX_BUFFER_COUNT; i++)
  {
    m_pBoundVertexBuffers[i] = nullptr;
    m_VertexBufferOffsets[i] = 0;
  }

  for (ezUInt32 i = 0; i < EZ_GAL_MAX_BIND_GROUPS; i++)
  {
    // m_DescriptorSets and m_BindGroupDirty are handled in MarkAllStateDirty().
    m_BindGroups[i].m_hBindGroupLayout = {};
    m_BindGroups[i].m_BindGroupItems.Clear();
    m_pBindGroups[i] = nullptr;
    m_DynamicOffsets[i].m_DynamicUniformBuffers.Clear();
    m_DynamicOffsets[i].m_DynamicUniformVkBuffers.Clear();
    m_DynamicOffsets[i].m_DynamicUniformBufferOffsets.Clear();
  }
  m_DescriptorCache.Clear();
  m_PushConstants.Clear();

  m_renderPass = vk::RenderPassBeginInfo();
}

void ezGALCommandEncoderImplVulkan::EndFrame()
{
  m_pUniformBufferPool->EndFrame();
}

void ezGALCommandEncoderImplVulkan::BeforeCommandBufferSubmit()
{
  m_pUniformBufferPool->BeforeCommandBufferSubmit();
}

void ezGALCommandEncoderImplVulkan::AfterCommandBufferSubmit(vk::Fence submitFence)
{
  m_pCommandBuffer = nullptr;
  // We can't carry state across individual command buffers, so mark all state as dirty.
  MarkAllStateDirty();
  m_BoundVertexBuffersRange.Reset();
  for (ezUInt32 i = 0; i < EZ_GAL_MAX_VERTEX_BUFFER_COUNT; i++)
  {
    if (m_pBoundVertexBuffers[i])
      m_BoundVertexBuffersRange.SetToIncludeValue(i);
  }

  // Fence logic
  m_GALDeviceVulkan.GetFenceQueue().FenceSubmitted(submitFence);
}

void ezGALCommandEncoderImplVulkan::MarkAllStateDirty()
{
  m_bPipelineStateDirty = true;
  m_bViewportDirty = true;
  m_bScissorDirty = true;
  m_bIndexBufferDirty = true;
  m_bDynamicOffsetsDirty = true;
  // Vulkan dynamic state (including stencil reference) is per-command-buffer, so we must re-emit it whenever a new command buffer becomes active. The HasStencilTest guard in FlushDeferredStateChanges suppresses the call when it is not needed.
  m_bStencilRefDirty = true;
  for (ezUInt32 i = 0; i < EZ_GAL_MAX_BIND_GROUPS; i++)
  {
    m_DescriptorSets[i] = nullptr;
    m_BindGroupDirty[i] = true;
  }
}

void ezGALCommandEncoderImplVulkan::SetCurrentCommandBuffer(vk::CommandBuffer* commandBuffer)
{
  m_pCommandBuffer = commandBuffer;
}

// State setting functions

void ezGALCommandEncoderImplVulkan::SetBindGroupPlatform(ezUInt32 uiBindGroup, const ezGALBindGroupCreationDescription& bindGroup)
{
  m_BindGroups[uiBindGroup] = bindGroup;
  m_pBindGroups[uiBindGroup] = nullptr;
  m_DynamicOffsets[uiBindGroup].m_DynamicUniformBuffers.Clear();
  m_DynamicOffsets[uiBindGroup].m_DynamicUniformVkBuffers.Clear();
  m_DynamicOffsets[uiBindGroup].m_DynamicUniformBufferOffsets.Clear();
  m_BindGroupDirty[uiBindGroup] = true;
}

void ezGALCommandEncoderImplVulkan::SetBindGroupPlatform(ezUInt32 uiBindGroup, const ezGALBindGroup* pBindGroup)
{
  auto pBindGroupVulkan = static_cast<const ezGALBindGroupVulkan*>(pBindGroup);
  if (pBindGroupVulkan != m_pBindGroups[uiBindGroup])
  {
    m_BindGroups[uiBindGroup].m_hBindGroupLayout = {};
    m_BindGroups[uiBindGroup].m_BindGroupItems.Clear();
    m_pBindGroups[uiBindGroup] = pBindGroupVulkan;
    m_DynamicOffsets[uiBindGroup].m_DynamicUniformBuffers.Clear();
    m_DynamicOffsets[uiBindGroup].m_DynamicUniformVkBuffers.Clear();
    m_DynamicOffsets[uiBindGroup].m_DynamicUniformBufferOffsets = pBindGroupVulkan->GetOffsets();
    m_BindGroupDirty[uiBindGroup] = true;
  }
}

void ezGALCommandEncoderImplVulkan::SetPushConstantsPlatform(ezArrayPtr<const ezUInt8> data)
{
  m_bPushConstantsDirty = true;
  m_PushConstants = data;
}

// Query functions

ezGALTimestampHandle ezGALCommandEncoderImplVulkan::InsertTimestampPlatform()
{
  return m_GALDeviceVulkan.GetQueryPool().InsertTimestamp(*m_pCommandBuffer);
}

ezGALOcclusionHandle ezGALCommandEncoderImplVulkan::BeginOcclusionQueryPlatform(ezEnum<ezGALQueryType> type)
{
  return m_GALDeviceVulkan.GetQueryPool().BeginOcclusionQuery(*m_pCommandBuffer, type);
}

void ezGALCommandEncoderImplVulkan::EndOcclusionQueryPlatform(ezGALOcclusionHandle hOcclusion)
{
  m_GALDeviceVulkan.GetQueryPool().EndOcclusionQuery(*m_pCommandBuffer, hOcclusion);
}

ezGALFenceHandle ezGALCommandEncoderImplVulkan::InsertFencePlatform()
{
  return m_GALDeviceVulkan.GetFenceQueue().GetCurrentFenceHandle();
}


// Resource update functions

void ezGALCommandEncoderImplVulkan::CopyBufferPlatform(const ezGALBuffer* pDestination, const ezGALBuffer* pSource)
{
  auto pDestinationVulkan = static_cast<const ezGALBufferVulkan*>(pDestination);
  auto pSourceVulkan = static_cast<const ezGALBufferVulkan*>(pSource);

  EZ_ASSERT_DEV(pSource->GetSize() == pDestination->GetSize(), "Source and destination buffer sizes mismatch!");

  vk::BufferCopy bufferCopy = {};
  bufferCopy.size = pSource->GetSize();

  m_pCommandBuffer->copyBuffer(pSourceVulkan->GetVkBuffer(), pDestinationVulkan->GetVkBuffer(), 1, &bufferCopy);
}

void ezGALCommandEncoderImplVulkan::CopyBufferRegionPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, const ezGALBuffer* pSource,
  ezUInt32 uiSourceOffset, ezUInt32 uiByteCount)
{
  auto pDestinationVulkan = static_cast<const ezGALBufferVulkan*>(pDestination);
  auto pSourceVulkan = static_cast<const ezGALBufferVulkan*>(pSource);

  vk::BufferCopy bufferCopy = {};
  bufferCopy.dstOffset = uiDestOffset;
  bufferCopy.srcOffset = uiSourceOffset;
  bufferCopy.size = uiByteCount;

  m_pCommandBuffer->copyBuffer(pSourceVulkan->GetVkBuffer(), pDestinationVulkan->GetVkBuffer(), 1, &bufferCopy);
}

void ezGALCommandEncoderImplVulkan::UpdateBufferPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> pSourceData, ezGALUpdateMode::Enum updateMode)
{
  auto pVulkanDestination = static_cast<const ezGALBufferVulkan*>(pDestination);
  switch (updateMode)
  {
    case ezGALUpdateMode::TransientConstantBuffer:
      EZ_ASSERT_DEBUG(pDestination->GetDescription().m_BufferFlags.AreAllSet(ezGALBufferUsageFlags::Transient | ezGALBufferUsageFlags::ConstantBuffer), "Only transient constant buffer can make use of TransientConstantBuffer update mode");
      EZ_ASSERT_DEBUG(uiDestOffset == 0, "Offset not supported");
      EZ_ASSERT_DEBUG(pVulkanDestination->GetDescription().m_uiTotalSize == pSourceData.GetCount(), "Transient buffers must be updated in their entirety");
      m_pUniformBufferPool->UpdateBuffer(pVulkanDestination, pSourceData);
      m_bDynamicOffsetsDirty = true;
      break;
    case ezGALUpdateMode::AheadOfTime:
      m_GALDeviceVulkan.GetInitContext().UpdateBuffer(pVulkanDestination, uiDestOffset, pSourceData);
      break;
      EZ_DEFAULT_CASE_NOT_IMPLEMENTED
  }
}

void ezGALCommandEncoderImplVulkan::CopyTexturePlatform(const ezGALTexture* pDestination, const ezGALTexture* pSource)
{
  auto destination = static_cast<const ezGALTextureVulkan*>(pDestination->GetParentResource());
  auto source = static_cast<const ezGALTextureVulkan*>(pSource->GetParentResource());

  const ezGALTextureCreationDescription& destDesc = pDestination->GetDescription();
  const ezGALTextureCreationDescription& srcDesc = pSource->GetDescription();

  EZ_ASSERT_DEBUG(ezGALResourceFormat::IsDepthFormat(destDesc.m_Format) == ezGALResourceFormat::IsDepthFormat(srcDesc.m_Format), "");
  EZ_ASSERT_DEBUG(destDesc.m_uiArraySize == srcDesc.m_uiArraySize, "");
  EZ_ASSERT_DEBUG(destDesc.m_uiMipLevelCount == srcDesc.m_uiMipLevelCount, "");
  EZ_ASSERT_DEBUG(destDesc.m_uiWidth == srcDesc.m_uiWidth, "");
  EZ_ASSERT_DEBUG(destDesc.m_uiHeight == srcDesc.m_uiHeight, "");
  EZ_ASSERT_DEBUG(destDesc.m_uiDepth == srcDesc.m_uiDepth, "");

  vk::ImageAspectFlagBits imageAspect = ezGALResourceFormat::IsDepthFormat(destDesc.m_Format) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;

  const ezUInt32 uiArrayLayers = (destDesc.m_Type == ezGALTextureType::TextureCube || destDesc.m_Type == ezGALTextureType::TextureCubeArray) ? destDesc.m_uiArraySize * 6 : destDesc.m_uiArraySize;

  ezHybridArray<vk::ImageCopy, 14> imageCopies;

  for (ezUInt32 i = 0; i < destDesc.m_uiMipLevelCount; ++i)
  {
    vk::ImageCopy& imageCopy = imageCopies.ExpandAndGetRef();
    imageCopy.dstOffset = vk::Offset3D();
    imageCopy.dstSubresource.aspectMask = imageAspect;
    imageCopy.dstSubresource.baseArrayLayer = 0;
    imageCopy.dstSubresource.layerCount = uiArrayLayers;
    imageCopy.dstSubresource.mipLevel = i;
    imageCopy.extent = destination->GetMipLevelSize(i);
    imageCopy.srcOffset = vk::Offset3D();
    imageCopy.srcSubresource.aspectMask = imageAspect;
    imageCopy.srcSubresource.baseArrayLayer = 0;
    imageCopy.srcSubresource.layerCount = uiArrayLayers;
    imageCopy.srcSubresource.mipLevel = i;
  }

  m_pCommandBuffer->copyImage(source->GetImage(), vk::ImageLayout::eTransferSrcOptimal, destination->GetImage(), vk::ImageLayout::eTransferDstOptimal, destDesc.m_uiMipLevelCount, imageCopies.GetData());
}

void ezGALCommandEncoderImplVulkan::CopyTextureRegionPlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
  const ezVec3U32& DestinationPoint, const ezGALTexture* pSource,
  const ezGALTextureSubresource& SourceSubResource, const ezBoundingBoxu32& Box)
{
  auto destination = static_cast<const ezGALTextureVulkan*>(pDestination->GetParentResource());
  auto source = static_cast<const ezGALTextureVulkan*>(pSource->GetParentResource());

  const ezGALTextureCreationDescription& destDesc = pDestination->GetDescription();
  const ezGALTextureCreationDescription& srcDesc = pSource->GetDescription();

  EZ_ASSERT_DEBUG(ezGALResourceFormat::IsDepthFormat(destDesc.m_Format) == ezGALResourceFormat::IsDepthFormat(srcDesc.m_Format), "");

  vk::ImageAspectFlagBits imageAspect = ezGALResourceFormat::IsDepthFormat(destDesc.m_Format) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;

  ezVec3U32 extent = Box.m_vMax - Box.m_vMin;

  vk::ImageCopy imageCopy = {};
  imageCopy.dstOffset.x = DestinationPoint.x;
  imageCopy.dstOffset.y = DestinationPoint.y;
  imageCopy.dstOffset.z = DestinationPoint.z;
  imageCopy.dstSubresource.aspectMask = imageAspect;
  imageCopy.dstSubresource.baseArrayLayer = DestinationSubResource.m_uiArraySlice;
  imageCopy.dstSubresource.layerCount = 1;
  imageCopy.dstSubresource.mipLevel = DestinationSubResource.m_uiMipLevel;
  imageCopy.extent.width = extent.x;
  imageCopy.extent.height = extent.y;
  imageCopy.extent.depth = extent.z;
  imageCopy.srcOffset.x = Box.m_vMin.x;
  imageCopy.srcOffset.y = Box.m_vMin.y;
  imageCopy.srcOffset.z = Box.m_vMin.z;
  imageCopy.srcSubresource.aspectMask = imageAspect;
  imageCopy.srcSubresource.baseArrayLayer = SourceSubResource.m_uiArraySlice;
  imageCopy.srcSubresource.layerCount = 1;
  imageCopy.srcSubresource.mipLevel = SourceSubResource.m_uiMipLevel;

  m_pCommandBuffer->copyImage(source->GetImage(), vk::ImageLayout::eTransferSrcOptimal, destination->GetImage(), vk::ImageLayout::eTransferDstOptimal, 1, &imageCopy);
}

void ezGALCommandEncoderImplVulkan::UpdateTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
  const ezBoundingBoxu32& DestinationBox, const ezGALSystemMemoryDescription& data)
{
  const ezGALTextureVulkan* pVulkanDestination = static_cast<const ezGALTextureVulkan*>(pDestination);
  m_GALDeviceVulkan.GetInitContext().UpdateTexture(pVulkanDestination, DestinationSubResource, DestinationBox, data);
}

void ezGALCommandEncoderImplVulkan::ResolveTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
  const ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource)
{
  auto pVulkanDestination = static_cast<const ezGALTextureVulkan*>(pDestination->GetParentResource());
  auto pVulkanSource = static_cast<const ezGALTextureVulkan*>(pSource->GetParentResource());

  const ezGALTextureCreationDescription& destDesc = pDestination->GetDescription();
  const ezGALTextureCreationDescription& srcDesc = pSource->GetDescription();

  EZ_ASSERT_DEBUG(ezGALResourceFormat::IsDepthFormat(destDesc.m_Format) == ezGALResourceFormat::IsDepthFormat(srcDesc.m_Format), "");

  // TODO need to determine size of the subresource
  vk::ImageResolve resolveRegion = {};
  resolveRegion.dstSubresource.aspectMask = pVulkanDestination->GetFullRange().aspectMask;
  resolveRegion.dstSubresource.baseArrayLayer = DestinationSubResource.m_uiArraySlice;
  resolveRegion.dstSubresource.layerCount = 1;
  resolveRegion.dstSubresource.mipLevel = DestinationSubResource.m_uiMipLevel;
  resolveRegion.extent.width = ezMath::Min(destDesc.m_uiWidth, srcDesc.m_uiWidth);
  resolveRegion.extent.height = ezMath::Min(destDesc.m_uiHeight, srcDesc.m_uiHeight);
  resolveRegion.extent.depth = ezMath::Min(destDesc.m_uiDepth, srcDesc.m_uiDepth);
  resolveRegion.srcSubresource.aspectMask = pVulkanSource->GetFullRange().aspectMask;
  resolveRegion.srcSubresource.baseArrayLayer = SourceSubResource.m_uiArraySlice;
  resolveRegion.srcSubresource.layerCount = 1;
  resolveRegion.srcSubresource.mipLevel = SourceSubResource.m_uiMipLevel;

  if (srcDesc.m_SampleCount != ezGALMSAASampleCount::None)
  {
    m_pCommandBuffer->resolveImage(pVulkanSource->GetImage(), vk::ImageLayout::eTransferSrcOptimal, pVulkanDestination->GetImage(), vk::ImageLayout::eTransferDstOptimal, 1, &resolveRegion);
  }
  else
  {
    // DX11 allows calling resolve on a non-msaa source. For now, allow this as well in Vulkan.
    vk::Extent3D sourceMipLevelSize = pVulkanSource->GetMipLevelSize(SourceSubResource.m_uiMipLevel);
    vk::Offset3D sourceMipLevelEndOffset = {(ezInt32)sourceMipLevelSize.width, (ezInt32)sourceMipLevelSize.height, (ezInt32)sourceMipLevelSize.depth};
    vk::Extent3D dstMipLevelSize = pVulkanDestination->GetMipLevelSize(DestinationSubResource.m_uiMipLevel);
    vk::Offset3D dstMipLevelEndOffset = {(ezInt32)sourceMipLevelSize.width, (ezInt32)sourceMipLevelSize.height, (ezInt32)sourceMipLevelSize.depth};

    vk::ImageBlit imageBlitRegion;
    imageBlitRegion.srcSubresource = resolveRegion.srcSubresource;
    imageBlitRegion.srcOffsets[1] = sourceMipLevelEndOffset;
    imageBlitRegion.dstSubresource = resolveRegion.dstSubresource;
    imageBlitRegion.dstOffsets[1] = dstMipLevelEndOffset;

    m_pCommandBuffer->blitImage(pVulkanSource->GetImage(), vk::ImageLayout::eTransferSrcOptimal, pVulkanDestination->GetImage(), vk::ImageLayout::eTransferDstOptimal, 1, &imageBlitRegion, vk::Filter::eNearest);
  }
}

void ezGALCommandEncoderImplVulkan::CopyImageToBuffer(const ezGALTextureVulkan* pSource, const ezGALBufferVulkan* pDestination)
{
  CopyImageToBuffer(pSource, pDestination->GetVkBuffer());
}

void ezGALCommandEncoderImplVulkan::CopyImageToBuffer(const ezGALTextureVulkan* pSource, vk::Buffer destination)
{
  const ezGALTextureCreationDescription& textureDesc = pSource->GetDescription();
  const vk::ImageAspectFlags imageAspect = pSource->GetAspectMask();

  ezHybridArray<ezGALTextureVulkan::SubResourceOffset, 8> subResourceOffsets;
  pSource->ComputeSubResourceOffsets(&m_GALDeviceVulkan, pSource->GetDescription(), subResourceOffsets);

  ezHybridArray<vk::BufferImageCopy, 8> imageCopy;
  const ezUInt32 arraySize = (textureDesc.m_Type == ezGALTextureType::TextureCube || textureDesc.m_Type == ezGALTextureType::TextureCubeArray) ? textureDesc.m_uiArraySize * 6 : textureDesc.m_uiArraySize;
  const ezUInt32 mipLevels = textureDesc.m_uiMipLevelCount;

  for (ezUInt32 uiLayer = 0; uiLayer < arraySize; uiLayer++)
  {
    for (ezUInt32 uiMipLevel = 0; uiMipLevel < mipLevels; uiMipLevel++)
    {
      const vk::Extent3D mipLevelSize = pSource->GetMipLevelSize(uiMipLevel);
      const ezUInt32 uiSubresourceIndex = uiMipLevel + uiLayer * mipLevels;
      const ezGALTextureVulkan::SubResourceOffset& offset = subResourceOffsets[uiSubresourceIndex];

      vk::BufferImageCopy& copy = imageCopy.ExpandAndGetRef();

      copy.bufferOffset = offset.m_uiOffset;
      copy.bufferRowLength = offset.m_uiRowLength;
      copy.bufferImageHeight = offset.m_uiImageHeight;
      copy.imageSubresource.aspectMask = imageAspect;
      copy.imageSubresource.mipLevel = uiMipLevel;
      copy.imageSubresource.baseArrayLayer = uiLayer;
      copy.imageSubresource.layerCount = 1;
      copy.imageOffset = vk::Offset3D(0, 0, 0);
      copy.imageExtent = mipLevelSize;
    }
  }

  m_pCommandBuffer->copyImageToBuffer(pSource->GetImage(), vk::ImageLayout::eTransferSrcOptimal, destination, imageCopy.GetCount(), imageCopy.GetData());
}

void ezGALCommandEncoderImplVulkan::ReadbackTexturePlatform(const ezGALReadbackTexture* pDestination, const ezGALTexture* pSource)
{
  const ezGALTextureVulkan* pVulkanSourceTexture = static_cast<const ezGALTextureVulkan*>(pSource->GetParentResource());
  const ezGALReadbackTextureVulkan* pVulkanDestinationTexture = static_cast<const ezGALReadbackTextureVulkan*>(pDestination->GetParentResource());

  const ezGALTextureCreationDescription& textureDesc = pVulkanSourceTexture->GetDescription();
  const bool bMSAASourceTexture = textureDesc.m_SampleCount != ezGALMSAASampleCount::None;
  EZ_ASSERT_DEV(!bMSAASourceTexture, "MSAA read-back not implemented!");

  CopyImageToBuffer(pVulkanSourceTexture, pVulkanDestinationTexture->GetVkBuffer());

  ezBarrierUtilsVulkan barrier(m_GALDeviceVulkan, *m_pCommandBuffer);
  barrier.BufferBarrier(pVulkanDestinationTexture->GetVkBuffer(), ezGALResourceState::CopyDestination, ezGALResourceState::CpuRead);
}


void ezGALCommandEncoderImplVulkan::ReadbackBufferPlatform(const ezGALReadbackBuffer* pDestination, const ezGALBuffer* pSource)
{
  auto pDestinationVulkan = static_cast<const ezGALReadbackBufferVulkan*>(pDestination);
  auto pSourceVulkan = static_cast<const ezGALBufferVulkan*>(pSource);

  EZ_ASSERT_DEV(pSource->GetSize() == pDestination->GetSize(), "Source and destination buffer sizes mismatch!");

  vk::BufferCopy bufferCopy = {};
  bufferCopy.size = pSource->GetSize();

  m_pCommandBuffer->copyBuffer(pSourceVulkan->GetVkBuffer(), pDestinationVulkan->GetVkBuffer(), 1, &bufferCopy);

  ezBarrierUtilsVulkan barrier(m_GALDeviceVulkan, *m_pCommandBuffer);
  barrier.BufferBarrier(pDestinationVulkan->GetVkBuffer(), ezGALResourceState::CopyDestination, ezGALResourceState::CpuRead);
}

void ezGALCommandEncoderImplVulkan::FlushPlatform()
{
  m_GALDeviceVulkan.Submit();
  SetCurrentCommandBuffer(&m_GALDeviceVulkan.GetCurrentCommandBuffer());
}

void ezGALCommandEncoderImplVulkan::TextureBarrierPlatform(ezArrayPtr<const ezGALTextureBarrier> barriers)
{
  if (barriers.IsEmpty())
    return;

  ezBarrierUtilsVulkan barrierUtils(m_GALDeviceVulkan, *m_pCommandBuffer);
  barrierUtils.TextureBarrier(barriers);
}

void ezGALCommandEncoderImplVulkan::BufferBarrierPlatform(ezArrayPtr<const ezGALBufferBarrier> barriers)
{
  if (barriers.IsEmpty())
    return;

  ezBarrierUtilsVulkan barrierUtils(m_GALDeviceVulkan, *m_pCommandBuffer);
  barrierUtils.BufferBarrier(barriers);
}

// Debug helper functions

void ezGALCommandEncoderImplVulkan::PushMarkerPlatform(const char* szMarker)
{
  if (m_GALDeviceVulkan.GetExtensions().m_bDebugUtilsMarkers)
  {
    constexpr float markerColor[4] = {0, 0, 0, 0};
    vk::DebugUtilsLabelEXT markerInfo = {};
    ezMemoryUtils::Copy(markerInfo.color.data(), markerColor, EZ_ARRAY_SIZE(markerColor));
    markerInfo.pLabelName = szMarker;

    m_pCommandBuffer->beginDebugUtilsLabelEXT(markerInfo, m_GALDeviceVulkan.GetDispatchContext());
  }
}

void ezGALCommandEncoderImplVulkan::PopMarkerPlatform()
{
  if (m_GALDeviceVulkan.GetExtensions().m_bDebugUtilsMarkers)
  {
    m_pCommandBuffer->endDebugUtilsLabelEXT(m_GALDeviceVulkan.GetDispatchContext());
  }
}

void ezGALCommandEncoderImplVulkan::InsertEventMarkerPlatform(const char* szMarker)
{
  if (m_GALDeviceVulkan.GetExtensions().m_bDebugUtilsMarkers)
  {
    constexpr float markerColor[4] = {1, 1, 1, 1};
    vk::DebugUtilsLabelEXT markerInfo = {};
    ezMemoryUtils::Copy(markerInfo.color.data(), markerColor, EZ_ARRAY_SIZE(markerColor));
    markerInfo.pLabelName = szMarker;
    m_pCommandBuffer->insertDebugUtilsLabelEXT(markerInfo, m_GALDeviceVulkan.GetDispatchContext());
  }
}

//////////////////////////////////////////////////////////////////////////

void ezGALCommandEncoderImplVulkan::BeginRenderingPlatform(const ezGALRenderingSetup& renderingSetup)
{
  m_bDynamicOffsetsDirty = true;
  // #TODO_VULKAN we should always have a command buffer, so this call should not be necessary.
  m_GALDeviceVulkan.GetCurrentCommandBuffer();
  // We have to ensure we have enough queries before entering the render pass as we can't replenish pools while within.
  m_GALDeviceVulkan.GetQueryPool().EnsureFreeQueryPoolSize(*m_pCommandBuffer);

  m_renderPass.renderPass = ezResourceCacheVulkan::RequestRenderPass(renderingSetup.GetRenderPass());
  m_renderPass.framebuffer = ezResourceCacheVulkan::RequestFrameBuffer(m_renderPass.renderPass, renderingSetup.GetFrameBuffer());
  m_uiLayers = renderingSetup.GetFrameBuffer().m_uiSliceCount;
  ezSizeU32 size = renderingSetup.GetFrameBuffer().m_Size;
  SetScissorRectPlatform(ezRectU32(size.width, size.height));

  {
    m_renderPass.renderArea.offset.setX(0).setY(0);
    m_renderPass.renderArea.extent.setHeight(size.height).setWidth(size.width);

    m_clearValues.Clear();
    const bool bHasDepth = renderingSetup.HasDepthStencilTarget();
    const ezUInt32 uiColorCount = renderingSetup.GetColorTargetCount();

    if (bHasDepth)
    {
      vk::ClearValue& depthClear = m_clearValues.ExpandAndGetRef();
      depthClear.depthStencil.setDepth(renderingSetup.GetClearDepth()).setStencil(renderingSetup.GetClearStencil());

      const ezGALRenderTargetViewVulkan* pRenderTargetView = static_cast<const ezGALRenderTargetViewVulkan*>(m_GALDeviceVulkan.GetRenderTargetView(renderingSetup.GetFrameBuffer().m_hDepthTarget));
      m_depthMask = pRenderTargetView->GetRange().aspectMask;
    }
    for (ezUInt32 i = 0; i < uiColorCount; i++)
    {
      vk::ClearValue& colorClear = m_clearValues.ExpandAndGetRef();
      ezColor col = renderingSetup.GetClearColor(i);
      colorClear.color.setFloat32({col.r, col.g, col.b, col.a});
    }

    m_renderPass.clearValueCount = m_clearValues.GetCount();
    m_renderPass.pClearValues = m_clearValues.GetData();
  }

  m_bPipelineStateDirty = true;
  m_bViewportDirty = true;
  m_bScissorDirty = true;

  m_pCommandBuffer->beginRenderPass(m_renderPass, vk::SubpassContents::eInline);
}

void ezGALCommandEncoderImplVulkan::EndRenderingPlatform()
{
  m_pCommandBuffer->endRenderPass();

  m_depthMask = {};
  m_uiLayers = 0;

  m_renderPass.renderPass = nullptr;
  m_renderPass.framebuffer = nullptr;
}

void ezGALCommandEncoderImplVulkan::ClearPlatform(const ezColor& ClearColor, ezUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, ezUInt8 uiStencilClear)
{
  // #TODO_VULKAN Not sure if we need barriers here.
  ezHybridArray<vk::ClearAttachment, 8> attachments;

  // Clear color
  if (uiRenderTargetClearMask != 0)
  {
    for (ezUInt32 i = 0; i < EZ_GAL_MAX_RENDERTARGET_COUNT; i++)
    {
      if (uiRenderTargetClearMask & (1u << i) && i < m_pGraphicsPipeline->GetDescription().m_RenderPass.m_uiRTCount)
      {
        vk::ClearAttachment& attachment = attachments.ExpandAndGetRef();
        attachment.aspectMask = vk::ImageAspectFlagBits::eColor;
        attachment.clearValue.color.setFloat32({ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a});
        attachment.colorAttachment = i;
      }
    }
  }
  // Clear depth / stencil
  if ((bClearDepth || bClearStencil) && m_depthMask != vk::ImageAspectFlagBits::eNone)
  {
    vk::ClearAttachment& attachment = attachments.ExpandAndGetRef();
    if (bClearDepth && (m_depthMask & vk::ImageAspectFlagBits::eDepth))
    {
      attachment.aspectMask |= vk::ImageAspectFlagBits::eDepth;
      attachment.clearValue.depthStencil.setDepth(fDepthClear);
    }
    if (bClearStencil && (m_depthMask & vk::ImageAspectFlagBits::eStencil))
    {
      attachment.aspectMask |= vk::ImageAspectFlagBits::eStencil;
      attachment.clearValue.depthStencil.setStencil(uiStencilClear);
    }
  }

  vk::ClearRect rect;
  rect.baseArrayLayer = 0;
  rect.layerCount = m_uiLayers;
  rect.rect = m_renderPass.renderArea;
  m_pCommandBuffer->clearAttachments(attachments.GetCount(), attachments.GetData(), 1, &rect);
}

// Draw functions

ezResult ezGALCommandEncoderImplVulkan::DrawPlatform(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex)
{
  EZ_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pCommandBuffer->draw(uiVertexCount, 1, uiStartVertex, 0);
  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoderImplVulkan::DrawIndexedPlatform(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex)
{
  EZ_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pCommandBuffer->drawIndexed(uiIndexCount, 1, uiStartIndex, 0, 0);
  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoderImplVulkan::DrawIndexedInstancedPlatform(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex)
{
  EZ_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pCommandBuffer->drawIndexed(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex, 0, 0);
  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoderImplVulkan::DrawIndexedInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  EZ_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pCommandBuffer->drawIndexedIndirect(static_cast<const ezGALBufferVulkan*>(pIndirectArgumentBuffer)->GetVkBuffer(), uiArgumentOffsetInBytes, 1, 0);
  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoderImplVulkan::DrawInstancedPlatform(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex)
{
  EZ_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pCommandBuffer->draw(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex, 0);
  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoderImplVulkan::DrawInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  EZ_SUCCEED_OR_RETURN(FlushDeferredStateChanges());

  m_pCommandBuffer->drawIndirect(static_cast<const ezGALBufferVulkan*>(pIndirectArgumentBuffer)->GetVkBuffer(), uiArgumentOffsetInBytes, 1, 0);
  return EZ_SUCCESS;
}

void ezGALCommandEncoderImplVulkan::SetIndexBufferPlatform(const ezGALBuffer* pIndexBuffer)
{
  if (m_pIndexBuffer != pIndexBuffer)
  {
    m_pIndexBuffer = static_cast<const ezGALBufferVulkan*>(pIndexBuffer);
    m_bIndexBufferDirty = true;
  }
}

void ezGALCommandEncoderImplVulkan::SetVertexBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pVertexBuffer, ezUInt32 uiOffset)
{
  EZ_ASSERT_DEV(uiSlot < EZ_GAL_MAX_VERTEX_BUFFER_COUNT, "Invalid slot index");
  vk::Buffer buffer = pVertexBuffer != nullptr ? static_cast<const ezGALBufferVulkan*>(pVertexBuffer)->GetVkBuffer() : nullptr;

  if (buffer != m_pBoundVertexBuffers[uiSlot] || uiOffset != m_VertexBufferOffsets[uiSlot])
  {
    m_pBoundVertexBuffers[uiSlot] = buffer;
    m_VertexBufferOffsets[uiSlot] = uiOffset;
    m_BoundVertexBuffersRange.SetToIncludeValue(uiSlot);
  }
}

void ezGALCommandEncoderImplVulkan::SetGraphicsPipelinePlatform(const ezGALGraphicsPipeline* pGraphicsPipeline)
{
  if (m_pGraphicsPipeline != pGraphicsPipeline)
  {
    const vk::PipelineLayout oldLayout = m_pShader ? m_pShader->GetVkPipelineLayout() : vk::PipelineLayout{};
    m_pGraphicsPipeline = static_cast<const ezGALGraphicsPipelineVulkan*>(pGraphicsPipeline);
    bool bScissorEnabled = false;
    if (m_pGraphicsPipeline)
    {
      m_pShader = static_cast<const ezGALShaderVulkan*>(m_GALDeviceVulkan.GetShader(m_pGraphicsPipeline->GetDescription().m_hShader));
      bScissorEnabled = m_GALDeviceVulkan.GetRasterizerState(m_pGraphicsPipeline->GetDescription().m_hRasterizerState)->GetDescription().m_bScissorTest;
    }
    // When the pipeline layout changes, previously bound descriptor sets are invalidated by Vulkan
    // (see Vulkan spec 14.2.2 "compatibility for set N"). Force a full rebind.
    const vk::PipelineLayout newLayout = m_pShader ? m_pShader->GetVkPipelineLayout() : vk::PipelineLayout{};
    if (newLayout != oldLayout)
    {
      for (ezUInt32 i = 0; i < EZ_GAL_MAX_BIND_GROUPS; i++)
      {
        m_BindGroupDirty[i] = true;
      }
    }
    if (bScissorEnabled != m_bScissorEnabled)
    {
      // Whether scissor follows m_scissor or is derived from m_viewport changes, so the scissor must be re-applied.
      m_bScissorDirty = true;
      m_bScissorEnabled = bScissorEnabled;
    }
    m_bPipelineStateDirty = true;
  }
}

void ezGALCommandEncoderImplVulkan::SetComputePipelinePlatform(const ezGALComputePipeline* pComputePipeline)
{
  if (m_pComputePipeline != pComputePipeline)
  {
    const vk::PipelineLayout oldLayout = m_pShader ? m_pShader->GetVkPipelineLayout() : vk::PipelineLayout{};
    m_pComputePipeline = static_cast<const ezGALComputePipelineVulkan*>(pComputePipeline);
    if (m_pComputePipeline)
    {
      m_pShader = static_cast<const ezGALShaderVulkan*>(m_GALDeviceVulkan.GetShader(m_pComputePipeline->GetDescription().m_hShader));
    }
    const vk::PipelineLayout newLayout = m_pShader ? m_pShader->GetVkPipelineLayout() : vk::PipelineLayout{};
    if (newLayout != oldLayout)
    {
      for (ezUInt32 i = 0; i < EZ_GAL_MAX_BIND_GROUPS; i++)
      {
        m_BindGroupDirty[i] = true;
      }
    }
    m_bPipelineStateDirty = true;
  }
}

void ezGALCommandEncoderImplVulkan::SetViewportPlatform(const ezRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  // We use ezClipSpaceYMode::Regular and rely in the Vulkan 1.1 feature that a negative height performs y-inversion of the clip-space to framebuffer-space transform.
  // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VK_KHR_maintenance1.html
  vk::Viewport viewport = {rect.x, rect.height + rect.y, rect.width, -rect.height, fMinDepth, fMaxDepth};
  if (m_viewport != viewport)
  {
    // Viewport is marked as dynamic in the pipeline layout and thus does not mark m_bPipelineStateDirty.
    m_viewport = viewport;
    m_bViewportDirty = true;
    if (!m_bScissorEnabled)
    {
      // When scissor test is disabled the scissor rect is derived from the viewport, so it must be re-applied too.
      m_bScissorDirty = true;
    }
  }
}

void ezGALCommandEncoderImplVulkan::SetScissorRectPlatform(const ezRectU32& rect)
{
  vk::Rect2D scissor(vk::Offset2D(rect.x, rect.y), vk::Extent2D(rect.width, rect.height));
  if (m_scissor != scissor)
  {
    m_scissor = scissor;
    m_bScissorDirty = true;
  }
}

void ezGALCommandEncoderImplVulkan::SetStencilReferencePlatform(ezUInt8 uiStencilRefValue)
{
  // The actual setStencilReference call is deferred to FlushDeferredStateChanges so it can be skipped if the bound graphics pipeline has no stencil test.
  if (m_uiStencilRefValue != uiStencilRefValue)
  {
    m_uiStencilRefValue = uiStencilRefValue;
    m_bStencilRefDirty = true;
  }
}

//////////////////////////////////////////////////////////////////////////

void ezGALCommandEncoderImplVulkan::BeginComputePlatform()
{
  m_GALDeviceVulkan.GetCurrentCommandBuffer();

  m_bInsideCompute = true;
  m_bPipelineStateDirty = true;
  m_bDynamicOffsetsDirty = true;
}

void ezGALCommandEncoderImplVulkan::EndComputePlatform()
{
  m_bInsideCompute = false;
}

ezResult ezGALCommandEncoderImplVulkan::DispatchPlatform(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ)
{
  EZ_SUCCEED_OR_RETURN(FlushDeferredStateChanges());
  m_pCommandBuffer->dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoderImplVulkan::DispatchIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  EZ_SUCCEED_OR_RETURN(FlushDeferredStateChanges());
  m_pCommandBuffer->dispatchIndirect(static_cast<const ezGALBufferVulkan*>(pIndirectArgumentBuffer)->GetVkBuffer(), uiArgumentOffsetInBytes);
  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoderImplVulkan::FlushDeferredStateChanges()
{
  EZ_PROFILE_SCOPE("FlushDeferredStateChanges");
  if (m_bPipelineStateDirty)
  {
    vk::Pipeline pipeline;
    if (m_bInsideCompute)
    {
      pipeline = m_pComputePipeline->GetPipeline();
    }
    else
    {
      pipeline = m_pGraphicsPipeline->GetPipeline();
    }

    m_pCommandBuffer->bindPipeline(m_bInsideCompute ? vk::PipelineBindPoint::eCompute : vk::PipelineBindPoint::eGraphics, pipeline);
    m_bPipelineStateDirty = false;
  }

  if (!m_bInsideCompute && m_bViewportDirty)
  {
    m_pCommandBuffer->setViewport(0, 1, &m_viewport);
    m_bViewportDirty = false;
  }

  if (!m_bInsideCompute && m_bScissorDirty)
  {
    if (m_bScissorEnabled)
    {
      m_pCommandBuffer->setScissor(0, 1, &m_scissor);
    }
    else
    {
      vk::Rect2D noScissor({int(m_viewport.x), int(m_viewport.y + m_viewport.height)}, {ezUInt32(m_viewport.width), ezUInt32(-m_viewport.height)});
      m_pCommandBuffer->setScissor(0, 1, &noScissor);
    }
    m_bScissorDirty = false;
  }

  if (!m_bInsideCompute && m_BoundVertexBuffersRange.IsValid())
  {
    const ezUInt32 uiStartSlot = m_BoundVertexBuffersRange.m_uiMin;
    const ezUInt32 uiNumSlots = m_BoundVertexBuffersRange.GetCount();

    ezUInt32 uiCurrentStartSlot = uiStartSlot;
    // Finding valid ranges.
    for (ezUInt32 i = uiStartSlot; i < (uiStartSlot + uiNumSlots); i++)
    {
      if (!m_pBoundVertexBuffers[i])
      {
        if (i - uiCurrentStartSlot > 0)
        {
          // There are some null elements in the array. We can't submit these to Vulkan and need to skip them so flush everything before it.
          m_pCommandBuffer->bindVertexBuffers(uiCurrentStartSlot, i - uiCurrentStartSlot, m_pBoundVertexBuffers + uiCurrentStartSlot, m_VertexBufferOffsets + uiCurrentStartSlot);
        }
        uiCurrentStartSlot = i + 1;
      }
    }
    // The last element in the buffer range must always be valid so we can simply flush the rest.
    if (m_pBoundVertexBuffers[uiCurrentStartSlot])
      m_pCommandBuffer->bindVertexBuffers(uiCurrentStartSlot, m_BoundVertexBuffersRange.m_uiMax - uiCurrentStartSlot + 1, m_pBoundVertexBuffers + uiCurrentStartSlot, m_VertexBufferOffsets + uiCurrentStartSlot);

    m_BoundVertexBuffersRange.Reset();
  }

  if (!m_bInsideCompute && m_bIndexBufferDirty)
  {
    if (m_pIndexBuffer)
      m_pCommandBuffer->bindIndexBuffer(m_pIndexBuffer->GetVkBuffer(), 0, m_pIndexBuffer->GetIndexType());
    m_bIndexBufferDirty = false;
  }

  // Bind Groups
  {
    // Only bind groups that have changes or higher indices than those that have changed need to be updated. Thus, we track the first bind group index with actual changes here.
    ezUInt32 uiFirstChangedBindGroup = EZ_GAL_MAX_BIND_GROUPS;
    const ezUInt32 uiBindGroups = m_pShader->GetBindGroupCount();
    bool bAnyBindGroupDirty = false;
    for (ezUInt32 uiBindGroup = 0; uiBindGroup < uiBindGroups; ++uiBindGroup)
    {
      // Is a bind group resource bound to this set?
      if (m_pBindGroups[uiBindGroup] != nullptr)
      {
        if (m_pShader->GetBindGroupLayout(uiBindGroup) != m_pBindGroups[uiBindGroup]->GetDescription().m_hBindGroupLayout)
        {
          ezLog::Error("Bind group resource layout missmatch");
          return EZ_FAILURE;
        }
        if (m_BindGroupDirty[uiBindGroup])
        {
          uiFirstChangedBindGroup = ezMath::Min(uiFirstChangedBindGroup, uiBindGroup);
          m_BindGroupDirty[uiBindGroup] = false;
          bAnyBindGroupDirty = true;
          m_DescriptorSets[uiBindGroup] = m_pBindGroups[uiBindGroup]->GetDescriptorSet();
          EnsureBindGroupTextureLayout(m_pBindGroups[uiBindGroup]->GetDescription());
        }
        continue;
      }

      if (m_pShader->GetBindGroupLayout(uiBindGroup) != m_BindGroups[uiBindGroup].m_hBindGroupLayout)
      {
        ezLog::Error("Bind group layout missmatch");
        return EZ_FAILURE;
      }

      if (m_BindGroupDirty[uiBindGroup])
      {
        uiFirstChangedBindGroup = ezMath::Min(uiFirstChangedBindGroup, uiBindGroup);
        m_BindGroupDirty[uiBindGroup] = false;
        bAnyBindGroupDirty = true;
        // Need to call FindDynamicUniformBuffers first to generate the m_DynamicUniformVkBuffers list which is needed for the hash lookup inside CreateTransientDescriptorSet.
        FindDynamicUniformBuffers(m_BindGroups[uiBindGroup], m_DynamicOffsets[uiBindGroup]);
        m_DescriptorSets[uiBindGroup] = CreateDescriptorSet(m_BindGroups[uiBindGroup], m_DynamicOffsets[uiBindGroup]);
        EnsureBindGroupTextureLayout(m_BindGroups[uiBindGroup]);
        continue;
      }

      if (m_bDynamicOffsetsDirty)
      {
        // If a dynamic uniform buffer is updated, it can cause it's location to bump forward in the same buffer (OffsetsChanged) or if the old buffer is full, it can be moved to a new buffer (BuffersChanged), in which case a new descriptor set must be created.
        const DynamicUniformBufferChanges changes = UpdateDynamicUniformBufferOffsets(m_DynamicOffsets[uiBindGroup]);
        switch (changes)
        {
          case DynamicUniformBufferChanges::None:
            break;
          case DynamicUniformBufferChanges::BuffersChanged:
            m_Statistics.m_uiDynamicUniformBufferChanged++;
            m_DescriptorSets[uiBindGroup] = CreateDescriptorSet(m_BindGroups[uiBindGroup], m_DynamicOffsets[uiBindGroup]);
            [[fallthrough]];
          case DynamicUniformBufferChanges::OffsetsChanged:
            uiFirstChangedBindGroup = ezMath::Min(uiFirstChangedBindGroup, uiBindGroup);
            break;
        }
      }
    }

    if (m_bDynamicOffsetsDirty || bAnyBindGroupDirty)
    {
      ezHybridArray<ezUInt32, 8> dynamicUniformBufferOffsets;
      for (ezUInt32 uiBindGroup = uiFirstChangedBindGroup; uiBindGroup < uiBindGroups; ++uiBindGroup)
      {
        dynamicUniformBufferOffsets.PushBackRange(m_DynamicOffsets[uiBindGroup].m_DynamicUniformBufferOffsets);
      }

      // Pending descriptor writes. Must be executed before bind call unless VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT is used (Vulkan 1.2). https://registry.khronos.org/vulkan/specs/latest/man/html/VkDescriptorSetLayoutBindingFlagsCreateInfo.html
      if (ezUInt32 uiWrites = m_pWritePool->FlushWrites(); uiWrites != 0)
      {
        m_Statistics.m_uiDescriptorSetsUpdated++;
        m_Statistics.m_uiDescriptorWrites += uiWrites;
      }

      if (uiFirstChangedBindGroup != EZ_GAL_MAX_BIND_GROUPS)
      {
        m_pCommandBuffer->bindDescriptorSets(m_bInsideCompute ? vk::PipelineBindPoint::eCompute : vk::PipelineBindPoint::eGraphics, m_pShader->GetVkPipelineLayout(), uiFirstChangedBindGroup, uiBindGroups - uiFirstChangedBindGroup, m_DescriptorSets + uiFirstChangedBindGroup, dynamicUniformBufferOffsets.GetCount(), dynamicUniformBufferOffsets.GetData());
      }
      m_bDynamicOffsetsDirty = false;
    }
  }

  // Push Constants
  if (m_bPushConstantsDirty && m_pShader->GetPushConstantRange().size > 0)
  {
    if (m_pShader->GetPushConstantRange().size <= m_PushConstants.GetCount())
    {
      m_pCommandBuffer->pushConstants(m_pShader->GetVkPipelineLayout(), m_pShader->GetPushConstantRange().stageFlags, m_pShader->GetPushConstantRange().offset, m_pShader->GetPushConstantRange().size, m_PushConstants.GetData());
    }
    else
    {
      ezLog::Warning("Push constant size mismatch: shader expects {} bytes but only {} bytes were provided. Skipping push constants.",
        m_pShader->GetPushConstantRange().size, m_PushConstants.GetCount());
    }
  }
  m_bPushConstantsDirty = false;

  // Stencil reference is dynamic state and depends on whether the currently bound pipeline has a stencil test, so it must be applied after the pipeline is bound.
  if (!m_bInsideCompute && m_bStencilRefDirty && m_pGraphicsPipeline && m_pGraphicsPipeline->HasStencilTest())
  {
    m_pCommandBuffer->setStencilReference(vk::StencilFaceFlagBits::eFrontAndBack, m_uiStencilRefValue);
    m_bStencilRefDirty = false;
  }

  return EZ_SUCCESS;
}

vk::DescriptorSet ezGALCommandEncoderImplVulkan::CreateDescriptorSet(const ezGALBindGroupCreationDescription& desc, const DynamicOffsets& offsets)
{
  // Hash current description
  ezUInt64 uiHash = HashBindGroup(desc, offsets);

  // Look up Hash
  vk::DescriptorSet descriptorSet;
  if (m_DescriptorCache.TryGetValue(uiHash, descriptorSet))
  {
    m_Statistics.m_uiDescriptorSetsReused++;
    return descriptorSet;
  }

  // Create new descriptor set
  m_Statistics.m_uiDescriptorSetsCreated++;
  const ezGALBindGroupLayoutVulkan* pLayout = static_cast<const ezGALBindGroupLayoutVulkan*>(m_GALDeviceVulkan.GetBindGroupLayout(desc.m_hBindGroupLayout));
  descriptorSet = ezTransientDescriptorSetPoolVulkan::CreateTransientDescriptorSet(pLayout->GetDescriptorSetLayout());
  m_pWritePool->WriteTransientDescriptor(descriptorSet, desc, m_pUniformBufferPool.Borrow());

  m_DescriptorCache.Insert(uiHash, descriptorSet);
  return descriptorSet;
}

void ezGALCommandEncoderImplVulkan::EnsureBindGroupTextureLayout(const ezGALBindGroupCreationDescription& desc)
{
  const ezGALBindGroupLayoutVulkan* pLayout = static_cast<const ezGALBindGroupLayoutVulkan*>(m_GALDeviceVulkan.GetBindGroupLayout(desc.m_hBindGroupLayout));
  ezArrayPtr<const ezShaderResourceBinding> bindings = pLayout->GetDescription().m_ResourceBindings;
  const ezUInt32 uiBindings = bindings.GetCount();
  for (ezUInt32 i = 0; i < uiBindings; ++i)
  {
    const ezShaderResourceBinding& binding = bindings[i];
    const ezGALBindGroupItem& item = desc.m_BindGroupItems[i];

    switch (binding.m_ResourceType)
    {
      case ezGALShaderResourceType::Texture:
      case ezGALShaderResourceType::TextureRW:
      case ezGALShaderResourceType::TextureAndSampler:
      {
        const ezGALTextureVulkan* pTexture = static_cast<const ezGALTextureVulkan*>(m_GALDeviceVulkan.GetTexture(item.m_Texture.m_hTexture));
        const bool bIsDepthTexture = ezConversionUtilsVulkan::IsDepthFormat(pTexture->GetImageFormat());
        const ezBitflags<ezGALResourceState> targetState = binding.m_ResourceType == ezGALShaderResourceType::TextureRW
                                                             ? ezGALResourceState::UnorderedAccess
                                                             : (bIsDepthTexture ? ezGALResourceState::DepthStencilRead : ezGALResourceState::ShaderResource);
      }
      break;
      default:
        break;
    }
  }
}

void ezGALCommandEncoderImplVulkan::FindDynamicUniformBuffers(const ezGALBindGroupCreationDescription& desc, ezGALCommandEncoderImplVulkan::DynamicOffsets& out_offsets)
{
  out_offsets.m_DynamicUniformBuffers.Clear();
  out_offsets.m_DynamicUniformVkBuffers.Clear();
  out_offsets.m_DynamicUniformBufferOffsets.Clear();

  const ezGALBindGroupLayoutVulkan* pLayout = static_cast<const ezGALBindGroupLayoutVulkan*>(m_GALDeviceVulkan.GetBindGroupLayout(desc.m_hBindGroupLayout));
  ezArrayPtr<const ezShaderResourceBinding> bindings = pLayout->GetDescription().m_ResourceBindings;
  const ezUInt32 uiBindings = bindings.GetCount();
  for (ezUInt32 i = 0; i < uiBindings; ++i)
  {
    const ezShaderResourceBinding& binding = bindings[i];
    const ezGALBindGroupItem& item = desc.m_BindGroupItems[i];

    switch (binding.m_ResourceType)
    {
      case ezGALShaderResourceType::ConstantBuffer:
      {
        const ezGALBufferVulkan* pBuffer = static_cast<const ezGALBufferVulkan*>(m_GALDeviceVulkan.GetBuffer(item.m_Buffer.m_hBuffer));
        if (pBuffer->GetDescription().m_BufferFlags.IsSet(ezGALBufferUsageFlags::Transient))
        {
          out_offsets.m_DynamicUniformBuffers.PushBack(pBuffer);
          const vk::DescriptorBufferInfo* pBufferInfo = m_pUniformBufferPool->GetBuffer(pBuffer);
          out_offsets.m_DynamicUniformVkBuffers.PushBack(pBufferInfo->buffer);
          out_offsets.m_DynamicUniformBufferOffsets.PushBack(static_cast<ezUInt32>(pBufferInfo->offset));
        }
        else
        {
          // Non-transient buffers are handled like dynamic uniform buffers but the offset is fixed so no need to track the pointers.
          out_offsets.m_DynamicUniformBuffers.PushBack(nullptr);
          out_offsets.m_DynamicUniformVkBuffers.PushBack(nullptr);
          out_offsets.m_DynamicUniformBufferOffsets.PushBack(static_cast<ezUInt32>(item.m_Buffer.m_BufferRange.m_uiByteOffset));
        }
      }
      break;
      default:
        break;
    }
  }
  UpdateDynamicUniformBufferOffsets(out_offsets);
}

ezUInt64 ezGALCommandEncoderImplVulkan::HashBindGroup(const ezGALBindGroupCreationDescription& desc, const ezGALCommandEncoderImplVulkan::DynamicOffsets& offsets)
{
  // We only need to hash the bind group layout, the items and the current set of dynamic uniform buffers.
  ezHashStreamWriter64 writer;
  writer << desc.m_hBindGroupLayout.GetInternalID().m_Data;
  if (!desc.m_BindGroupItems.IsEmpty())
  {
    auto data = desc.m_BindGroupItems.GetByteArrayPtr();
    writer.WriteBytes(data.GetPtr(), data.GetCount()).IgnoreResult();
  }
  if (!offsets.m_DynamicUniformVkBuffers.IsEmpty())
  {
    auto data = offsets.m_DynamicUniformVkBuffers.GetByteArrayPtr();
    writer.WriteBytes(data.GetPtr(), data.GetCount()).IgnoreResult();
  }
  return writer.GetHashValue();
}

ezGALCommandEncoderImplVulkan::DynamicUniformBufferChanges ezGALCommandEncoderImplVulkan::UpdateDynamicUniformBufferOffsets(ezGALCommandEncoderImplVulkan::DynamicOffsets& ref_offsets)
{
  bool bBuffersChanged = false;
  bool bOffsetsChanged = false;
  ezUInt32 uiCount = ref_offsets.m_DynamicUniformBuffers.GetCount();
  for (ezUInt32 i = 0; i < uiCount; ++i)
  {
    const ezGALBufferVulkan* pBuffer = ref_offsets.m_DynamicUniformBuffers[i];
    if (pBuffer == nullptr)
    {
      // Non-transient buffers are not tracked here and will always have a fixed offset defined in FindDynamicUniformBuffers.
      continue;
    }
    const vk::DescriptorBufferInfo* pBufferInfo = m_pUniformBufferPool->GetBuffer(pBuffer);
    if (ref_offsets.m_DynamicUniformVkBuffers[i] != pBufferInfo->buffer)
    {
      ref_offsets.m_DynamicUniformVkBuffers[i] = pBufferInfo->buffer;
      bBuffersChanged = true;
    }
    if (ref_offsets.m_DynamicUniformBufferOffsets[i] != pBufferInfo->offset)
    {
      ref_offsets.m_DynamicUniformBufferOffsets[i] = static_cast<ezUInt32>(pBufferInfo->offset);
      bOffsetsChanged = true;
    }
  }

  if (bBuffersChanged)
    return DynamicUniformBufferChanges::BuffersChanged;

  return bOffsetsChanged ? DynamicUniformBufferChanges::OffsetsChanged : DynamicUniformBufferChanges::None;
}

ezGALCommandEncoderImplVulkan::Statistics ezGALCommandEncoderImplVulkan::GetAndResetStatistics()
{
  Statistics stats = m_Statistics;
  m_Statistics = {};
  return stats;
}

ezDescriptorWritePoolVulkan& ezGALCommandEncoderImplVulkan::GetDescriptorWritePool() const
{
  return *m_pWritePool.Borrow();
}
