#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/CommandEncoder/CommandEncoderImplVulkan.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Pools/DescriptorSetPoolVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/QueryVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Resources/ResourceViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Resources/UnorderedAccessViewVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>

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

// State setting functions

void ezGALCommandEncoderImplVulkan::SetShaderPlatform(const ezGALShader* pShader)
{
  if (pShader != nullptr)
  {
    m_PipelineDesc.m_pCurrentShader = static_cast<const ezGALShaderVulkan*>(pShader);
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
  // TODO how to implement this in Vulkan?
  //ID3D11Query* pDXQuery = static_cast<ezGALDeviceVulkan*>(GetDevice())->GetTimestamp(hTimestamp);
  //
  //m_pDXContext->End(pDXQuery);

  EZ_ASSERT_NOT_IMPLEMENTED;
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
  vk::Buffer destination = static_cast<const ezGALBufferVulkan*>(pDestination)->GetVkBuffer();
  vk::Buffer source = static_cast<const ezGALBufferVulkan*>(pSource)->GetVkBuffer();

  EZ_ASSERT_DEV(pSource->GetSize() != pDestination->GetSize(), "Source and destination buffer sizes mismatch!");

  // TODO do this in an immediate command buffer?
  vk::BufferCopy bufferCopy = {};
  bufferCopy.size = pSource->GetSize();

  m_pCommandBuffer->copyBuffer(source, destination, 1, &bufferCopy);
}

void ezGALCommandEncoderImplVulkan::CopyBufferRegionPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, const ezGALBuffer* pSource,
  ezUInt32 uiSourceOffset, ezUInt32 uiByteCount)
{
  vk::Buffer destination = static_cast<const ezGALBufferVulkan*>(pDestination)->GetVkBuffer();
  vk::Buffer source = static_cast<const ezGALBufferVulkan*>(pSource)->GetVkBuffer();
  vk::BufferCopy bufferCopy = {};
  bufferCopy.dstOffset = uiDestOffset;
  bufferCopy.srcOffset = uiSourceOffset;
  bufferCopy.size = uiByteCount;

  m_pCommandBuffer->copyBuffer(source, destination, 1, &bufferCopy);
  //#TODO_VULKAN memory barrier needed here?
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
      ezMemoryUtils::Copy((ezUInt8*)pData, pSourceData.GetPtr(), pSourceData.GetCount());
      ezMemoryAllocatorVulkan::UnmapMemory(alloc);
    }
    break;
    case ezGALUpdateMode::CopyToTempStorage:
    {
      EZ_ASSERT_DEBUG(!m_bRenderPassActive, "Vulkan does not support copying buffers while a render pass is active. TODO: Fix high level render code to make this impossible.");

      m_GALDeviceVulkan.UploadBufferStaging(pVulkanDestination, pSourceData, uiDestOffset);
    }
    break;
    default:
      break;
  }
}

void ezGALCommandEncoderImplVulkan::CopyTexturePlatform(const ezGALTexture* pDestination, const ezGALTexture* pSource)
{
  EZ_ASSERT_DEBUG(!m_bRenderPassActive, "Vulkan does not support updating buffers while a render pass is active. TODO: Fix high level render code to make this impossible.");

  auto destination = static_cast<const ezGALTextureVulkan*>(pDestination);
  auto source = static_cast<const ezGALTextureVulkan*>(pSource);

  const ezGALTextureCreationDescription& destDesc = pDestination->GetDescription();
  const ezGALTextureCreationDescription& srcDesc = pSource->GetDescription();

  EZ_ASSERT_DEBUG(ezGALResourceFormat::IsDepthFormat(destDesc.m_Format) == ezGALResourceFormat::IsDepthFormat(srcDesc.m_Format), "");
  EZ_ASSERT_DEBUG(destDesc.m_uiArraySize == srcDesc.m_uiArraySize, "");
  EZ_ASSERT_DEBUG(destDesc.m_uiMipLevelCount == srcDesc.m_uiMipLevelCount, "");
  EZ_ASSERT_DEBUG(destDesc.m_uiWidth == srcDesc.m_uiWidth, "");
  EZ_ASSERT_DEBUG(destDesc.m_uiHeight == srcDesc.m_uiHeight, "");
  EZ_ASSERT_DEBUG(destDesc.m_uiDepth == srcDesc.m_uiDepth, "");

  vk::ImageAspectFlagBits imageAspect = ezGALResourceFormat::IsDepthFormat(destDesc.m_Format) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;

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

  m_pCommandBuffer->copyImage(source->GetImage(), vk::ImageLayout::eGeneral, destination->GetImage(), vk::ImageLayout::eGeneral, destDesc.m_uiMipLevelCount, imageCopies.GetData());
}

void ezGALCommandEncoderImplVulkan::CopyTextureRegionPlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
  const ezVec3U32& DestinationPoint, const ezGALTexture* pSource,
  const ezGALTextureSubresource& SourceSubResource, const ezBoundingBoxu32& Box)
{
  auto destination = static_cast<const ezGALTextureVulkan*>(pDestination);
  auto source = static_cast<const ezGALTextureVulkan*>(pSource);

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
  const ezBoundingBoxu32& DestinationBox, const ezGALSystemMemoryDescription& pSourceData)
{
  //#TODO_VULKAN texture implementation missing.
  ezUInt32 uiWidth = ezMath::Max(DestinationBox.m_vMax.x - DestinationBox.m_vMin.x, 1u);
  ezUInt32 uiHeight = ezMath::Max(DestinationBox.m_vMax.y - DestinationBox.m_vMin.y, 1u);
  ezUInt32 uiDepth = ezMath::Max(DestinationBox.m_vMax.z - DestinationBox.m_vMin.z, 1u);
  ezGALResourceFormat::Enum format = pDestination->GetDescription().m_Format;

  if (ezGALTextureVulkan* pTempTexture = nullptr)
  {
    ezVulkanAllocation alloc = pTempTexture->GetAllocation();
    void* pData = nullptr;

    VK_ASSERT_DEV(ezMemoryAllocatorVulkan::MapMemory(alloc, &pData));
    EZ_ASSERT_DEV(pData, "Implementation error");

    ezUInt32 uiRowPitch = uiWidth * ezGALResourceFormat::GetBitsPerElement(format) / 8;
    ezUInt32 uiSlicePitch = uiRowPitch * uiHeight;
    EZ_ASSERT_DEV(pSourceData.m_uiRowPitch == uiRowPitch, "Invalid row pitch. Expected {0} got {1}", uiRowPitch, pSourceData.m_uiRowPitch);
    EZ_ASSERT_DEV(pSourceData.m_uiSlicePitch == 0 || pSourceData.m_uiSlicePitch == uiSlicePitch,
      "Invalid slice pitch. Expected {0} got {1}", uiSlicePitch, pSourceData.m_uiSlicePitch);

    ezMemoryUtils::RawByteCopy(pData, pSourceData.m_pData, uiSlicePitch * uiDepth);

    ezMemoryAllocatorVulkan::UnmapMemory(alloc);

    ezGALTextureSubresource sourceSubResource;
    sourceSubResource.m_uiArraySlice = 0;
    sourceSubResource.m_uiMipLevel = 0;
    ezBoundingBoxu32 sourceBox;
    sourceBox.SetElements(ezVec3U32::ZeroVector(), DestinationBox.m_vMax - DestinationBox.m_vMin);
    CopyTextureRegionPlatform(pDestination, DestinationSubResource, DestinationBox.m_vMin, pTempTexture, sourceSubResource, sourceBox);
  }
  else
  {
    EZ_REPORT_FAILURE("Could not find a temp texture for update.");
  }
}

void ezGALCommandEncoderImplVulkan::ResolveTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
  const ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource)
{
  EZ_ASSERT_ALWAYS(DestinationSubResource.m_uiMipLevel == 0, "Resolving of higher mips not implemented yet!");
  EZ_ASSERT_ALWAYS(SourceSubResource.m_uiMipLevel == 0, "Resolving of higher mips not implemented yet!");

  auto pVulkanDestination = static_cast<const ezGALTextureVulkan*>(pDestination);
  auto pVulkanSource = static_cast<const ezGALTextureVulkan*>(pSource);

  const ezGALTextureCreationDescription& destDesc = pDestination->GetDescription();
  const ezGALTextureCreationDescription& srcDesc = pSource->GetDescription();

  EZ_ASSERT_DEBUG(ezGALResourceFormat::IsDepthFormat(destDesc.m_Format) == ezGALResourceFormat::IsDepthFormat(srcDesc.m_Format), "");

  vk::ImageAspectFlagBits imageAspect = ezGALResourceFormat::IsDepthFormat(destDesc.m_Format) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;

  // TODO need to determine size of the subresource
  vk::ImageResolve resolveRegion = {};
  resolveRegion.dstSubresource.aspectMask = imageAspect;
  resolveRegion.dstSubresource.baseArrayLayer = DestinationSubResource.m_uiArraySlice;
  resolveRegion.dstSubresource.layerCount = 1; // TODO is this correct?
  resolveRegion.dstSubresource.mipLevel = 0;   // TODO implement resolve of higher mips
  resolveRegion.extent.width = destDesc.m_uiWidth;
  resolveRegion.extent.height = destDesc.m_uiHeight;
  resolveRegion.extent.depth = destDesc.m_uiDepth;
  resolveRegion.srcSubresource.aspectMask = imageAspect;
  resolveRegion.srcSubresource.baseArrayLayer = DestinationSubResource.m_uiArraySlice;
  resolveRegion.srcSubresource.layerCount = 1;
  resolveRegion.srcSubresource.mipLevel = 0; // TODO implement resolve of higher mips

  m_pCommandBuffer->resolveImage(pVulkanSource->GetImage(), vk::ImageLayout::eGeneral, pVulkanDestination->GetImage(), vk::ImageLayout::eGeneral, 1, &resolveRegion);
}

void ezGALCommandEncoderImplVulkan::ReadbackTexturePlatform(const ezGALTexture* pTexture)
{
  const ezGALTextureVulkan* pVulkanTexture = static_cast<const ezGALTextureVulkan*>(pTexture);

  // MSAA textures (e.g. backbuffers) need to be converted to non MSAA versions
  const bool bMSAASourceTexture = pVulkanTexture->GetDescription().m_SampleCount != ezGALMSAASampleCount::None;

  EZ_ASSERT_DEV(pVulkanTexture->GetStagingBuffer(), "No staging resource available for read-back");
  EZ_ASSERT_DEV(pVulkanTexture->GetImage(), "Texture object is invalid");

  if (bMSAASourceTexture)
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }
  else
  {
    const ezGALTextureCreationDescription& textureDesc = pVulkanTexture->GetDescription();

    vk::ImageAspectFlagBits imageAspect = ezGALResourceFormat::IsDepthFormat(textureDesc.m_Format) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;

    vk::BufferImageCopy copyRegion = {};
    copyRegion.bufferImageHeight = textureDesc.m_uiHeight;
    copyRegion.bufferRowLength = textureDesc.m_uiWidth;
    copyRegion.bufferOffset = 0;
    copyRegion.imageExtent.width = textureDesc.m_uiWidth;
    copyRegion.imageExtent.height = textureDesc.m_uiWidth;
    copyRegion.imageExtent.depth = textureDesc.m_uiDepth;
    copyRegion.imageSubresource.aspectMask = imageAspect;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = textureDesc.m_uiArraySize;
    copyRegion.imageSubresource.mipLevel = 0; // TODO need to support all mip levels

    // TODO do we need to do this immediately?
    m_pCommandBuffer->copyImageToBuffer(pVulkanTexture->GetImage(), vk::ImageLayout::eGeneral, pVulkanTexture->GetStagingBuffer()->GetVkBuffer(), 1, &copyRegion);
  }
}

void ezGALCommandEncoderImplVulkan::CopyTextureReadbackResultPlatform(const ezGALTexture* pTexture, ezArrayPtr<ezGALTextureSubresource> SourceSubResource, ezArrayPtr<ezGALSystemMemoryDescription> TargetData)
{
  //#TODO_VULKAN changed interface
  auto pVulkanTexture = static_cast<const ezGALTextureVulkan*>(pTexture);

  const ezGALBufferVulkan* pStagingBuffer = pVulkanTexture->GetStagingBuffer();

  EZ_ASSERT_DEV(pStagingBuffer, "No staging resource available for read-back");

  const ezUInt32 uiSubResources = SourceSubResource.GetCount();
  for (ezUInt32 i = 0; i < uiSubResources; i++)
  {
    const ezGALTextureSubresource& subRes = SourceSubResource[i];
    const ezGALSystemMemoryDescription& memDesc = TargetData[i];

    //vkGetImageSubresourceLayout(m_vkDevice, )
    //
    //pStagingBuffer->

    //// Data should be tightly packed in the staging buffer already, so
    //// just map the memory and copy it over
    //void* pSrcData = m_vkDevice.mapMemory(pStagingBuffer->GetMemory(), pStagingBuffer->GetMemoryOffset(), pStagingBuffer->GetSize());
    //if (pSrcData)
    //{
    //  // TODO size of the buffer could missmatch the texture data size necessary
    //  ezMemoryUtils::RawByteCopy(pData->GetPtr()->m_pData, pSrcData, pStagingBuffer->GetSize());

    //  m_vkDevice.unmapMemory(pStagingBuffer->GetMemory());
    //}
  }
}

void ezGALCommandEncoderImplVulkan::GenerateMipMapsPlatform(const ezGALResourceView* pResourceView)
{
  const ezGALResourceViewVulkan* pDXResourceView = static_cast<const ezGALResourceViewVulkan*>(pResourceView);

  // TODO texture blit based approach
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
  vk::DebugMarkerMarkerInfoEXT markerInfo = {};
  ezMemoryUtils::Copy(markerInfo.color.data(), markerColor, EZ_ARRAY_SIZE(markerColor));
  markerInfo.pMarkerName = szMarker;
  m_pCommandBuffer->debugMarkerBeginEXT(markerInfo);
}

void ezGALCommandEncoderImplVulkan::PopMarkerPlatform()
{
  m_pCommandBuffer->debugMarkerEndEXT();
}

void ezGALCommandEncoderImplVulkan::InsertEventMarkerPlatform(const char* szMarker)
{
  constexpr float markerColor[4] = {1, 1, 1, 1};
  vk::DebugMarkerMarkerInfoEXT markerInfo = {};
  ezMemoryUtils::Copy(markerInfo.color.data(), markerColor, EZ_ARRAY_SIZE(markerColor));
  markerInfo.pMarkerName = szMarker;
  m_pCommandBuffer->debugMarkerInsertEXT(markerInfo);
}

//////////////////////////////////////////////////////////////////////////

void ezGALCommandEncoderImplVulkan::BeginRendering(vk::CommandBuffer& commandBuffer, const ezGALRenderingSetup& renderingSetup)
{
  m_pCommandBuffer = &commandBuffer;

  m_PipelineDesc.m_renderPass = ezResourceCacheVulkan::RequestRenderPass(renderingSetup);
  m_PipelineDesc.m_uiAttachmentCount = renderingSetup.m_RenderTargetSetup.GetRenderTargetCount();
  ezSizeU32 size;
  m_frameBuffer = ezResourceCacheVulkan::RequestFrameBuffer(m_PipelineDesc.m_renderPass, renderingSetup.m_RenderTargetSetup, size, m_PipelineDesc.m_msaa);

  SetScissorRectPlatform(ezRectU32(size.width, size.height));

  {
    m_renderPass.renderPass = m_PipelineDesc.m_renderPass;
    m_renderPass.framebuffer = m_frameBuffer;
    m_renderPass.renderArea.offset.setX(0).setY(0);
    m_renderPass.renderArea.extent.setHeight(size.height).setWidth(size.width);

    m_clearValues.Clear();

    const bool bHasDepth = !renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget().IsInvalidated();
    const ezUInt32 uiColorCount = renderingSetup.m_RenderTargetSetup.GetRenderTargetCount();
    if (bHasDepth)
    {
      vk::ClearValue& depthClear = m_clearValues.ExpandAndGetRef();
      depthClear.depthStencil.setDepth(1.0f).setStencil(0);
    }
    for (ezUInt32 i = 0; i < uiColorCount; i++)
    {
      vk::ClearValue& colorClear = m_clearValues.ExpandAndGetRef();
      ezColor col = renderingSetup.m_ClearColor;
      colorClear.color.setFloat32({col.r, col.g, col.b, col.a});
    }

    m_renderPass.clearValueCount = m_clearValues.GetCount();
    m_renderPass.pClearValues = m_clearValues.GetData();
  }

  m_bPipelineStateDirty = true;
  m_bViewportDirty = true;
}

void ezGALCommandEncoderImplVulkan::EndRendering()
{
  m_PipelineDesc.m_msaa = ezGALMSAASampleCount::None;
  m_PipelineDesc.m_renderPass = nullptr;
  m_frameBuffer = nullptr;

  m_pCommandBuffer->endRenderPass();
  m_pCommandBuffer = nullptr;
  m_bRenderPassActive = false;
}

void ezGALCommandEncoderImplVulkan::ClearPlatform(const ezColor& ClearColor, ezUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, ezUInt8 uiStencilClear)
{
  // TODO:
  EZ_ASSERT_NOT_IMPLEMENTED;
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
    m_bPipelineStateDirty = true;
  }
}

void ezGALCommandEncoderImplVulkan::SetViewportPlatform(const ezRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  // We use ezClipSpaceYMode::Regular and rely in the Vulkan 1.1 feature that a nagative height performs y-inversion of the clip-space to framebuffer-space transform.
  // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VK_KHR_maintenance1.html
  vk::Viewport viewport = {rect.x, rect.height + rect.y, rect.width, -rect.height, fMinDepth, fMaxDepth};
  if (m_viewport != viewport)
  {
    // viewport is marked as dynamic in the pipeline layout and thus does not mark m_bPipelineStateDirty.
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

void ezGALCommandEncoderImplVulkan::BeginCompute(vk::CommandBuffer& commandBuffer)
{
  m_pCommandBuffer = &commandBuffer;

  // TODO: do we need a renderpass for compute only?
}

void ezGALCommandEncoderImplVulkan::EndCompute()
{
  m_pCommandBuffer = nullptr;
}

void ezGALCommandEncoderImplVulkan::DispatchPlatform(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ)
{
  m_pCommandBuffer->dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

void ezGALCommandEncoderImplVulkan::DispatchIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
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
  if (!m_bRenderPassActive)
  {
    m_pCommandBuffer->beginRenderPass(m_renderPass, vk::SubpassContents::eInline);
    m_bRenderPassActive = true;
  }

  if (m_bPipelineStateDirty)
  {
    const ezGALShaderVulkan::DescriptorSetLayoutDesc& descriptorLayoutDesc = m_PipelineDesc.m_pCurrentShader->GetDescriptorSetLayout();

    m_LayoutDesc.m_layout = ezResourceCacheVulkan::RequestDescriptorSetLayout(descriptorLayoutDesc);
    m_PipelineDesc.m_layout = ezResourceCacheVulkan::RequestPipelineLayout(m_LayoutDesc);
    vk::Pipeline pipeline = ezResourceCacheVulkan::RequestGraphicsPipeline(m_PipelineDesc);

    m_pCommandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    m_bPipelineStateDirty = false;
    // Changes to the descriptor layout always require the descriptor set to be re-created.
    m_bDescriptorsDirty = true;
  }

  if (m_bViewportDirty)
  {
    m_pCommandBuffer->setViewport(0, 1, &m_viewport);
    m_pCommandBuffer->setScissor(0, 1, &m_scissor);
    m_bViewportDirty = false;
  }

  if (m_BoundVertexBuffersRange.IsValid())
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
    m_pCommandBuffer->bindVertexBuffers(uiCurrentStartSlot, m_BoundVertexBuffersRange.m_uiMax - uiCurrentStartSlot + 1, m_pBoundVertexBuffers + uiCurrentStartSlot, m_VertexBufferOffsets + uiCurrentStartSlot);

    m_BoundVertexBuffersRange.Reset();
  }

  if (m_bIndexBufferDirty)
  {
    if (m_pIndexBuffer)
      m_pCommandBuffer->bindIndexBuffer(m_pIndexBuffer->GetVkBuffer(), 0, m_pIndexBuffer->GetIndexType());
    m_bIndexBufferDirty = false;
  }

  if (true/*m_bDescriptorsDirty*/)
  {
    //#TODO_VULKAN we always create a new descriptor set as we don't know if a buffer was modified since the last draw call (ezGALBufferVulkan::DiscardBuffer).
    // Need to figure out a fast check if any buffer or buffer of a resource view was discarded.
    m_bDescriptorsDirty = false;

    ezHybridArray<vk::WriteDescriptorSet, 16> descriptorWrites;
    vk::DescriptorSet descriptorSet = ezDescriptorSetPoolVulkan::CreateDescriptorSet(m_LayoutDesc.m_layout);

    ezArrayPtr<const ezGALShaderVulkan::BindingMapping> bindingMapping = m_PipelineDesc.m_pCurrentShader->GetBindingMapping();
    const ezUInt32 uiCount = bindingMapping.GetCount();
    for (ezUInt32 i = 0; i < uiCount; i++)
    {
      const ezGALShaderVulkan::BindingMapping& mapping = bindingMapping[i];
      vk::WriteDescriptorSet& write = descriptorWrites.ExpandAndGetRef();
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
          const ezGALResourceViewVulkan* pResourceView = m_pBoundShaderResourceViews[mapping.m_stage][mapping.m_uiSource];
          if (!pResourceView->GetDescription().m_hTexture.IsInvalidated())
          {
            write.pImageInfo = &pResourceView->GetImageInfo();
          }
          else
          {
            write.pBufferInfo = &pResourceView->GetBufferInfo();
          }
        }
        break;
        case ezGALShaderVulkan::BindingMapping::UAV:
        {
          const ezGALUnorderedAccessViewVulkan* pUAV = m_pBoundUnoderedAccessViews[mapping.m_uiSource];
          if (!pUAV->GetDescription().m_hTexture.IsInvalidated())
          {
            write.pImageInfo = &pUAV->GetImageInfo();
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

    ezDescriptorSetPoolVulkan::UpdateDescriptorSet(descriptorSet, descriptorWrites);
    m_pCommandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_PipelineDesc.m_layout, 0, 1, &descriptorSet, 0, nullptr);
  }
}
