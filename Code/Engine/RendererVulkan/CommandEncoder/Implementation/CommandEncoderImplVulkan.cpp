#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/RendererFallbackResources.h>
#include <RendererFoundation/Shader/BindGroup.h>
#include <RendererFoundation/Shader/BindGroupLayout.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/CommandEncoder/CommandEncoderImplVulkan.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/Pools/DescriptorSetPoolVulkan.h>
#include <RendererVulkan/Pools/FencePoolVulkan.h>
#include <RendererVulkan/Pools/QueryPoolVulkan.h>
#include <RendererVulkan/Pools/StagingBufferPoolVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/ReadbackBufferVulkan.h>
#include <RendererVulkan/Resources/ReadbackTextureVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Shader/BindGroupLayoutVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>
#include <RendererVulkan/State/ComputePipelineVulkan.h>
#include <RendererVulkan/State/GraphicsPipelineVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>
#include <RendererVulkan/Utils/ImageCopyVulkan.h>
#include <RendererVulkan/Utils/PipelineBarrierVulkan.h>

namespace
{
  bool UsesClearOp(const ezGALRenderingSetup& renderingSetup)
  {
    if (renderingSetup.HasDepthStencilTarget())
    {
      if (renderingSetup.GetRenderPass().m_DepthLoadOp == ezGALRenderTargetLoadOp::Clear || renderingSetup.GetRenderPass().m_StencilLoadOp == ezGALRenderTargetLoadOp::Clear)
        return true;
    }

    const ezUInt32 uiRTs = renderingSetup.GetRenderPass().m_uiRTCount;
    for (ezUInt32 i = 0; i < uiRTs; ++i)
    {
      if (renderingSetup.GetRenderPass().m_ColorLoadOp[i] == ezGALRenderTargetLoadOp::Clear)
        return true;
    }

    return false;
  }
} // namespace

ezGALCommandEncoderImplVulkan::ezGALCommandEncoderImplVulkan(ezGALDeviceVulkan& device)
  : m_GALDeviceVulkan(device)
{
  m_vkDevice = device.GetVulkanDevice();
  m_pUniformBufferPool = EZ_NEW(device.GetAllocator(), ezUniformBufferPoolVulkan, &device);
  m_pUniformBufferPool->Initialize();
}

ezGALCommandEncoderImplVulkan::~ezGALCommandEncoderImplVulkan()
{
  m_pUniformBufferPool->DeInitialize();
  m_pUniformBufferPool = nullptr;
}

void ezGALCommandEncoderImplVulkan::Reset()
{
  EZ_ASSERT_DEBUG(!m_bRenderPassActive, "Render pass was not closed");

  m_bPipelineStateDirty = true;
  m_bViewportDirty = true;
  m_bIndexBufferDirty = true;
  m_bDescriptorsDirty = true;
  m_BoundVertexBuffersRange.Reset();

  m_pShader = nullptr;
  m_pGraphicsPipeline = nullptr;
  m_pComputePipeline = nullptr;

  m_viewport = vk::Viewport();
  m_scissor = vk::Rect2D();

  for (ezUInt32 i = 0; i < EZ_GAL_MAX_RENDERTARGET_COUNT; i++)
  {
    m_pBoundRenderTargets[i] = nullptr;
  }
  m_pBoundDepthStencilTarget = nullptr;
  m_uiBoundRenderTargetCount = 0;

  m_pIndexBuffer = nullptr;
  for (ezUInt32 i = 0; i < EZ_GAL_MAX_VERTEX_BUFFER_COUNT; i++)
  {
    m_pBoundVertexBuffers[i] = nullptr;
    m_VertexBufferOffsets[i] = 0;
  }

  for (ezUInt32 i = 0; i < EZ_GAL_MAX_BIND_GROUPS; i++)
  {
    m_BindGroups[i].m_Desc.m_hBindGroupLayout = {};
    m_BindGroups[i].m_Desc.m_BindGroupItems.Clear();
    m_BindGroups[i].m_DynamicUniformBufferOffsets.Clear();
    m_BindGroups[i].m_DynamicUniformBuffers.Clear();
  }

  m_renderPass = vk::RenderPassBeginInfo();
  m_clearValues.Clear();
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
  m_pPipelineBarrier = nullptr;

  // We can't carry state across individual command buffers, so mark all state as dirty.
  EZ_ASSERT_DEBUG(!m_bRenderPassActive, "Render pass was not closed");

  m_bPipelineStateDirty = true;
  m_bViewportDirty = true;
  m_bIndexBufferDirty = true;
  m_bDescriptorsDirty = true;
  m_BoundVertexBuffersRange.Reset();
  for (ezUInt32 i = 0; i < EZ_GAL_MAX_VERTEX_BUFFER_COUNT; i++)
  {
    if (m_pBoundVertexBuffers[i])
      m_BoundVertexBuffersRange.SetToIncludeValue(i);
  }

  // Fence logic
  m_GALDeviceVulkan.GetFenceQueue().FenceSubmitted(submitFence);
}

void ezGALCommandEncoderImplVulkan::SetCurrentCommandBuffer(vk::CommandBuffer* commandBuffer, ezPipelineBarrierVulkan* pipelineBarrier)
{
  m_pCommandBuffer = commandBuffer;
  m_pPipelineBarrier = pipelineBarrier;
}

// State setting functions

void ezGALCommandEncoderImplVulkan::SetBindGroupPlatform(ezUInt32 uiBindGroup, const ezGALBindGroupCreationDescription& bindGroup)
{
  m_BindGroups[uiBindGroup].m_Desc = bindGroup;
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

  EZ_ASSERT_DEV(pSource->GetSize() != pDestination->GetSize(), "Source and destination buffer sizes mismatch!");

  vk::BufferCopy bufferCopy = {};
  bufferCopy.size = pSource->GetSize();

  m_pPipelineBarrier->AccessBuffer(pSourceVulkan, 0, bufferCopy.size, pSourceVulkan->GetUsedByPipelineStage(), pSourceVulkan->GetAccessMask(), vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferRead);
  m_pPipelineBarrier->AccessBuffer(pDestinationVulkan, 0, bufferCopy.size, pDestinationVulkan->GetUsedByPipelineStage(), pDestinationVulkan->GetAccessMask(), vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite);

  m_pPipelineBarrier->Flush();

  m_pCommandBuffer->copyBuffer(pSourceVulkan->GetVkBuffer(), pDestinationVulkan->GetVkBuffer(), 1, &bufferCopy);

  m_pPipelineBarrier->AccessBuffer(pSourceVulkan, 0, bufferCopy.size, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferRead, pSourceVulkan->GetUsedByPipelineStage(), pSourceVulkan->GetAccessMask());
  m_pPipelineBarrier->AccessBuffer(pDestinationVulkan, 0, bufferCopy.size, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, pDestinationVulkan->GetUsedByPipelineStage(), pDestinationVulkan->GetAccessMask());
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

  m_pPipelineBarrier->AccessBuffer(pSourceVulkan, 0, bufferCopy.size, pSourceVulkan->GetUsedByPipelineStage(), pSourceVulkan->GetAccessMask(), vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferRead);
  m_pPipelineBarrier->AccessBuffer(pDestinationVulkan, 0, bufferCopy.size, pDestinationVulkan->GetUsedByPipelineStage(), pDestinationVulkan->GetAccessMask(), vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite);
  m_pPipelineBarrier->Flush();

  m_pCommandBuffer->copyBuffer(pSourceVulkan->GetVkBuffer(), pDestinationVulkan->GetVkBuffer(), 1, &bufferCopy);

  m_pPipelineBarrier->AccessBuffer(pSourceVulkan, 0, bufferCopy.size, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferRead, pDestinationVulkan->GetUsedByPipelineStage(), pDestinationVulkan->GetAccessMask());
  m_pPipelineBarrier->AccessBuffer(pDestinationVulkan, 0, bufferCopy.size, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, pDestinationVulkan->GetUsedByPipelineStage(), pDestinationVulkan->GetAccessMask());
}

void ezGALCommandEncoderImplVulkan::UpdateBufferPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> pSourceData, ezGALUpdateMode::Enum updateMode)
{
  // EZ_CHECK_ALIGNMENT(pSourceData.GetPtr(), 16);

  auto pVulkanDestination = static_cast<const ezGALBufferVulkan*>(pDestination);
  switch (updateMode)
  {
    case ezGALUpdateMode::TransientConstantBuffer:
      EZ_ASSERT_DEBUG(pDestination->GetDescription().m_BufferFlags.AreAllSet(ezGALBufferUsageFlags::Transient | ezGALBufferUsageFlags::ConstantBuffer), "Only transient constant buffer can make use of TransientConstantBuffer update mode");
      EZ_ASSERT_DEBUG(uiDestOffset == 0, "Offset not supported");
      EZ_ASSERT_DEBUG(pVulkanDestination->GetDescription().m_uiTotalSize == pSourceData.GetCount(), "Transient buffers must be updated in their entirety");
      m_pUniformBufferPool->UpdateBuffer(pVulkanDestination, pSourceData);
      break;
    case ezGALUpdateMode::AheadOfTime:
      m_GALDeviceVulkan.GetInitContext().UpdateBuffer(pVulkanDestination, uiDestOffset, pSourceData);
      break;
  }
}

void ezGALCommandEncoderImplVulkan::CopyTexturePlatform(const ezGALTexture* pDestination, const ezGALTexture* pSource)
{
  if (m_bRenderPassActive)
  {
    m_pCommandBuffer->endRenderPass();
    m_bRenderPassActive = false;
  }

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

  m_pPipelineBarrier->EnsureImageLayout(source, vk::ImageLayout::eTransferSrcOptimal, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferRead);
  m_pPipelineBarrier->EnsureImageLayout(destination, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite);
  m_pPipelineBarrier->Flush();

  ezHybridArray<vk::ImageCopy, 14> imageCopies;

  for (ezUInt32 i = 0; i < destDesc.m_uiMipLevelCount; ++i)
  {
    vk::ImageCopy& imageCopy = imageCopies.ExpandAndGetRef();
    imageCopy.dstOffset = vk::Offset3D();
    imageCopy.dstSubresource.aspectMask = imageAspect;
    imageCopy.dstSubresource.baseArrayLayer = 0;
    imageCopy.dstSubresource.layerCount = destDesc.m_uiArraySize;
    imageCopy.dstSubresource.mipLevel = i;
    imageCopy.extent = destination->GetMipLevelSize(i);
    imageCopy.srcOffset = vk::Offset3D();
    imageCopy.srcSubresource.aspectMask = imageAspect;
    imageCopy.srcSubresource.baseArrayLayer = 0;
    imageCopy.srcSubresource.layerCount = srcDesc.m_uiArraySize;
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
  imageCopy.srcSubresource.baseArrayLayer = DestinationSubResource.m_uiArraySlice;
  imageCopy.srcSubresource.layerCount = 1;
  imageCopy.srcSubresource.mipLevel = SourceSubResource.m_uiMipLevel;

  m_pCommandBuffer->copyImage(source->GetImage(), vk::ImageLayout::eGeneral, destination->GetImage(), vk::ImageLayout::eGeneral, 1, &imageCopy);
}

void ezGALCommandEncoderImplVulkan::UpdateTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
  const ezBoundingBoxu32& DestinationBox, const ezGALSystemMemoryDescription& data)
{
  const ezGALTextureVulkan* pDestVulkan = static_cast<const ezGALTextureVulkan*>(pDestination);
  vk::ImageSubresourceRange range = pDestVulkan->GetFullRange();
  range.baseArrayLayer = DestinationSubResource.m_uiArraySlice;
  range.baseMipLevel = DestinationSubResource.m_uiMipLevel;
  range.levelCount = 1;
  range.layerCount = 1;
  m_pPipelineBarrier->EnsureImageLayout(pDestVulkan, range, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite);
  m_pPipelineBarrier->Flush();

  ezUInt32 uiWidth = ezMath::Max(DestinationBox.m_vMax.x - DestinationBox.m_vMin.x, 1u);
  ezUInt32 uiHeight = ezMath::Max(DestinationBox.m_vMax.y - DestinationBox.m_vMin.y, 1u);
  ezUInt32 uiDepth = ezMath::Max(DestinationBox.m_vMax.z - DestinationBox.m_vMin.z, 1u);

  const vk::Format format = pDestVulkan->GetImageFormat();
  const ezUInt8 uiBlockSize = vk::blockSize(format);
  const auto blockExtent = vk::blockExtent(format);
  const VkExtent3D blockCount = {
    (uiWidth + blockExtent[0] - 1) / blockExtent[0],
    (uiHeight + blockExtent[1] - 1) / blockExtent[1],
    (uiDepth + blockExtent[2] - 1) / blockExtent[2]};

  const vk::DeviceSize uiTotalSize = uiBlockSize * blockCount.width * blockCount.height * blockCount.depth;
  ezStagingBufferVulkan stagingBuffer = m_GALDeviceVulkan.GetStagingBufferPool().AllocateBuffer(uiTotalSize);

  const ezUInt32 uiBufferRowPitch = uiBlockSize * blockCount.width;
  const ezUInt32 uiBufferSlicePitch = uiBufferRowPitch * blockCount.height;
  EZ_ASSERT_DEV(uiBufferRowPitch == data.m_uiRowPitch, "Row pitch with padding is not implemented yet.");

  EZ_ASSERT_DEBUG(data.m_pData.GetCount() >= uiTotalSize, "Not enough data provided to update texture");
  ezMemoryUtils::RawByteCopy(stagingBuffer.m_Data.GetPtr(), data.m_pData.GetPtr(), uiTotalSize);

  vk::BufferImageCopy region = {};
  region.imageSubresource.aspectMask = range.aspectMask;
  region.imageSubresource.mipLevel = range.baseMipLevel;
  region.imageSubresource.baseArrayLayer = range.baseArrayLayer;
  region.imageSubresource.layerCount = range.layerCount;

  region.imageOffset = vk::Offset3D(DestinationBox.m_vMin.x, DestinationBox.m_vMin.y, DestinationBox.m_vMin.z);
  region.imageExtent = vk::Extent3D(uiWidth, uiHeight, uiDepth);

  region.bufferOffset = stagingBuffer.m_uiOffset;
  region.bufferRowLength = blockExtent[0] * uiBufferRowPitch / uiBlockSize;
  region.bufferImageHeight = blockExtent[1] * uiBufferSlicePitch / uiBufferRowPitch;

  m_pCommandBuffer->copyBufferToImage(stagingBuffer.m_buffer, pDestVulkan->GetImage(), pDestVulkan->GetPreferredLayout(vk::ImageLayout::eTransferDstOptimal), 1, &region);
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

  m_pPipelineBarrier->EnsureImageLayout(pVulkanSource, vk::ImageLayout::eTransferSrcOptimal, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferRead);
  m_pPipelineBarrier->EnsureImageLayout(pVulkanDestination, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite);
  m_pPipelineBarrier->Flush();
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

  m_pPipelineBarrier->EnsureImageLayout(pVulkanSource, pVulkanSource->GetPreferredLayout(), pVulkanSource->GetUsedByPipelineStage(), pVulkanSource->GetAccessMask());
  m_pPipelineBarrier->EnsureImageLayout(pVulkanDestination, pVulkanDestination->GetPreferredLayout(), pVulkanDestination->GetUsedByPipelineStage(), pVulkanDestination->GetAccessMask());
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
  const ezUInt32 uiBufferSize = pSource->ComputeSubResourceOffsets(&m_GALDeviceVulkan, pSource->GetDescription(), subResourceOffsets);

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

  m_pPipelineBarrier->EnsureImageLayout(pSource, vk::ImageLayout::eTransferSrcOptimal, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferRead);
  m_pPipelineBarrier->Flush();

  m_pCommandBuffer->copyImageToBuffer(pSource->GetImage(), vk::ImageLayout::eTransferSrcOptimal, destination, imageCopy.GetCount(), imageCopy.GetData());

  m_pPipelineBarrier->EnsureImageLayout(pSource, pSource->GetPreferredLayout(), pSource->GetUsedByPipelineStage(), pSource->GetAccessMask());
  m_pPipelineBarrier->AddBufferBarrierInternal(destination, 0, uiBufferSize, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eHost, vk::AccessFlagBits::eHostRead);
  m_pPipelineBarrier->Flush();
}

void ezGALCommandEncoderImplVulkan::ReadbackTexturePlatform(const ezGALReadbackTexture* pDestination, const ezGALTexture* pSource)
{
  EZ_ASSERT_DEV(!m_bRenderPassActive, "Can't readback within a render pass");

  const ezGALTextureVulkan* pVulkanSourceTexture = static_cast<const ezGALTextureVulkan*>(pSource->GetParentResource());
  const ezGALReadbackTextureVulkan* pVulkanDestinationTexture = static_cast<const ezGALReadbackTextureVulkan*>(pDestination->GetParentResource());

  const ezGALTextureCreationDescription& textureDesc = pVulkanSourceTexture->GetDescription();
  const vk::ImageAspectFlagBits imageAspect = ezGALResourceFormat::IsDepthFormat(textureDesc.m_Format) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  const bool bMSAASourceTexture = textureDesc.m_SampleCount != ezGALMSAASampleCount::None;
  EZ_ASSERT_DEV(!bMSAASourceTexture, "MSAA read-back not implemented!");

  CopyImageToBuffer(pVulkanSourceTexture, pVulkanDestinationTexture->GetVkBuffer());

  // There is no need to change the layout back of this texture right now but as the next layout will most certainly not be another eTransferSrcOptimal we might as well change it back to its default state.
  m_pPipelineBarrier->EnsureImageLayout(pVulkanSourceTexture, pVulkanSourceTexture->GetPreferredLayout(), pVulkanSourceTexture->GetUsedByPipelineStage(), pVulkanSourceTexture->GetAccessMask());
}


void ezGALCommandEncoderImplVulkan::ReadbackBufferPlatform(const ezGALReadbackBuffer* pDestination, const ezGALBuffer* pSource)
{
  EZ_ASSERT_DEV(!m_bRenderPassActive, "Can't readback within a render pass");

  auto pDestinationVulkan = static_cast<const ezGALReadbackBufferVulkan*>(pDestination);
  auto pSourceVulkan = static_cast<const ezGALBufferVulkan*>(pSource);

  EZ_ASSERT_DEV(pSource->GetSize() == pDestination->GetSize(), "Source and destination buffer sizes mismatch!");

  vk::BufferCopy bufferCopy = {};
  bufferCopy.size = pSource->GetSize();

  m_pPipelineBarrier->AccessBuffer(pSourceVulkan, 0, bufferCopy.size, pSourceVulkan->GetUsedByPipelineStage(), pSourceVulkan->GetAccessMask(), vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferRead);
  m_pPipelineBarrier->Flush();

  m_pCommandBuffer->copyBuffer(pSourceVulkan->GetVkBuffer(), pDestinationVulkan->GetVkBuffer(), 1, &bufferCopy);

  m_pPipelineBarrier->AccessBuffer(pSourceVulkan, 0, bufferCopy.size, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferRead, pSourceVulkan->GetUsedByPipelineStage(), pSourceVulkan->GetAccessMask());
  m_pPipelineBarrier->AddBufferBarrierInternal(pDestinationVulkan->GetVkBuffer(), 0, pSource->GetSize(), vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eHost, vk::AccessFlagBits::eHostRead);
  m_pPipelineBarrier->Flush();
}

ezUInt32 GetMipSize(ezUInt32 uiSize, ezUInt32 uiMipLevel)
{
  for (ezUInt32 i = 0; i < uiMipLevel; i++)
  {
    uiSize = uiSize / 2;
  }
  return ezMath::Max(1u, uiSize);
}

void ezGALCommandEncoderImplVulkan::GenerateMipMapsPlatform(const ezGALTexture* pTexture, ezGALTextureRange range)
{
  const ezGALTextureVulkan* pVulkanTexture = static_cast<const ezGALTextureVulkan*>(pTexture);
  if (m_bRenderPassActive)
  {
    m_pCommandBuffer->endRenderPass();
    m_bRenderPassActive = false;
  }

  const vk::ImageSubresourceRange viewRange = ezConversionUtilsVulkan::GetSubresourceRange(pTexture->GetDescription().m_Format, range);
  if (viewRange.levelCount == 1)
    return;

  const vk::FormatProperties formatProps = m_GALDeviceVulkan.GetVulkanPhysicalDevice().getFormatProperties(pVulkanTexture->GetImageFormat());
  const bool bSupportsBlit = ((formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitSrc) && (formatProps.linearTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst));
  // MSAA textures (e.g. backbuffers) need to be converted to non MSAA versions
  const ezGALTextureCreationDescription& textureDesc = pVulkanTexture->GetDescription();
  const bool bMSAASourceTexture = textureDesc.m_SampleCount != ezGALMSAASampleCount::None;
  if (bMSAASourceTexture)
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }
  else
  {
    if (bSupportsBlit)
    {
      {
        vk::ImageSubresourceRange otherLevels = viewRange;
        otherLevels.baseMipLevel += 1;
        otherLevels.levelCount -= 1;
        m_pPipelineBarrier->EnsureImageLayout(pVulkanTexture, otherLevels, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite);
      }

      for (ezUInt32 uiMipLevel = viewRange.baseMipLevel; uiMipLevel < (viewRange.baseMipLevel + viewRange.levelCount - 1); uiMipLevel++)
      {
        {
          vk::ImageSubresourceRange currentLevel = viewRange;
          currentLevel.baseMipLevel = uiMipLevel;
          currentLevel.levelCount = 1;
          m_pPipelineBarrier->EnsureImageLayout(pVulkanTexture, currentLevel, vk::ImageLayout::eTransferSrcOptimal, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferRead);
          m_pPipelineBarrier->Flush();
        }
        vk::Extent3D sourceMipLevelSize = pVulkanTexture->GetMipLevelSize(uiMipLevel);
        vk::Offset3D sourceMipLevelEndOffset = {(ezInt32)sourceMipLevelSize.width, (ezInt32)sourceMipLevelSize.height, (ezInt32)sourceMipLevelSize.depth};
        vk::Extent3D destinationMipLevelSize = pVulkanTexture->GetMipLevelSize(uiMipLevel + 1);
        vk::Offset3D destinationMipLevelEndOffset = {(ezInt32)destinationMipLevelSize.width, (ezInt32)destinationMipLevelSize.height, (ezInt32)destinationMipLevelSize.depth};

        vk::ImageSubresourceLayers sourceLayers;
        sourceLayers.aspectMask = viewRange.aspectMask;
        sourceLayers.mipLevel = uiMipLevel;
        sourceLayers.baseArrayLayer = viewRange.baseArrayLayer;
        sourceLayers.layerCount = viewRange.layerCount;

        vk::ImageSubresourceLayers destinationLayers = sourceLayers;
        destinationLayers.mipLevel++;

        vk::ImageBlit imageBlitRegion;
        imageBlitRegion.srcSubresource = sourceLayers;
        imageBlitRegion.srcOffsets[1] = sourceMipLevelEndOffset;
        imageBlitRegion.dstSubresource = destinationLayers;
        imageBlitRegion.dstOffsets[1] = destinationMipLevelEndOffset;

        m_pCommandBuffer->blitImage(pVulkanTexture->GetImage(), vk::ImageLayout::eTransferSrcOptimal, pVulkanTexture->GetImage(), vk::ImageLayout::eTransferDstOptimal, 1, &imageBlitRegion, vk::Filter::eLinear);
      }
      // There is no need to change the layout back of this texture right now but as the next layout will most certainly not be another eTransferSrcOptimal we might as well change it back to its default state.
      m_pPipelineBarrier->EnsureImageLayout(pVulkanTexture, range, pVulkanTexture->GetPreferredLayout(), pVulkanTexture->GetUsedByPipelineStage(), pVulkanTexture->GetAccessMask());
    }
    else
    {
      {
        vk::ImageSubresourceRange otherLevels = viewRange;
        otherLevels.baseMipLevel += 1;
        otherLevels.levelCount -= 1;
        m_pPipelineBarrier->EnsureImageLayout(pVulkanTexture, otherLevels, vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead);
      }

      ezImageCopyVulkan copy(m_GALDeviceVulkan);
      const bool bStereoSupport = m_GALDeviceVulkan.GetCapabilities().m_bSupportsVSRenderTargetArrayIndex || m_GALDeviceVulkan.GetCapabilities().m_bShaderStageSupported[ezGALShaderStage::GeometryShader];
      if (bStereoSupport)
      {
        copy.Init(pVulkanTexture, pVulkanTexture, ezShaderUtils::ezBuiltinShaderType::DownscaleImageArray);
        for (ezUInt32 uiMipLevel = viewRange.baseMipLevel; uiMipLevel < (viewRange.baseMipLevel + viewRange.levelCount - 1); uiMipLevel++)
        {
          vk::Extent3D sourceMipLevelSize = pVulkanTexture->GetMipLevelSize(uiMipLevel);
          vk::Offset3D sourceMipLevelEndOffset = {(ezInt32)sourceMipLevelSize.width, (ezInt32)sourceMipLevelSize.height, (ezInt32)sourceMipLevelSize.depth};
          vk::Extent3D destinationMipLevelSize = pVulkanTexture->GetMipLevelSize(uiMipLevel + 1);
          vk::Offset3D destinationMipLevelEndOffset = {(ezInt32)destinationMipLevelSize.width, (ezInt32)destinationMipLevelSize.height, (ezInt32)destinationMipLevelSize.depth};

          vk::ImageSubresourceLayers sourceLayers;
          sourceLayers.aspectMask = viewRange.aspectMask;
          sourceLayers.mipLevel = uiMipLevel;
          sourceLayers.baseArrayLayer = viewRange.baseArrayLayer;
          sourceLayers.layerCount = viewRange.layerCount;

          vk::ImageSubresourceLayers destinationLayers = sourceLayers;
          destinationLayers.mipLevel++;

          vk::Extent3D mipLevelSize = pVulkanTexture->GetMipLevelSize(0);
          copy.Copy({0, 0, 0}, sourceLayers, {0, 0, 0}, destinationLayers, {(ezUInt32)destinationMipLevelSize.width, (ezUInt32)destinationMipLevelSize.height, (ezUInt32)destinationMipLevelSize.depth});
        }
      }
      else
      {
        copy.Init(pVulkanTexture, pVulkanTexture, ezShaderUtils::ezBuiltinShaderType::DownscaleImage);
        const ezUInt32 arraySize = (textureDesc.m_Type == ezGALTextureType::TextureCube || textureDesc.m_Type == ezGALTextureType::TextureCubeArray) ? textureDesc.m_uiArraySize * 6 : textureDesc.m_uiArraySize;
        const ezUInt32 mipLevels = textureDesc.m_uiMipLevelCount;

        for (ezUInt32 uiLayer = viewRange.baseArrayLayer; uiLayer < (viewRange.baseArrayLayer + viewRange.layerCount); uiLayer++)
        {
          for (ezUInt32 uiMipLevel = viewRange.baseMipLevel; uiMipLevel < (viewRange.baseMipLevel + viewRange.levelCount - 1); uiMipLevel++)
          {
            vk::Extent3D sourceMipLevelSize = pVulkanTexture->GetMipLevelSize(uiMipLevel);
            vk::Offset3D sourceMipLevelEndOffset = {(ezInt32)sourceMipLevelSize.width, (ezInt32)sourceMipLevelSize.height, (ezInt32)sourceMipLevelSize.depth};
            vk::Extent3D destinationMipLevelSize = pVulkanTexture->GetMipLevelSize(uiMipLevel + 1);
            vk::Offset3D destinationMipLevelEndOffset = {(ezInt32)destinationMipLevelSize.width, (ezInt32)destinationMipLevelSize.height, (ezInt32)destinationMipLevelSize.depth};

            vk::ImageSubresourceLayers sourceLayers;
            sourceLayers.aspectMask = viewRange.aspectMask;
            sourceLayers.mipLevel = uiMipLevel;
            sourceLayers.baseArrayLayer = viewRange.baseArrayLayer;
            sourceLayers.layerCount = 1;

            vk::ImageSubresourceLayers destinationLayers = sourceLayers;
            destinationLayers.mipLevel++;

            vk::Extent3D mipLevelSize = pVulkanTexture->GetMipLevelSize(0);
            copy.Copy({0, 0, 0}, sourceLayers, {0, 0, 0}, destinationLayers, {(ezUInt32)destinationMipLevelSize.width, (ezUInt32)destinationMipLevelSize.height, (ezUInt32)destinationMipLevelSize.depth});
          }
        }
      }

      m_pPipelineBarrier->EnsureImageLayout(pVulkanTexture, range, pVulkanTexture->GetPreferredLayout(), pVulkanTexture->GetUsedByPipelineStage(), pVulkanTexture->GetAccessMask());

      m_bPipelineStateDirty = true;
      m_bViewportDirty = true;
      m_bDescriptorsDirty = true;
    }
  }
}

void ezGALCommandEncoderImplVulkan::FlushPlatform()
{
  m_GALDeviceVulkan.Submit();
  SetCurrentCommandBuffer(&m_GALDeviceVulkan.GetCurrentCommandBuffer(), &m_GALDeviceVulkan.GetCurrentPipelineBarrier());
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

    m_pCommandBuffer->beginDebugUtilsLabelEXT(markerInfo);
  }
}

void ezGALCommandEncoderImplVulkan::PopMarkerPlatform()
{
  if (m_GALDeviceVulkan.GetExtensions().m_bDebugUtilsMarkers)
  {
    m_pCommandBuffer->endDebugUtilsLabelEXT();
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
    m_pCommandBuffer->insertDebugUtilsLabelEXT(markerInfo);
  }
}

//////////////////////////////////////////////////////////////////////////

void ezGALCommandEncoderImplVulkan::BeginRenderingPlatform(const ezGALRenderingSetup& renderingSetup)
{
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

    const bool m_bHasDepth = renderingSetup.HasDepthStencilTarget();
    const ezUInt32 uiColorCount = renderingSetup.GetColorTargetCount();
    m_bClearSubmitted = !UsesClearOp(renderingSetup);

    if (m_bHasDepth)
    {
      vk::ClearValue& depthClear = m_clearValues.ExpandAndGetRef();
      depthClear.depthStencil.setDepth(renderingSetup.GetClearDepth()).setStencil(renderingSetup.GetClearStencil());

      const ezGALRenderTargetViewVulkan* pRenderTargetView = static_cast<const ezGALRenderTargetViewVulkan*>(m_GALDeviceVulkan.GetRenderTargetView(renderingSetup.GetFrameBuffer().m_hDepthTarget));
      m_depthMask = pRenderTargetView->GetRange().aspectMask;
      m_pPipelineBarrier->EnsureImageLayout(pRenderTargetView, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests, vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead);
    }
    for (ezUInt32 i = 0; i < uiColorCount; i++)
    {
      vk::ClearValue& colorClear = m_clearValues.ExpandAndGetRef();
      ezColor col = renderingSetup.GetClearColor(i);
      colorClear.color.setFloat32({col.r, col.g, col.b, col.a});

      const ezGALRenderTargetViewVulkan* pRenderTargetView = static_cast<const ezGALRenderTargetViewVulkan*>(m_GALDeviceVulkan.GetRenderTargetView(renderingSetup.GetFrameBuffer().m_hColorTarget[i]));
      m_pPipelineBarrier->EnsureImageLayout(pRenderTargetView, vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead);
    }

    m_renderPass.clearValueCount = m_clearValues.GetCount();
    m_renderPass.pClearValues = m_clearValues.GetData();
  }

  m_bPipelineStateDirty = true;
  m_bViewportDirty = true;
}

void ezGALCommandEncoderImplVulkan::EndRenderingPlatform()
{
  if (!m_bClearSubmitted)
  {
    m_pPipelineBarrier->Flush();
    // If we end rendering without having flushed the clear, just begin and immediately end rendering.
    m_pCommandBuffer->beginRenderPass(m_renderPass, vk::SubpassContents::eInline);
    m_bClearSubmitted = true;
    m_bRenderPassActive = true;
  }

  if (m_bRenderPassActive)
  {
    m_pCommandBuffer->endRenderPass();
    m_bRenderPassActive = false;
  }

  m_depthMask = {};
  m_uiLayers = 0;

  m_renderPass.renderPass = nullptr;
  m_renderPass.framebuffer = nullptr;
}

void ezGALCommandEncoderImplVulkan::ClearPlatform(const ezColor& ClearColor, ezUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, ezUInt8 uiStencilClear)
{
  if (!m_bRenderPassActive && !m_bInsideCompute)
  {
    if (m_pPipelineBarrier->IsDirty())
    {
      m_pPipelineBarrier->Flush();
    }

    m_pCommandBuffer->beginRenderPass(m_renderPass, vk::SubpassContents::eInline);
    m_bClearSubmitted = true;
    m_bRenderPassActive = true;
  }
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
    m_pGraphicsPipeline = static_cast<const ezGALGraphicsPipelineVulkan*>(pGraphicsPipeline);
    if (m_pGraphicsPipeline)
    {
      m_pShader = static_cast<const ezGALShaderVulkan*>(m_GALDeviceVulkan.GetShader(m_pGraphicsPipeline->GetDescription().m_hShader));
      m_bScissorEnabled = m_GALDeviceVulkan.GetRasterizerState(m_pGraphicsPipeline->GetDescription().m_hRasterizerState)->GetDescription().m_bScissorTest;
    }
    else
    {
      m_bScissorEnabled = false;
    }
    m_bPipelineStateDirty = true;
  }
}

void ezGALCommandEncoderImplVulkan::SetComputePipelinePlatform(const ezGALComputePipeline* pComputePipeline)
{
  if (m_pComputePipeline != pComputePipeline)
  {
    m_pComputePipeline = static_cast<const ezGALComputePipelineVulkan*>(pComputePipeline);
    if (m_pComputePipeline)
    {
      m_pShader = static_cast<const ezGALShaderVulkan*>(m_GALDeviceVulkan.GetShader(m_pComputePipeline->GetDescription().m_hShader));
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
  }
}

void ezGALCommandEncoderImplVulkan::SetScissorRectPlatform(const ezRectU32& rect)
{
  vk::Rect2D scissor(vk::Offset2D(rect.x, rect.y), vk::Extent2D(rect.width, rect.height));
  if (m_scissor != scissor)
  {
    // viewport is marked as dynamic in the pipeline layout and thus does not mark m_bPipelineStateDirty.
    m_scissor = scissor;
    m_bViewportDirty = true;
  }
}

void ezGALCommandEncoderImplVulkan::SetStencilReferencePlatform(ezUInt8 uiStencilRefValue)
{
  m_pCommandBuffer->setStencilReference(vk::StencilFaceFlagBits::eFrontAndBack, uiStencilRefValue);
}

//////////////////////////////////////////////////////////////////////////

void ezGALCommandEncoderImplVulkan::BeginComputePlatform()
{
  m_GALDeviceVulkan.GetCurrentCommandBuffer();

  m_bClearSubmitted = true;
  m_bInsideCompute = true;
  m_bPipelineStateDirty = true;
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

#define EZ_VULKAN_CHECK_STATE(bCondition, szErrorMsg, ...)                                  \
  do                                                                                        \
  {                                                                                         \
    /*EZ_ASSERT_DEBUG(bCondition, szErrorMsg, ##__VA_ARGS__); */                            \
    EZ_MSVC_ANALYSIS_WARNING_PUSH                                                           \
    EZ_MSVC_ANALYSIS_WARNING_DISABLE(6326) /* disable static analysis for the comparison */ \
    if (!!(bCondition) == false)                                                            \
    {                                                                                       \
      ezLog::Error(szErrorMsg, ##__VA_ARGS__);                                              \
      return EZ_FAILURE;                                                                    \
    }                                                                                       \
    EZ_MSVC_ANALYSIS_WARNING_POP                                                            \
  } while (false)

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
    // Changes to the descriptor layout always require the descriptor set to be re-created.
    m_bDescriptorsDirty = true;
  }

  if (!m_bInsideCompute && m_bViewportDirty)
  {
    m_pCommandBuffer->setViewport(0, 1, &m_viewport);
    if (m_bScissorEnabled)
      m_pCommandBuffer->setScissor(0, 1, &m_scissor);
    else
    {
      vk::Rect2D noScissor({int(m_viewport.x), int(m_viewport.y + m_viewport.height)}, {ezUInt32(m_viewport.width), ezUInt32(-m_viewport.height)});
      m_pCommandBuffer->setScissor(0, 1, &noScissor);
    }
    m_bViewportDirty = false;
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

  if (true /*m_bDescriptorsDirty*/)
  {
    // #TODO_VULKAN we always create a new descriptor set
    m_bDescriptorsDirty = false;


    const ezUInt32 uiBindGroups = m_pShader->GetBindGroupCount();
    m_DescriptorSets.SetCount(uiBindGroups);
    m_DynamicUniformBufferOffsets.Clear();

    for (ezUInt32 uiBindGroup = 0; uiBindGroup < uiBindGroups; ++uiBindGroup)
    {
      if (m_pShader->GetBindGroupLayout(uiBindGroup) != m_BindGroups[uiBindGroup].m_Desc.m_hBindGroupLayout)
      {
        ezLog::Error("Bind group layout missmatch");
        return EZ_FAILURE;
      }
      m_DescriptorSets[uiBindGroup] = ezDescriptorSetPoolVulkan::CreateDescriptorSet(m_pShader->GetDescriptorSetLayout(uiBindGroup));

      EZ_SUCCEED_OR_RETURN(CreateDescriptorSet(uiBindGroup));

      m_DynamicUniformBufferOffsets.PushBackRange(m_BindGroups[uiBindGroup].m_DynamicUniformBufferOffsets);
    }
    m_pCommandBuffer->bindDescriptorSets(m_bInsideCompute ? vk::PipelineBindPoint::eCompute : vk::PipelineBindPoint::eGraphics, m_pShader->GetVkPipelineLayout(), 0, m_DescriptorSets.GetCount(), m_DescriptorSets.GetData(), m_DynamicUniformBufferOffsets.GetCount(), m_DynamicUniformBufferOffsets.GetData());
  }

  if (m_bPushConstantsDirty && m_pShader->GetPushConstantRange().size > 0)
  {
    EZ_ASSERT_DEBUG(m_pShader->GetPushConstantRange().size == m_PushConstants.GetCount(), "");

    m_pCommandBuffer->pushConstants(m_pShader->GetVkPipelineLayout(), m_pShader->GetPushConstantRange().stageFlags, m_pShader->GetPushConstantRange().offset, m_PushConstants.GetCount(), m_PushConstants.GetData());
  }

  if (m_bRenderPassActive && m_pPipelineBarrier->IsDirty())
  {
    m_pCommandBuffer->endRenderPass();
    m_bRenderPassActive = false;

    m_pPipelineBarrier->FullBarrier();
  }

  m_pPipelineBarrier->Flush();

  if (!m_bRenderPassActive && !m_bInsideCompute)
  {
    m_pCommandBuffer->beginRenderPass(m_renderPass, vk::SubpassContents::eInline);
    m_bClearSubmitted = true;
    m_bRenderPassActive = true;
  }
  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoderImplVulkan::CreateDescriptorSet(ezUInt32 uiBindGroup)
{
  m_DescriptorWrites.Clear();
  m_DescriptorImageInfos.Clear();
  m_DescriptorBufferInfos.Clear();
  m_DescriptorBufferViews.Clear();
  m_BindGroups[uiBindGroup].m_DynamicUniformBuffers.Clear();
  m_BindGroups[uiBindGroup].m_DynamicUniformBufferOffsets.Clear();

  const ezGALBindGroupLayoutVulkan* pLayout = static_cast<const ezGALBindGroupLayoutVulkan*>(m_GALDeviceVulkan.GetBindGroupLayout(m_BindGroups[uiBindGroup].m_Desc.m_hBindGroupLayout));
  if (pLayout == nullptr)
    return EZ_FAILURE;

  m_DescriptorSets[uiBindGroup] = ezDescriptorSetPoolVulkan::CreateDescriptorSet(pLayout->GetDescriptorSetLayout());
  ezArrayPtr<const ezShaderResourceBinding> bindings = pLayout->GetDescription().m_ResourceBindings;
  const ezUInt32 uiBindings = bindings.GetCount();
  for (ezUInt32 i = 0; i < uiBindings; ++i)
  {
    const ezShaderResourceBinding& binding = bindings[i];
    const ezGALBindGroupItem& item = m_BindGroups[uiBindGroup].m_Desc.m_BindGroupItems[i];

    vk::WriteDescriptorSet& write = m_DescriptorWrites.ExpandAndGetRef();
    write.dstArrayElement = 0;
    write.descriptorType = ezConversionUtilsVulkan::GetDescriptorType(binding.m_ResourceType);
    write.dstBinding = binding.m_iSlot;
    write.dstSet = m_DescriptorSets[uiBindGroup];
    write.descriptorCount = binding.m_uiArraySize;
    switch (binding.m_ResourceType)
    {
      case ezGALShaderResourceType::ConstantBuffer:
      {
        vk::DescriptorBufferInfo& bufferInfo = m_DescriptorBufferInfos.ExpandAndGetRef();
        write.pBufferInfo = &bufferInfo;
        const ezGALBufferVulkan* pBuffer = static_cast<const ezGALBufferVulkan*>(m_GALDeviceVulkan.GetBuffer(item.m_Buffer.m_hBuffer));
        if (pBuffer->GetDescription().m_BufferFlags.IsSet(ezGALBufferUsageFlags::Transient))
        {
          bufferInfo = *m_pUniformBufferPool->GetBuffer(pBuffer);
        }
        else
        {
          bufferInfo = pBuffer->GetBufferInfo();
        }

        // Move offset out and into the separate offset array.
        m_BindGroups[uiBindGroup].m_DynamicUniformBuffers.PushBack(pBuffer);
        m_BindGroups[uiBindGroup].m_DynamicUniformBufferOffsets.PushBack((ezUInt32)bufferInfo.offset);
        bufferInfo.offset = 0;
      }
      break;
      case ezGALShaderResourceType::Texture:
      case ezGALShaderResourceType::TextureRW:
      case ezGALShaderResourceType::TextureAndSampler:
      {
        vk::DescriptorImageInfo& imageInfo = m_DescriptorImageInfos.ExpandAndGetRef();
        write.pImageInfo = &imageInfo;
        const ezGALTextureVulkan* pTexture = static_cast<const ezGALTextureVulkan*>(m_GALDeviceVulkan.GetTexture(item.m_Texture.m_hTexture));
        imageInfo = pTexture->GetDescriptorImageInfo(item.m_Texture.m_TextureRange, binding.m_ResourceType, binding.m_TextureType, item.m_Texture.m_OverrideViewFormat);
        imageInfo.imageLayout = binding.m_ResourceType == ezGALShaderResourceType::TextureRW ? vk::ImageLayout::eGeneral : pTexture->GetPreferredLayout(ezConversionUtilsVulkan::GetDefaultLayout(pTexture->GetImageFormat()));
        m_pPipelineBarrier->EnsureImageLayout(pTexture, item.m_Texture.m_TextureRange, imageInfo.imageLayout, ezConversionUtilsVulkan::GetPipelineStages(binding.m_Stages), vk::AccessFlagBits::eShaderRead);

        if (binding.m_ResourceType == ezGALShaderResourceType::TextureAndSampler)
        {
          const ezGALSamplerStateVulkan* pSampler = static_cast<const ezGALSamplerStateVulkan*>(m_GALDeviceVulkan.GetSamplerState(item.m_Texture.m_hSampler));
          imageInfo.sampler = pSampler->GetImageInfo().sampler;
        }
      }
      break;
      case ezGALShaderResourceType::TexelBuffer:
      case ezGALShaderResourceType::TexelBufferRW:
      {
        vk::BufferView& bufferView = m_DescriptorBufferViews.ExpandAndGetRef();
        write.pTexelBufferView = &bufferView;
        const ezGALBufferVulkan* pBuffer = static_cast<const ezGALBufferVulkan*>(m_GALDeviceVulkan.GetBuffer(item.m_Buffer.m_hBuffer));
        bufferView = pBuffer->GetTexelBufferView(item.m_Buffer.m_BufferRange, item.m_Buffer.m_OverrideTexelBufferFormat);
      }
      break;
      case ezGALShaderResourceType::StructuredBuffer:
      case ezGALShaderResourceType::ByteAddressBuffer:
      case ezGALShaderResourceType::StructuredBufferRW:
      case ezGALShaderResourceType::ByteAddressBufferRW:
      {
        vk::DescriptorBufferInfo& bufferInfo = m_DescriptorBufferInfos.ExpandAndGetRef();
        write.pBufferInfo = &bufferInfo;
        const ezGALBufferVulkan* pBuffer = static_cast<const ezGALBufferVulkan*>(m_GALDeviceVulkan.GetBuffer(item.m_Buffer.m_hBuffer));
        bufferInfo.buffer = pBuffer->GetBufferInfo().buffer;
        bufferInfo.offset = item.m_Buffer.m_BufferRange.m_uiByteOffset;
        bufferInfo.range = item.m_Buffer.m_BufferRange.m_uiByteCount;
      }
      break;
      case ezGALShaderResourceType::Sampler:
      {
        vk::DescriptorImageInfo& imageInfo = m_DescriptorImageInfos.ExpandAndGetRef();
        write.pImageInfo = &imageInfo;
        const ezGALSamplerStateVulkan* pSampler = static_cast<const ezGALSamplerStateVulkan*>(m_GALDeviceVulkan.GetSamplerState(item.m_Sampler.m_hSampler));
        imageInfo.sampler = pSampler->GetImageInfo().sampler;
      }
      break;
      default:
        break;
    }
  }

  ezDescriptorSetPoolVulkan::UpdateDescriptorSet(m_DescriptorSets[uiBindGroup], m_DescriptorWrites);
  return EZ_SUCCESS;
}
