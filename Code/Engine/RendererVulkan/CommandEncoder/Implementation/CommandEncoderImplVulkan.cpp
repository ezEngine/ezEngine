#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/CommandEncoder/CommandEncoderImplVulkan.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Pools/DescriptorSetPoolVulkan.h>
#include <RendererVulkan/Pools/QueryPoolVulkan.h>
#include <RendererVulkan/Pools/StagingBufferPoolVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/FallbackResourcesVulkan.h>
#include <RendererVulkan/Resources/QueryVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Resources/ResourceViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Resources/UnorderedAccessViewVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>
#include <RendererVulkan/Utils/ImageCopyVulkan.h>
#include <RendererVulkan/Utils/PipelineBarrierVulkan.h>

ezGALCommandEncoderImplVulkan::ezGALCommandEncoderImplVulkan(ezGALDeviceVulkan& device)
  : m_GALDeviceVulkan(device)
{
  m_vkDevice = device.GetVulkanDevice();
}

ezGALCommandEncoderImplVulkan::~ezGALCommandEncoderImplVulkan() = default;

void ezGALCommandEncoderImplVulkan::Reset()
{
  EZ_ASSERT_DEBUG(!m_bRenderPassActive, "Render pass was not closed");

  m_bPipelineStateDirty = true;
  m_bViewportDirty = true;
  m_bIndexBufferDirty = true;
  m_bDescriptorsDirty = true;
  m_BoundVertexBuffersRange.Reset();

  m_LayoutDesc = {};
  m_PipelineDesc = {};
  m_frameBuffer = nullptr;

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

  for (ezUInt32 i = 0; i < EZ_GAL_MAX_CONSTANT_BUFFER_COUNT; i++)
  {
    m_pBoundConstantBuffers[i] = nullptr;
  }
  for (ezUInt32 i = 0; i < ezGALShaderStage::ENUM_COUNT; i++)
  {
    m_pBoundShaderResourceViews[i].Clear();
  }
  m_pBoundUnoderedAccessViews.Clear();

  ezMemoryUtils::ZeroFill(&m_pBoundSamplerStates[0][0], ezGALShaderStage::ENUM_COUNT * EZ_GAL_MAX_SAMPLER_COUNT);

  m_renderPass = vk::RenderPassBeginInfo();
  m_clearValues.Clear();
}

void ezGALCommandEncoderImplVulkan::MarkDirty()
{
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
}

void ezGALCommandEncoderImplVulkan::SetCurrentCommandBuffer(vk::CommandBuffer* commandBuffer, ezPipelineBarrierVulkan* pipelineBarrier)
{
  m_pCommandBuffer = commandBuffer;
  m_pPipelineBarrier = pipelineBarrier;
}

// State setting functions

void ezGALCommandEncoderImplVulkan::SetShaderPlatform(const ezGALShader* pShader)
{
  if (pShader != nullptr)
  {
    m_PipelineDesc.m_pCurrentShader = static_cast<const ezGALShaderVulkan*>(pShader);
    m_ComputeDesc.m_pCurrentShader = m_PipelineDesc.m_pCurrentShader;
    m_bPipelineStateDirty = true;
  }
}

void ezGALCommandEncoderImplVulkan::SetConstantBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pBuffer)
{
  // \todo Check if the device supports the slot index?
  m_pBoundConstantBuffers[uiSlot] = pBuffer != nullptr ? static_cast<const ezGALBufferVulkan*>(pBuffer) : nullptr;
  m_bDescriptorsDirty = true;
}

void ezGALCommandEncoderImplVulkan::SetSamplerStatePlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALSamplerState* pSamplerState)
{
  // \todo Check if the device supports the stage / the slot index
  m_pBoundSamplerStates[Stage][uiSlot] = pSamplerState != nullptr ? static_cast<const ezGALSamplerStateVulkan*>(pSamplerState) : nullptr;
  m_bDescriptorsDirty = true;
}

void ezGALCommandEncoderImplVulkan::SetResourceViewPlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALResourceView* pResourceView)
{
  auto& boundShaderResourceViews = m_pBoundShaderResourceViews[Stage];
  boundShaderResourceViews.EnsureCount(uiSlot + 1);
  boundShaderResourceViews[uiSlot] = pResourceView != nullptr ? static_cast<const ezGALResourceViewVulkan*>(pResourceView) : nullptr;
  m_bDescriptorsDirty = true;
}

void ezGALCommandEncoderImplVulkan::SetUnorderedAccessViewPlatform(ezUInt32 uiSlot, const ezGALUnorderedAccessView* pUnorderedAccessView)
{
  m_pBoundUnoderedAccessViews.EnsureCount(uiSlot + 1);
  m_pBoundUnoderedAccessViews[uiSlot] = pUnorderedAccessView != nullptr ? static_cast<const ezGALUnorderedAccessViewVulkan*>(pUnorderedAccessView) : nullptr;
  m_bDescriptorsDirty = true;
}

// Query functions

void ezGALCommandEncoderImplVulkan::BeginQueryPlatform(const ezGALQuery* pQuery)
{
  auto pVulkanQuery = static_cast<const ezGALQueryVulkan*>(pQuery);

  // TODO how to decide the query type etc in Vulkan?

  m_pCommandBuffer->beginQuery(pVulkanQuery->GetPool(), pVulkanQuery->GetID(), {});
}

void ezGALCommandEncoderImplVulkan::EndQueryPlatform(const ezGALQuery* pQuery)
{
  auto pVulkanQuery = static_cast<const ezGALQueryVulkan*>(pQuery);

  m_pCommandBuffer->endQuery(pVulkanQuery->GetPool(), pVulkanQuery->GetID());
}

ezResult ezGALCommandEncoderImplVulkan::GetQueryResultPlatform(const ezGALQuery* pQuery, ezUInt64& uiQueryResult)
{
  auto pVulkanQuery = static_cast<const ezGALQueryVulkan*>(pQuery);
  vk::Result result = m_vkDevice.getQueryPoolResults(pVulkanQuery->GetPool(), pVulkanQuery->GetID(), 1u, sizeof(ezUInt64), &uiQueryResult, 0, vk::QueryResultFlagBits::e64);

  return result == vk::Result::eSuccess ? EZ_SUCCESS : EZ_FAILURE;
}

void ezGALCommandEncoderImplVulkan::InsertTimestampPlatform(ezGALTimestampHandle hTimestamp)
{
  m_GALDeviceVulkan.GetQueryPool().InsertTimestamp(m_GALDeviceVulkan.GetCurrentCommandBuffer(), hTimestamp);
}

// Resource update functions

void ezGALCommandEncoderImplVulkan::ClearUnorderedAccessViewPlatform(const ezGALUnorderedAccessView* pUnorderedAccessView, ezVec4 clearValues)
{
  // this looks to require custom code, either using buffer copies or
  // clearing via a compute shader

  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALCommandEncoderImplVulkan::ClearUnorderedAccessViewPlatform(const ezGALUnorderedAccessView* pUnorderedAccessView, ezVec4U32 clearValues)
{
  // Same as the other clearing variant

  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALCommandEncoderImplVulkan::CopyBufferPlatform(const ezGALBuffer* pDestination, const ezGALBuffer* pSource)
{
  auto pDestinationVulkan = static_cast<const ezGALBufferVulkan*>(pDestination);
  auto pSourceVulkan = static_cast<const ezGALBufferVulkan*>(pSource);

  EZ_ASSERT_DEV(pSource->GetSize() != pDestination->GetSize(), "Source and destination buffer sizes mismatch!");

  // TODO do this in an immediate command buffer?
  vk::BufferCopy bufferCopy = {};
  bufferCopy.size = pSource->GetSize();

  //#TODO_VULKAN better barrier management of buffers.
  m_pPipelineBarrier->Flush();

  m_pCommandBuffer->copyBuffer(pSourceVulkan->GetVkBuffer(), pDestinationVulkan->GetVkBuffer(), 1, &bufferCopy);

  m_pPipelineBarrier->AccessBuffer(pSourceVulkan, 0, bufferCopy.size, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferRead, pDestinationVulkan->GetUsedByPipelineStage(), pDestinationVulkan->GetAccessMask());
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

  //#TODO_VULKAN better barrier management of buffers.
  m_pPipelineBarrier->Flush();

  m_pCommandBuffer->copyBuffer(pSourceVulkan->GetVkBuffer(), pDestinationVulkan->GetVkBuffer(), 1, &bufferCopy);

  m_pPipelineBarrier->AccessBuffer(pSourceVulkan, 0, bufferCopy.size, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferRead, pDestinationVulkan->GetUsedByPipelineStage(), pDestinationVulkan->GetAccessMask());
  m_pPipelineBarrier->AccessBuffer(pDestinationVulkan, 0, bufferCopy.size, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, pDestinationVulkan->GetUsedByPipelineStage(), pDestinationVulkan->GetAccessMask());
}

void ezGALCommandEncoderImplVulkan::UpdateBufferPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> pSourceData,
  ezGALUpdateMode::Enum updateMode)
{
  EZ_CHECK_ALIGNMENT_16(pSourceData.GetPtr());

  auto pVulkanDestination = static_cast<const ezGALBufferVulkan*>(pDestination);

  switch (updateMode)
  {
    case ezGALUpdateMode::Discard:
      pVulkanDestination->DiscardBuffer();
      [[fallthrough]];
    case ezGALUpdateMode::NoOverwrite:
    {
      ezVulkanAllocation alloc = pVulkanDestination->GetAllocation();
      void* pData = nullptr;
      VK_ASSERT_DEV(ezMemoryAllocatorVulkan::MapMemory(alloc, &pData));
      EZ_ASSERT_DEV(pData, "Implementation error");
      ezMemoryUtils::Copy(ezMemoryUtils::AddByteOffset((ezUInt8*)pData, uiDestOffset), pSourceData.GetPtr(), pSourceData.GetCount());
      ezMemoryAllocatorVulkan::UnmapMemory(alloc);
    }
    break;
    case ezGALUpdateMode::CopyToTempStorage:
    {
      if (m_bRenderPassActive)
      {
        m_pCommandBuffer->endRenderPass();
        m_bRenderPassActive = false;
      }

      EZ_ASSERT_DEBUG(!m_bRenderPassActive, "Vulkan does not support copying buffers while a render pass is active. TODO: Fix high level render code to make this impossible.");

      m_GALDeviceVulkan.UploadBufferStaging(&m_GALDeviceVulkan.GetStagingBufferPool(), m_pPipelineBarrier, *m_pCommandBuffer, pVulkanDestination, pSourceData, uiDestOffset);
    }
    break;
    default:
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


  // TODO need to copy every mip level
  ezHybridArray<vk::ImageCopy, 14> imageCopies;

  for (ezUInt32 i = 0; i < destDesc.m_uiMipLevelCount; ++i)
  {
    vk::ImageCopy& imageCopy = imageCopies.ExpandAndGetRef();
    imageCopy.dstOffset = vk::Offset3D();
    imageCopy.dstSubresource.aspectMask = imageAspect;
    imageCopy.dstSubresource.baseArrayLayer = 0;
    imageCopy.dstSubresource.layerCount = destDesc.m_uiArraySize;
    imageCopy.dstSubresource.mipLevel = i;
    imageCopy.extent.width = destDesc.m_uiWidth;
    imageCopy.extent.height = destDesc.m_uiHeight;
    imageCopy.extent.depth = destDesc.m_uiDepth;
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

  //aaa
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
  ezStagingBufferVulkan stagingBuffer = m_GALDeviceVulkan.GetStagingBufferPool().AllocateBuffer(0, uiTotalSize);

  const ezUInt32 uiBufferRowPitch = uiBlockSize * blockCount.width;
  const ezUInt32 uiBufferSlicePitch = uiBufferRowPitch * blockCount.height;
  EZ_ASSERT_DEV(uiBufferRowPitch == data.m_uiRowPitch, "Row pitch with padding is not implemented yet.");

  void* pData = nullptr;
  ezMemoryAllocatorVulkan::MapMemory(stagingBuffer.m_alloc, &pData);
  ezMemoryUtils::RawByteCopy(pData, data.m_pData, uiTotalSize);
  ezMemoryAllocatorVulkan::UnmapMemory(stagingBuffer.m_alloc);

  vk::BufferImageCopy region = {};
  region.imageSubresource.aspectMask = range.aspectMask;
  region.imageSubresource.mipLevel = range.baseMipLevel;
  region.imageSubresource.baseArrayLayer = range.baseArrayLayer;
  region.imageSubresource.layerCount = range.layerCount;

  region.imageOffset = vk::Offset3D(DestinationBox.m_vMin.x, DestinationBox.m_vMin.y, DestinationBox.m_vMin.z);
  region.imageExtent = vk::Extent3D(uiWidth, uiHeight, uiDepth);

  region.bufferOffset = 0;
  region.bufferRowLength = blockExtent[0] * uiBufferRowPitch / uiBlockSize;
  region.bufferImageHeight = blockExtent[1] * uiBufferSlicePitch / uiBufferRowPitch;

  m_pCommandBuffer->copyBufferToImage(stagingBuffer.m_buffer, pDestVulkan->GetImage(), pDestVulkan->GetPreferredLayout(vk::ImageLayout::eTransferDstOptimal), 1, &region);
  m_GALDeviceVulkan.GetStagingBufferPool().ReclaimBuffer(stagingBuffer);
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
  const ezGALTextureCreationDescription& textureDesc = pSource->GetDescription();
  const vk::ImageAspectFlags imageAspect = pSource->GetAspectMask();

  ezHybridArray<ezGALTextureVulkan::SubResourceOffset, 8> subResourceOffsets;
  const ezUInt32 uiBufferSize = pSource->ComputeSubResourceOffsets(subResourceOffsets);

  ezHybridArray<vk::BufferImageCopy, 8> imageCopy;
  const ezUInt32 arraySize = textureDesc.m_Type == ezGALTextureType::TextureCube ? textureDesc.m_uiArraySize * 6 : textureDesc.m_uiArraySize;
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

  m_pCommandBuffer->copyImageToBuffer(pSource->GetImage(), vk::ImageLayout::eTransferSrcOptimal, pDestination->GetVkBuffer(), imageCopy.GetCount(), imageCopy.GetData());

  m_pPipelineBarrier->EnsureImageLayout(pSource, pSource->GetPreferredLayout(), pSource->GetUsedByPipelineStage(), pSource->GetAccessMask());
  m_pPipelineBarrier->AccessBuffer(pDestination, 0, uiBufferSize, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eHost, vk::AccessFlagBits::eMemoryRead);
}

void ezGALCommandEncoderImplVulkan::ReadbackTexturePlatform(const ezGALTexture* pTexture)
{
  if (!m_bClearSubmitted)
  {
    m_pPipelineBarrier->Flush();

    // If we want to readback one of the render targets, we need to first flush the clear.
    // #TODO_VULKAN Check whether pTexture is one of the render targets or change the top-level api to prevent this.
    m_pCommandBuffer->beginRenderPass(m_renderPass, vk::SubpassContents::eInline);
    m_bClearSubmitted = true;
    m_bRenderPassActive = true;
  }

  if (m_bRenderPassActive)
  {
    m_pCommandBuffer->endRenderPass();
    m_bRenderPassActive = false;
  }

  EZ_ASSERT_DEV(!m_bRenderPassActive, "Can't readback within a render pass");

  const ezGALTextureVulkan* pVulkanTexture = static_cast<const ezGALTextureVulkan*>(pTexture->GetParentResource());
  const ezGALTextureCreationDescription& textureDesc = pVulkanTexture->GetDescription();
  const vk::ImageAspectFlagBits imageAspect = ezGALResourceFormat::IsDepthFormat(textureDesc.m_Format) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  const bool bMSAASourceTexture = textureDesc.m_SampleCount != ezGALMSAASampleCount::None;
  EZ_ASSERT_DEV(!bMSAASourceTexture, "MSAA read-back not implemented!");
  const ezGALTextureVulkan::StagingMode stagingMode = pVulkanTexture->GetStagingMode();
  EZ_ASSERT_DEV(stagingMode != ezGALTextureVulkan::StagingMode::None, "No staging resource available for read-back");

  if (stagingMode == ezGALTextureVulkan::StagingMode::Buffer)
  {
    const ezGALBufferVulkan* pStagingBuffer = static_cast<const ezGALBufferVulkan*>(m_GALDeviceVulkan.GetBuffer(pVulkanTexture->GetStagingBuffer()));
    CopyImageToBuffer(pVulkanTexture, pStagingBuffer);
  }
  else
  {
    // Render to texture
    const ezGALTextureVulkan* pStagingTexture = static_cast<const ezGALTextureVulkan*>(m_GALDeviceVulkan.GetTexture(pVulkanTexture->GetStagingTexture()));
    const bool bSourceIsDepth = ezConversionUtilsVulkan::IsDepthFormat(pVulkanTexture->GetImageFormat());

    m_pPipelineBarrier->EnsureImageLayout(pVulkanTexture, bSourceIsDepth ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits::eFragmentShader, vk::AccessFlagBits::eShaderRead);
    m_pPipelineBarrier->EnsureImageLayout(pStagingTexture, vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite);
    m_pPipelineBarrier->Flush();

    ezImageCopyVulkan copy(m_GALDeviceVulkan);

    const bool bStereoSupport = m_GALDeviceVulkan.GetCapabilities().m_bVertexShaderRenderTargetArrayIndex || m_GALDeviceVulkan.GetCapabilities().m_bShaderStageSupported[ezGALShaderStage::GeometryShader];
    if (bStereoSupport)
    {
      copy.Init(pVulkanTexture, pStagingTexture, ezShaderUtils::ezBuiltinShaderType::CopyImageArray);
      const ezUInt32 arraySize = textureDesc.m_Type == ezGALTextureType::TextureCube ? textureDesc.m_uiArraySize * 6 : textureDesc.m_uiArraySize;
      const ezUInt32 mipLevels = textureDesc.m_uiMipLevelCount;
      for (ezUInt32 uiMipLevel = 0; uiMipLevel < textureDesc.m_uiMipLevelCount; uiMipLevel++)
      {
        vk::ImageSubresourceLayers subresourceLayersSource;
        subresourceLayersSource.aspectMask = imageAspect;
        subresourceLayersSource.mipLevel = uiMipLevel;
        subresourceLayersSource.baseArrayLayer = 0;
        subresourceLayersSource.layerCount = arraySize;

        vk::ImageSubresourceLayers subresourceLayersTarget;
        subresourceLayersTarget.aspectMask = pStagingTexture->GetAspectMask();
        subresourceLayersTarget.mipLevel = uiMipLevel;
        subresourceLayersTarget.baseArrayLayer = 0;
        subresourceLayersTarget.layerCount = arraySize;

        vk::Extent3D mipLevelSize = pVulkanTexture->GetMipLevelSize(0);
        copy.Copy({0, 0, 0}, subresourceLayersSource, {0, 0, 0}, subresourceLayersTarget, {mipLevelSize.width, mipLevelSize.height, mipLevelSize.depth});
      }
    }
    else
    {
      copy.Init(pVulkanTexture, pStagingTexture, ezShaderUtils::ezBuiltinShaderType::CopyImage);
      const ezUInt32 arraySize = textureDesc.m_Type == ezGALTextureType::TextureCube ? textureDesc.m_uiArraySize * 6 : textureDesc.m_uiArraySize;
      const ezUInt32 mipLevels = textureDesc.m_uiMipLevelCount;

      for (ezUInt32 uiLayer = 0; uiLayer < arraySize; uiLayer++)
      {
        for (ezUInt32 uiMipLevel = 0; uiMipLevel < mipLevels; uiMipLevel++)
        {
          vk::ImageSubresourceLayers subresourceLayersSource;
          subresourceLayersSource.aspectMask = imageAspect;
          subresourceLayersSource.mipLevel = uiMipLevel;
          subresourceLayersSource.baseArrayLayer = uiLayer;
          subresourceLayersSource.layerCount = 1;

          vk::ImageSubresourceLayers subresourceLayersTarget;
          subresourceLayersTarget.aspectMask = pStagingTexture->GetAspectMask();
          subresourceLayersTarget.mipLevel = uiMipLevel;
          subresourceLayersTarget.baseArrayLayer = uiLayer;
          subresourceLayersTarget.layerCount = 1;

          vk::Extent3D mipLevelSize = pVulkanTexture->GetMipLevelSize(0);
          copy.Copy({0, 0, 0}, subresourceLayersSource, {0, 0, 0}, subresourceLayersTarget, {mipLevelSize.width, mipLevelSize.height, mipLevelSize.depth});
        }
      }
    }

    m_bPipelineStateDirty = true;
    m_bViewportDirty = true;
    m_bDescriptorsDirty = true;

    if (stagingMode == ezGALTextureVulkan::StagingMode::TextureAndBuffer)
    {
      // Copy to buffer
      const ezGALBufferVulkan* pStagingBuffer = static_cast<const ezGALBufferVulkan*>(m_GALDeviceVulkan.GetBuffer(pVulkanTexture->GetStagingBuffer()));
      CopyImageToBuffer(pVulkanTexture, pStagingBuffer);
    }
    else
    {
      // Readback texture directly
      m_pPipelineBarrier->EnsureImageLayout(pStagingTexture, vk::ImageLayout::eTransferSrcOptimal, vk::PipelineStageFlagBits::eHost, vk::AccessFlagBits::eHostRead);
    }
  }

  // There is no need to change the layout back of this texture right now but as the next layout will most certainly not be another eTransferSrcOptimal we might as well change it back to its default state.
  m_pPipelineBarrier->EnsureImageLayout(pVulkanTexture, pVulkanTexture->GetPreferredLayout(), pVulkanTexture->GetUsedByPipelineStage(), pVulkanTexture->GetAccessMask());

  //#TODO_VULKAN readback fence
  m_GALDeviceVulkan.Submit({}, {}, {});
  m_vkDevice.waitIdle();
  m_pPipelineBarrier = &m_GALDeviceVulkan.GetCurrentPipelineBarrier();
  m_pCommandBuffer = &m_GALDeviceVulkan.GetCurrentCommandBuffer();
}

ezUInt32 GetMipSize(ezUInt32 uiSize, ezUInt32 uiMipLevel)
{
  for (ezUInt32 i = 0; i < uiMipLevel; i++)
  {
    uiSize = uiSize / 2;
  }
  return ezMath::Max(1u, uiSize);
}

void ezGALCommandEncoderImplVulkan::CopyTextureReadbackResultPlatform(const ezGALTexture* pTexture, ezArrayPtr<ezGALTextureSubresource> SourceSubResource, ezArrayPtr<ezGALSystemMemoryDescription> TargetData)
{
  //#TODO_VULKAN readback fence
  auto pVulkanTexture = static_cast<const ezGALTextureVulkan*>(pTexture->GetParentResource());
  const ezGALTextureCreationDescription& textureDesc = pVulkanTexture->GetDescription();
  const ezGALTextureVulkan::StagingMode stagingMode = pVulkanTexture->GetStagingMode();
  EZ_ASSERT_DEV(stagingMode != ezGALTextureVulkan::StagingMode::None, "No staging resource available for read-back");

  if (stagingMode == ezGALTextureVulkan::StagingMode::Texture)
  {
    const ezGALTextureVulkan* pStagingTexture = static_cast<const ezGALTextureVulkan*>(m_GALDeviceVulkan.GetTexture(pVulkanTexture->GetStagingTexture()));
    vk::ImageAspectFlags stagingAspect = pStagingTexture->GetAspectMask();

    const ezUInt32 uiSubResources = SourceSubResource.GetCount();

    void* pData = nullptr;
    ezMemoryAllocatorVulkan::MapMemory(pStagingTexture->GetAllocation(), &pData);

    for (ezUInt32 i = 0; i < uiSubResources; i++)
    {
      const ezGALTextureSubresource& subRes = SourceSubResource[i];
      const ezGALSystemMemoryDescription& memDesc = TargetData[i];

      vk::ImageSubresource subResource{stagingAspect, subRes.m_uiMipLevel, subRes.m_uiArraySlice};
      vk::SubresourceLayout subResourceLayout;
      m_vkDevice.getImageSubresourceLayout(pStagingTexture->GetImage(), &subResource, &subResourceLayout);
      ezUInt8* pSubResourceData = reinterpret_cast<ezUInt8*>(pData) + subResourceLayout.offset;

      if (subResourceLayout.rowPitch == memDesc.m_uiRowPitch)
      {
        const ezUInt32 uiMemorySize = ezGALResourceFormat::GetBitsPerElement(textureDesc.m_Format) *
                                      GetMipSize(textureDesc.m_uiWidth, subRes.m_uiMipLevel) *
                                      GetMipSize(textureDesc.m_uiHeight, subRes.m_uiMipLevel) / 8;

        memcpy(memDesc.m_pData, pSubResourceData, uiMemorySize);
      }
      else
      {
        // Copy row by row
        const ezUInt32 uiHeight = GetMipSize(textureDesc.m_uiHeight, subRes.m_uiMipLevel);
        for (ezUInt32 y = 0; y < uiHeight; ++y)
        {
          const void* pSource = ezMemoryUtils::AddByteOffset(pSubResourceData, y * subResourceLayout.rowPitch);
          void* pDest = ezMemoryUtils::AddByteOffset(memDesc.m_pData, y * memDesc.m_uiRowPitch);

          memcpy(pDest, pSource, ezGALResourceFormat::GetBitsPerElement(textureDesc.m_Format) * GetMipSize(textureDesc.m_uiWidth, subRes.m_uiMipLevel) / 8);
        }
      }
    }

    ezMemoryAllocatorVulkan::UnmapMemory(pStagingTexture->GetAllocation());
  }
  else // One of the buffer variants.
  {
    const ezGALBufferVulkan* pStagingBuffer = static_cast<const ezGALBufferVulkan*>(m_GALDeviceVulkan.GetBuffer(pVulkanTexture->GetStagingBuffer()));
    const vk::Format stagingFormat = m_GALDeviceVulkan.GetFormatLookupTable().GetFormatInfo(pVulkanTexture->GetDescription().m_Format).m_readback;

    ezHybridArray<ezGALTextureVulkan::SubResourceOffset, 8> subResourceOffsets;
    const ezUInt32 uiBufferSize = pVulkanTexture->ComputeSubResourceOffsets(subResourceOffsets);


    const ezUInt32 uiSubResources = SourceSubResource.GetCount();

    void* pData = nullptr;
    ezMemoryAllocatorVulkan::MapMemory(pStagingBuffer->GetAllocation(), &pData);

    const ezUInt32 uiMipLevels = textureDesc.m_uiMipLevelCount;
    for (ezUInt32 i = 0; i < uiSubResources; i++)
    {
      const ezGALTextureSubresource& subRes = SourceSubResource[i];
      const ezUInt32 uiSubresourceIndex = subRes.m_uiMipLevel + subRes.m_uiArraySlice * uiMipLevels;
      const ezGALTextureVulkan::SubResourceOffset offset = subResourceOffsets[uiSubresourceIndex];
      const ezGALSystemMemoryDescription& memDesc = TargetData[i];
      const auto blockExtent = vk::blockExtent(stagingFormat);
      const ezUInt8 uiBlockSize = vk::blockSize(stagingFormat);

      const ezUInt32 uiRowPitch = offset.m_uiRowLength * blockExtent[0] * uiBlockSize;

      ezUInt8* pSubResourceData = reinterpret_cast<ezUInt8*>(pData) + offset.m_uiOffset;

      if (uiRowPitch == memDesc.m_uiRowPitch)
      {
        const ezUInt32 uiMemorySize = ezGALResourceFormat::GetBitsPerElement(textureDesc.m_Format) *
                                      GetMipSize(textureDesc.m_uiWidth, subRes.m_uiMipLevel) *
                                      GetMipSize(textureDesc.m_uiHeight, subRes.m_uiMipLevel) / 8;

        memcpy(memDesc.m_pData, pSubResourceData, uiMemorySize);
      }
      else
      {
        // Copy row by row
        const ezUInt32 uiHeight = GetMipSize(textureDesc.m_uiHeight, subRes.m_uiMipLevel);
        for (ezUInt32 y = 0; y < uiHeight; ++y)
        {
          const void* pSource = ezMemoryUtils::AddByteOffset(pSubResourceData, y * uiRowPitch);
          void* pDest = ezMemoryUtils::AddByteOffset(memDesc.m_pData, y * memDesc.m_uiRowPitch);

          memcpy(pDest, pSource, ezGALResourceFormat::GetBitsPerElement(textureDesc.m_Format) * GetMipSize(textureDesc.m_uiWidth, subRes.m_uiMipLevel) / 8);
        }
      }
    }

    ezMemoryAllocatorVulkan::UnmapMemory(pStagingBuffer->GetAllocation());
  }
}

void ezGALCommandEncoderImplVulkan::GenerateMipMapsPlatform(const ezGALResourceView* pResourceView)
{
  const ezGALResourceViewVulkan* pVulkanResourceView = static_cast<const ezGALResourceViewVulkan*>(pResourceView);
  if (m_bRenderPassActive)
  {
    m_pCommandBuffer->endRenderPass();
    m_bRenderPassActive = false;
  }


  const vk::ImageSubresourceRange viewRange = pVulkanResourceView->GetRange();
  if (viewRange.levelCount == 1)
    return;

  const ezGALTextureVulkan* pVulkanTexture = static_cast<const ezGALTextureVulkan*>(pVulkanResourceView->GetResource()->GetParentResource());
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
      m_pPipelineBarrier->EnsureImageLayout(pVulkanResourceView, pVulkanTexture->GetPreferredLayout(), pVulkanTexture->GetUsedByPipelineStage(), pVulkanTexture->GetAccessMask());
    }
    else
    {
      {
        vk::ImageSubresourceRange otherLevels = viewRange;
        otherLevels.baseMipLevel += 1;
        otherLevels.levelCount -= 1;
        m_pPipelineBarrier->EnsureImageLayout(pVulkanTexture, otherLevels, vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite);
      }

      ezImageCopyVulkan copy(m_GALDeviceVulkan);
      const bool bStereoSupport = m_GALDeviceVulkan.GetCapabilities().m_bVertexShaderRenderTargetArrayIndex || m_GALDeviceVulkan.GetCapabilities().m_bShaderStageSupported[ezGALShaderStage::GeometryShader];
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
        const ezUInt32 arraySize = textureDesc.m_Type == ezGALTextureType::TextureCube ? textureDesc.m_uiArraySize * 6 : textureDesc.m_uiArraySize;
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

      m_pPipelineBarrier->EnsureImageLayout(pVulkanResourceView, pVulkanTexture->GetPreferredLayout(), pVulkanTexture->GetUsedByPipelineStage(), pVulkanTexture->GetAccessMask());

      m_bPipelineStateDirty = true;
      m_bViewportDirty = true;
      m_bDescriptorsDirty = true;
    }
  }
}

void ezGALCommandEncoderImplVulkan::FlushPlatform()
{
  FlushDeferredStateChanges();
}

// Debug helper functions

void ezGALCommandEncoderImplVulkan::PushMarkerPlatform(const char* szMarker)
{
  // TODO early out if device doesn't support debug markers
  constexpr float markerColor[4] = {1, 1, 1, 1};
  vk::DebugUtilsLabelEXT markerInfo = {};
  ezMemoryUtils::Copy(markerInfo.color.data(), markerColor, EZ_ARRAY_SIZE(markerColor));
  markerInfo.pLabelName = szMarker;

  m_pCommandBuffer->beginDebugUtilsLabelEXT(markerInfo);
}

void ezGALCommandEncoderImplVulkan::PopMarkerPlatform()
{
  m_pCommandBuffer->endDebugUtilsLabelEXT();
}

void ezGALCommandEncoderImplVulkan::InsertEventMarkerPlatform(const char* szMarker)
{
  constexpr float markerColor[4] = {1, 1, 1, 1};
  vk::DebugUtilsLabelEXT markerInfo = {};
  ezMemoryUtils::Copy(markerInfo.color.data(), markerColor, EZ_ARRAY_SIZE(markerColor));
  markerInfo.pLabelName = szMarker;
  m_pCommandBuffer->insertDebugUtilsLabelEXT(markerInfo);
}

//////////////////////////////////////////////////////////////////////////

void ezGALCommandEncoderImplVulkan::BeginRendering(const ezGALRenderingSetup& renderingSetup)
{
  m_PipelineDesc.m_renderPass = ezResourceCacheVulkan::RequestRenderPass(renderingSetup);
  m_PipelineDesc.m_uiAttachmentCount = renderingSetup.m_RenderTargetSetup.GetRenderTargetCount();
  ezSizeU32 size;
  m_frameBuffer = ezResourceCacheVulkan::RequestFrameBuffer(m_PipelineDesc.m_renderPass, renderingSetup.m_RenderTargetSetup, size, m_PipelineDesc.m_msaa, m_uiLayers);

  SetScissorRectPlatform(ezRectU32(size.width, size.height));

  {
    m_renderPass.renderPass = m_PipelineDesc.m_renderPass;
    m_renderPass.framebuffer = m_frameBuffer;
    m_renderPass.renderArea.offset.setX(0).setY(0);
    m_renderPass.renderArea.extent.setHeight(size.height).setWidth(size.width);

    m_clearValues.Clear();

    const bool m_bHasDepth = !renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget().IsInvalidated();
    const ezUInt32 uiColorCount = renderingSetup.m_RenderTargetSetup.GetRenderTargetCount();
    m_bClearSubmitted = !(renderingSetup.m_bClearDepth || renderingSetup.m_bClearStencil || renderingSetup.m_uiRenderTargetClearMask);

    if (m_bHasDepth)
    {
      vk::ClearValue& depthClear = m_clearValues.ExpandAndGetRef();
      depthClear.depthStencil.setDepth(1.0f).setStencil(0);

      const ezGALRenderTargetViewVulkan* pRenderTargetView = static_cast<const ezGALRenderTargetViewVulkan*>(m_GALDeviceVulkan.GetRenderTargetView(renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget()));
      m_depthMask = pRenderTargetView->GetRange().aspectMask;
      m_pPipelineBarrier->EnsureImageLayout(pRenderTargetView, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests, vk::AccessFlagBits::eDepthStencilAttachmentWrite);
    }
    for (ezUInt32 i = 0; i < uiColorCount; i++)
    {
      vk::ClearValue& colorClear = m_clearValues.ExpandAndGetRef();
      ezColor col = renderingSetup.m_ClearColor;
      colorClear.color.setFloat32({col.r, col.g, col.b, col.a});

      const ezGALRenderTargetViewVulkan* pRenderTargetView = static_cast<const ezGALRenderTargetViewVulkan*>(m_GALDeviceVulkan.GetRenderTargetView(renderingSetup.m_RenderTargetSetup.GetRenderTarget(i)));
      m_pPipelineBarrier->EnsureImageLayout(pRenderTargetView, vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite);
    }

    m_renderPass.clearValueCount = m_clearValues.GetCount();
    m_renderPass.pClearValues = m_clearValues.GetData();
  }

  m_bPipelineStateDirty = true;
  m_bViewportDirty = true;
}

void ezGALCommandEncoderImplVulkan::EndRendering()
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
  m_PipelineDesc.m_msaa = ezGALMSAASampleCount::None;
  m_PipelineDesc.m_renderPass = nullptr;
  m_frameBuffer = nullptr;
}

void ezGALCommandEncoderImplVulkan::ClearPlatform(const ezColor& ClearColor, ezUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, ezUInt8 uiStencilClear)
{
  if (!m_bRenderPassActive && !m_bInsideCompute)
  {
    m_pCommandBuffer->beginRenderPass(m_renderPass, vk::SubpassContents::eInline);
    m_bClearSubmitted = true;
    m_bRenderPassActive = true;
  }
  //#TODO_VULKAN Not sure if we need barriers here.
  ezHybridArray<vk::ClearAttachment, 8> attachments;

  // Clear color
  if (uiRenderTargetClearMask != 0)
  {
    for (ezUInt32 i = 0; i < EZ_GAL_MAX_RENDERTARGET_COUNT; i++)
    {
      if (uiRenderTargetClearMask & (1u << i) && i < m_PipelineDesc.m_uiAttachmentCount)
      {
        vk::ClearAttachment& attachment = attachments.ExpandAndGetRef();
        attachment.aspectMask = vk::ImageAspectFlagBits::eColor;
        attachment.clearValue.color.setFloat32({ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a});
        attachment.colorAttachment = i;
      }
    }
  }
  // Clear depth / stencil
  if (bClearDepth || bClearStencil)
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

void ezGALCommandEncoderImplVulkan::DrawPlatform(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

  m_pCommandBuffer->draw(uiVertexCount, 1, uiStartVertex, 0);
}

void ezGALCommandEncoderImplVulkan::DrawIndexedPlatform(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex)
{
  FlushDeferredStateChanges();

  m_pCommandBuffer->drawIndexed(uiIndexCount, 1, uiStartIndex, 0, 0);
}

void ezGALCommandEncoderImplVulkan::DrawIndexedInstancedPlatform(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex)
{
  FlushDeferredStateChanges();

  m_pCommandBuffer->drawIndexed(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex, 0, 0);
}

void ezGALCommandEncoderImplVulkan::DrawIndexedInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  m_pCommandBuffer->drawIndexedIndirect(static_cast<const ezGALBufferVulkan*>(pIndirectArgumentBuffer)->GetVkBuffer(), uiArgumentOffsetInBytes, 1, 0);
}

void ezGALCommandEncoderImplVulkan::DrawInstancedPlatform(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

  m_pCommandBuffer->draw(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex, 0);
}

void ezGALCommandEncoderImplVulkan::DrawInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  m_pCommandBuffer->drawIndirect(static_cast<const ezGALBufferVulkan*>(pIndirectArgumentBuffer)->GetVkBuffer(), uiArgumentOffsetInBytes, 1, 0);
}

void ezGALCommandEncoderImplVulkan::DrawAutoPlatform()
{
  //FlushDeferredStateChanges();

  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALCommandEncoderImplVulkan::BeginStreamOutPlatform()
{
  FlushDeferredStateChanges();
}

void ezGALCommandEncoderImplVulkan::EndStreamOutPlatform()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALCommandEncoderImplVulkan::SetIndexBufferPlatform(const ezGALBuffer* pIndexBuffer)
{
  if (m_pIndexBuffer != pIndexBuffer)
  {
    m_pIndexBuffer = static_cast<const ezGALBufferVulkan*>(pIndexBuffer);
    m_bIndexBufferDirty = true;
  }
}

void ezGALCommandEncoderImplVulkan::SetVertexBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pVertexBuffer)
{
  EZ_ASSERT_DEV(uiSlot < EZ_GAL_MAX_VERTEX_BUFFER_COUNT, "Invalid slot index");
  vk::Buffer buffer = pVertexBuffer != nullptr ? static_cast<const ezGALBufferVulkan*>(pVertexBuffer)->GetVkBuffer() : nullptr;
  ezUInt32 stride = pVertexBuffer != nullptr ? pVertexBuffer->GetDescription().m_uiStructSize : 0;

  if (buffer != m_pBoundVertexBuffers[uiSlot])
  {
    m_pBoundVertexBuffers[uiSlot] = buffer;
    m_BoundVertexBuffersRange.SetToIncludeValue(uiSlot);

    if (m_PipelineDesc.m_VertexBufferStrides[uiSlot] != stride)
    {
      m_PipelineDesc.m_VertexBufferStrides[uiSlot] = stride;
      m_bPipelineStateDirty = true;
    }
  }
}

void ezGALCommandEncoderImplVulkan::SetVertexDeclarationPlatform(const ezGALVertexDeclaration* pVertexDeclaration)
{
  if (m_PipelineDesc.m_pCurrentVertexDecl != pVertexDeclaration)
  {
    m_PipelineDesc.m_pCurrentVertexDecl = static_cast<const ezGALVertexDeclarationVulkan*>(pVertexDeclaration);
    m_bPipelineStateDirty = true;
  }
}

void ezGALCommandEncoderImplVulkan::SetPrimitiveTopologyPlatform(ezGALPrimitiveTopology::Enum Topology)
{
  if (m_PipelineDesc.m_topology != Topology)
  {
    m_PipelineDesc.m_topology = Topology;
    m_bPipelineStateDirty = true;
  }
}

void ezGALCommandEncoderImplVulkan::SetBlendStatePlatform(const ezGALBlendState* pBlendState, const ezColor& BlendFactor, ezUInt32 uiSampleMask)
{
  //#TODO_VULKAN BlendFactor / uiSampleMask ?
  if (m_PipelineDesc.m_pCurrentBlendState != pBlendState)
  {
    m_PipelineDesc.m_pCurrentBlendState = pBlendState != nullptr ? static_cast<const ezGALBlendStateVulkan*>(pBlendState) : nullptr;
    m_bPipelineStateDirty = true;
  }
}

void ezGALCommandEncoderImplVulkan::SetDepthStencilStatePlatform(const ezGALDepthStencilState* pDepthStencilState, ezUInt8 uiStencilRefValue)
{
  //#TODO_VULKAN uiStencilRefValue ?
  if (m_PipelineDesc.m_pCurrentDepthStencilState != pDepthStencilState)
  {
    m_PipelineDesc.m_pCurrentDepthStencilState = pDepthStencilState != nullptr ? static_cast<const ezGALDepthStencilStateVulkan*>(pDepthStencilState) : nullptr;
    m_bPipelineStateDirty = true;
  }
}

void ezGALCommandEncoderImplVulkan::SetRasterizerStatePlatform(const ezGALRasterizerState* pRasterizerState)
{
  if (m_PipelineDesc.m_pCurrentRasterizerState != pRasterizerState)
  {
    m_PipelineDesc.m_pCurrentRasterizerState = pRasterizerState != nullptr ? static_cast<const ezGALRasterizerStateVulkan*>(pRasterizerState) : nullptr;
    if (m_PipelineDesc.m_pCurrentRasterizerState->GetDescription().m_bScissorTest != m_bScissorEnabled)
    {
      m_bScissorEnabled = m_PipelineDesc.m_pCurrentRasterizerState->GetDescription().m_bScissorTest;
      m_bViewportDirty = true;
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

void ezGALCommandEncoderImplVulkan::SetStreamOutBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pBuffer, ezUInt32 uiOffset)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

//////////////////////////////////////////////////////////////////////////

void ezGALCommandEncoderImplVulkan::BeginCompute()
{
  m_bClearSubmitted = true;
  m_bInsideCompute = true;
  m_bPipelineStateDirty = true;
}

void ezGALCommandEncoderImplVulkan::EndCompute()
{
  m_bInsideCompute = false;
}

void ezGALCommandEncoderImplVulkan::DispatchPlatform(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ)
{
  FlushDeferredStateChanges();
  m_pCommandBuffer->dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

void ezGALCommandEncoderImplVulkan::DispatchIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();
  m_pCommandBuffer->dispatchIndirect(static_cast<const ezGALBufferVulkan*>(pIndirectArgumentBuffer)->GetVkBuffer(), uiArgumentOffsetInBytes);
}

//////////////////////////////////////////////////////////////////////////

#if 0
static void SetShaderResources(ezGALShaderStage::Enum stage, ID3D11DeviceContext* pContext, ezUInt32 uiStartSlot, ezUInt32 uiNumSlots,
  ID3D11ShaderResourceView** pShaderResourceViews)
{
  switch (stage)
  {
    case ezGALShaderStage::VertexShader:
      pContext->VSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case ezGALShaderStage::HullShader:
      pContext->HSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case ezGALShaderStage::DomainShader:
      pContext->DSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case ezGALShaderStage::GeometryShader:
      pContext->GSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case ezGALShaderStage::PixelShader:
      pContext->PSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case ezGALShaderStage::ComputeShader:
      pContext->CSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }
}

static void SetConstantBuffers(ezGALShaderStage::Enum stage, ID3D11DeviceContext* pContext, ezUInt32 uiStartSlot, ezUInt32 uiNumSlots,
  ID3D11Buffer** pConstantBuffers)
{
  switch (stage)
  {
    case ezGALShaderStage::VertexShader:
      pContext->VSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case ezGALShaderStage::HullShader:
      pContext->HSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case ezGALShaderStage::DomainShader:
      pContext->DSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case ezGALShaderStage::GeometryShader:
      pContext->GSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case ezGALShaderStage::PixelShader:
      pContext->PSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case ezGALShaderStage::ComputeShader:
      pContext->CSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }
}

static void SetSamplers(ezGALShaderStage::Enum stage, ID3D11DeviceContext* pContext, ezUInt32 uiStartSlot, ezUInt32 uiNumSlots,
  ID3D11SamplerState** pSamplerStates)
{
  switch (stage)
  {
    case ezGALShaderStage::VertexShader:
      pContext->VSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case ezGALShaderStage::HullShader:
      pContext->HSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case ezGALShaderStage::DomainShader:
      pContext->DSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case ezGALShaderStage::GeometryShader:
      pContext->GSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case ezGALShaderStage::PixelShader:
      pContext->PSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case ezGALShaderStage::ComputeShader:
      pContext->CSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }
}
#endif

void ezGALCommandEncoderImplVulkan::FlushDeferredStateChanges()
{


  if (m_bPipelineStateDirty)
  {
    if (!m_PipelineDesc.m_pCurrentShader)
    {
      ezLog::Error("No shader set");
      return;
    }
    const ezGALShaderVulkan::DescriptorSetLayoutDesc& descriptorLayoutDesc = m_PipelineDesc.m_pCurrentShader->GetDescriptorSetLayout();

    m_LayoutDesc.m_layout = ezResourceCacheVulkan::RequestDescriptorSetLayout(descriptorLayoutDesc);
    m_PipelineDesc.m_layout = ezResourceCacheVulkan::RequestPipelineLayout(m_LayoutDesc);
    m_ComputeDesc.m_layout = m_PipelineDesc.m_layout;

    vk::Pipeline pipeline;
    if (m_bInsideCompute)
    {
      pipeline = ezResourceCacheVulkan::RequestComputePipeline(m_ComputeDesc);
    }
    else
    {
      pipeline = ezResourceCacheVulkan::RequestGraphicsPipeline(m_PipelineDesc);
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
    //#TODO_VULKAN we always create a new descriptor set as we don't know if a buffer was modified since the last draw call (ezGALBufferVulkan::DiscardBuffer).
    // Need to figure out a fast check if any buffer or buffer of a resource view was discarded.
    m_bDescriptorsDirty = false;

    m_DescriptorWrites.Clear();
    vk::DescriptorSet descriptorSet = ezDescriptorSetPoolVulkan::CreateDescriptorSet(m_LayoutDesc.m_layout);

    ezArrayPtr<const ezGALShaderVulkan::BindingMapping> bindingMapping = m_PipelineDesc.m_pCurrentShader->GetBindingMapping();
    const ezUInt32 uiCount = bindingMapping.GetCount();
    for (ezUInt32 i = 0; i < uiCount; i++)
    {
      const ezGALShaderVulkan::BindingMapping& mapping = bindingMapping[i];
      vk::WriteDescriptorSet& write = m_DescriptorWrites.ExpandAndGetRef();
      write.dstArrayElement = 0;
      write.descriptorType = mapping.m_descriptorType;
      write.dstBinding = mapping.m_uiTarget;
      write.dstSet = descriptorSet;
      write.descriptorCount = 1;
      switch (mapping.m_type)
      {
        case ezGALShaderVulkan::BindingMapping::ConstantBuffer:
        {
          const ezGALBufferVulkan* pBuffer = m_pBoundConstantBuffers[mapping.m_uiSource];
          write.pBufferInfo = &pBuffer->GetBufferInfo();
        }
        break;
        case ezGALShaderVulkan::BindingMapping::ResourceView:
        {
          const ezGALResourceViewVulkan* pResourceView = nullptr;
          if (mapping.m_uiSource < m_pBoundShaderResourceViews[mapping.m_stage].GetCount())
          {
            pResourceView = m_pBoundShaderResourceViews[mapping.m_stage][mapping.m_uiSource];
          }

          if (!pResourceView)
          {
            ezStringBuilder bla = mapping.m_sName;
            bool bDepth = bla.FindSubString_NoCase("shadow") != nullptr || bla.FindSubString_NoCase("depth");
            pResourceView = ezFallbackResourcesVulkan::GetFallbackResourceView(mapping.m_descriptorType, mapping.m_ezType, bDepth);
          }
          if (!pResourceView->GetDescription().m_hTexture.IsInvalidated())
          {
            write.pImageInfo = &pResourceView->GetImageInfo(ezShaderResourceType::IsArray(mapping.m_ezType));

            const ezGALTextureVulkan* pTexture = static_cast<const ezGALTextureVulkan*>(pResourceView->GetResource()->GetParentResource());
            const bool bIsDepth = ezGALResourceFormat::IsDepthFormat(pTexture->GetDescription().m_Format);

            m_pPipelineBarrier->EnsureImageLayout(pResourceView, pTexture->GetPreferredLayout(bIsDepth ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eShaderReadOnlyOptimal), mapping.m_targetStages, vk::AccessFlagBits::eShaderRead);
          }
          else
          {
            if (auto& bufferView = pResourceView->GetBufferView())
            {
              write.pTexelBufferView = &bufferView;
            }
            else
            {
              write.pBufferInfo = &pResourceView->GetBufferInfo();
            }
          }
        }
        break;
        case ezGALShaderVulkan::BindingMapping::UAV:
        {
          const ezGALUnorderedAccessViewVulkan* pUAV = m_pBoundUnoderedAccessViews[mapping.m_uiSource];
          if (!pUAV->GetDescription().m_hTexture.IsInvalidated())
          {
            write.pImageInfo = &pUAV->GetImageInfo();

            const ezGALTextureVulkan* pTexture = static_cast<const ezGALTextureVulkan*>(pUAV->GetResource()->GetParentResource());
            m_pPipelineBarrier->EnsureImageLayout(pUAV, pTexture->GetPreferredLayout(vk::ImageLayout::eGeneral), mapping.m_targetStages, vk::AccessFlagBits::eShaderRead);
          }
          else
          {
            write.pBufferInfo = &pUAV->GetBufferInfo();
          }
        }
        break;
        case ezGALShaderVulkan::BindingMapping::Sampler:
        {
          const ezGALSamplerStateVulkan* pSampler = m_pBoundSamplerStates[mapping.m_stage][mapping.m_uiSource];
          write.pImageInfo = &pSampler->GetImageInfo();
        }
        break;
        default:
          break;
      }
    }

    ezDescriptorSetPoolVulkan::UpdateDescriptorSet(descriptorSet, m_DescriptorWrites);
    m_pCommandBuffer->bindDescriptorSets(m_bInsideCompute ? vk::PipelineBindPoint::eCompute : vk::PipelineBindPoint::eGraphics, m_PipelineDesc.m_layout, 0, 1, &descriptorSet, 0, nullptr);
  }

  if (m_bRenderPassActive && m_pPipelineBarrier->IsDirty())
  {
    m_pCommandBuffer->endRenderPass();
    m_bRenderPassActive = false;
  }
  m_pPipelineBarrier->Flush();

  if (!m_bRenderPassActive && !m_bInsideCompute)
  {
    m_pCommandBuffer->beginRenderPass(m_renderPass, vk::SubpassContents::eInline);
    m_bClearSubmitted = true;
    m_bRenderPassActive = true;
  }
}
