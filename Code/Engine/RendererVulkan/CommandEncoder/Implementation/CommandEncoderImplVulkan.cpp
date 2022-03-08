#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/CommandEncoder/CommandEncoderImplVulkan.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/FenceVulkan.h>
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

// State setting functions

void ezGALCommandEncoderImplVulkan::SetShaderPlatform(const ezGALShader* pShader)
{
  if (pShader != nullptr)
  {
    m_pCurrentShader = static_cast<const ezGALShaderVulkan*>(pShader);
    m_bPipelineStateDirty = true;
  }
}

void ezGALCommandEncoderImplVulkan::SetConstantBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pBuffer)
{
  // \todo Check if the device supports the slot index?
  m_pBoundConstantBuffers[uiSlot] = pBuffer != nullptr ? static_cast<const ezGALBufferVulkan*>(pBuffer) : nullptr;

  // The GAL doesn't care about stages for constant buffer, but we need to handle this internaly.
  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    m_BoundConstantBuffersRange[stage].SetToIncludeValue(uiSlot);

  m_bDescriptorsDirty = true;
}

void ezGALCommandEncoderImplVulkan::SetSamplerStatePlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALSamplerState* pSamplerState)
{
  // \todo Check if the device supports the stage / the slot index
  m_pBoundSamplerStates[Stage][uiSlot] = pSamplerState != nullptr ? static_cast<const ezGALSamplerStateVulkan*>(pSamplerState) : nullptr;
  m_BoundSamplerStatesRange[Stage].SetToIncludeValue(uiSlot);

  m_bDescriptorsDirty = true;
}

void ezGALCommandEncoderImplVulkan::SetResourceViewPlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALResourceView* pResourceView)
{
  auto& boundShaderResourceViews = m_pBoundShaderResourceViews[Stage];
  boundShaderResourceViews.EnsureCount(uiSlot + 1);
  boundShaderResourceViews[uiSlot] = pResourceView != nullptr ? static_cast<const ezGALResourceViewVulkan*>(pResourceView) : nullptr;
  m_BoundShaderResourceViewsRange[Stage].SetToIncludeValue(uiSlot);

  m_bDescriptorsDirty = true;
}

void ezGALCommandEncoderImplVulkan::SetUnorderedAccessViewPlatform(ezUInt32 uiSlot, const ezGALUnorderedAccessView* pUnorderedAccessView)
{
  m_pBoundUnoderedAccessViews.EnsureCount(uiSlot + 1);
  m_pBoundUnoderedAccessViews[uiSlot] = pUnorderedAccessView != nullptr ? static_cast<const ezGALUnorderedAccessViewVulkan*>(pUnorderedAccessView) : nullptr;
  m_pBoundUnoderedAccessViewsRange.SetToIncludeValue(uiSlot);

  m_bDescriptorsDirty = true;
}

// Fence & Query functions

void ezGALCommandEncoderImplVulkan::InsertFencePlatform(const ezGALFence* pFence)
{
  auto pVulkanFence = static_cast<const ezGALFenceVulkan*>(pFence);

  vk::Queue queue = m_GALDeviceVulkan.GetQueue();

  m_pCommandBuffer->end();
  vk::SubmitInfo submitInfo = {};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = m_pCommandBuffer;

  queue.submit(1, &submitInfo, pVulkanFence->GetFence());
}

bool ezGALCommandEncoderImplVulkan::IsFenceReachedPlatform(const ezGALFence* pFence)
{
  auto pVulkanFence = static_cast<const ezGALFenceVulkan*>(pFence);
  vk::Result fenceStatus = m_vkDevice.getFenceStatus(pVulkanFence->GetFence());

  EZ_ASSERT_DEV(fenceStatus != vk::Result::eErrorDeviceLost, "Device lost during fence status query!");

  return fenceStatus == vk::Result::eSuccess;
}

void ezGALCommandEncoderImplVulkan::WaitForFencePlatform(const ezGALFence* pFence)
{
  /*while (!IsFenceReachedPlatform(pFence))
  {
    ezThreadUtils::YieldTimeSlice();
  }*/

  auto pVulkanFence = static_cast<const ezGALFenceVulkan*>(pFence)->GetFence();
  m_vkDevice.waitForFences(1, &pVulkanFence, true, 1000000000ui64);
}

void ezGALCommandEncoderImplVulkan::ResetFencePlatform(const ezGALFence* pFence)
{
  auto pVulkanFence = static_cast<const ezGALFenceVulkan*>(pFence)->GetFence();
  m_vkDevice.resetFences(1, &pVulkanFence);
}

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
}

void ezGALCommandEncoderImplVulkan::UpdateBufferPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> pSourceData,
  ezGALUpdateMode::Enum updateMode)
{
  EZ_CHECK_ALIGNMENT_16(pSourceData.GetPtr());

  auto pVulkanDestination = static_cast<const ezGALBufferVulkan*>(pDestination);

  if (pDestination->GetDescription().m_BufferType == ezGALBufferType::ConstantBuffer)
  {
    m_pCommandBuffer->updateBuffer(pVulkanDestination->GetVkBuffer(), uiDestOffset, pSourceData.GetCount(), pSourceData.GetPtr());
  }
  else
  {
    if (updateMode == ezGALUpdateMode::CopyToTempStorage)
    {
      if (ezGALBufferVulkan* tmpBuffer = m_GALDeviceVulkan.FindTempBuffer(pSourceData.GetCount()))
      {
        EZ_ASSERT_DEV(tmpBuffer->GetAllocationInfo().m_size >= pSourceData.GetCount(), "Source data is too big to copy staged!");

        ezVulkanAllocation alloc = tmpBuffer->GetAllocation();
        void* pData = nullptr;

        VK_ASSERT_DEV(ezMemoryAllocatorVulkan::MapMemory(alloc, &pData));
        EZ_ASSERT_DEV(pData, "Implementation error");

        ezMemoryUtils::Copy((ezUInt8*)pData, pSourceData.GetPtr(), pSourceData.GetCount());

        ezMemoryAllocatorVulkan::UnmapMemory(alloc);

        CopyBufferRegionPlatform(pDestination, uiDestOffset, tmpBuffer, 0, pSourceData.GetCount());
      }
      else
      {
        EZ_REPORT_FAILURE("Could not find a temp buffer for update.");
      }
    }
    else
    {
      // TODO need to check if the buffer is mappable

      // TODO is this behavior available on Vulkan?
      //D3D11_MAP mapType = (updateMode == ezGALUpdateMode::Discard) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;

      EZ_ASSERT_DEV(pVulkanDestination->GetAllocationInfo().m_size >= pSourceData.GetCount(), "Source data is too big to copy staged!");
      ezVulkanAllocation alloc = pVulkanDestination->GetAllocation();
      void* pData = nullptr;

      VK_ASSERT_DEV(ezMemoryAllocatorVulkan::MapMemory(alloc, &pData));
      EZ_ASSERT_DEV(pData, "Implementation error");

      ezMemoryUtils::Copy((ezUInt8*)pData, pSourceData.GetPtr(), pSourceData.GetCount());

      ezMemoryAllocatorVulkan::UnmapMemory(alloc);
    }
  }
}

void ezGALCommandEncoderImplVulkan::CopyTexturePlatform(const ezGALTexture* pDestination, const ezGALTexture* pSource)
{
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
  ezUInt32 uiWidth = ezMath::Max(DestinationBox.m_vMax.x - DestinationBox.m_vMin.x, 1u);
  ezUInt32 uiHeight = ezMath::Max(DestinationBox.m_vMax.y - DestinationBox.m_vMin.y, 1u);
  ezUInt32 uiDepth = ezMath::Max(DestinationBox.m_vMax.z - DestinationBox.m_vMin.z, 1u);
  ezGALResourceFormat::Enum format = pDestination->GetDescription().m_Format;

  if (ezGALTextureVulkan* pTempTexture = m_GALDeviceVulkan.FindTempTexture(uiWidth, uiHeight, uiDepth, format))
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

  ezResourceCacheVulkan::RenderPassDesc renderPassDesc;
  ezResourceCacheVulkan::GetRenderPassDesc(renderingSetup, renderPassDesc);
  vk::RenderPass renderPass = ezResourceCacheVulkan::RequestRenderPass(renderPassDesc);

  ezResourceCacheVulkan::FramebufferDesc framebufferDesc;
  ezResourceCacheVulkan::GetFrameBufferDesc(renderPass, renderingSetup, framebufferDesc);
  vk::Framebuffer frameBuffer = ezResourceCacheVulkan::RequestFrameBuffer(framebufferDesc);

  vk::RenderPassBeginInfo renderPassBeginInfo;
  {
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = frameBuffer;
    renderPassBeginInfo.renderArea.offset.setX(0).setY(0);
    renderPassBeginInfo.renderArea.extent.setHeight(framebufferDesc.height).setWidth(framebufferDesc.width);

    ezHybridArray<vk::ClearValue, EZ_GAL_MAX_RENDERTARGET_COUNT> clearValues;

    const bool bHasDepth = !renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget().IsInvalidated();
    const ezUInt32 uiColorCount = renderingSetup.m_RenderTargetSetup.HasRenderTargets() ? renderingSetup.m_RenderTargetSetup.GetMaxRenderTargetIndex() + 1 : 0;
    if (bHasDepth)
    {
      vk::ClearValue& depthClear = clearValues.ExpandAndGetRef();
      depthClear.depthStencil.setDepth(1.0f).setStencil(0);
    }
    for (ezUInt32 i = 0; i < uiColorCount; i++)
    {
      vk::ClearValue& colorClear = clearValues.ExpandAndGetRef();
      ezColor col = renderingSetup.m_ClearColor;
      colorClear.color.setFloat32({col.r, col.g, col.b, col.a});
    }

    renderPassBeginInfo.clearValueCount = clearValues.GetCount();
    renderPassBeginInfo.pClearValues = clearValues.GetData();
  }

  m_pCommandBuffer->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

  vk::Viewport viewport;
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(framebufferDesc.width);
  viewport.height = static_cast<float>(framebufferDesc.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vk::Rect2D scissor{{0, 0}, {framebufferDesc.width, framebufferDesc.height}};

  m_pCommandBuffer->setViewport(0, 1, &viewport);
  m_pCommandBuffer->setScissor(0, 1, &scissor);
}

void ezGALCommandEncoderImplVulkan::EndRendering()
{
  m_pCommandBuffer->endRenderPass();
  m_pCommandBuffer = nullptr;
}

void ezGALCommandEncoderImplVulkan::ClearPlatform(const ezColor& ClearColor, ezUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, ezUInt8 uiStencilClear)
{
  // TODO:
  EZ_ASSERT_NOT_IMPLEMENTED;
}

// Draw functions

void ezGALCommandEncoderImplVulkan::DrawPlatform(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex)
{
  //FlushDeferredStateChanges();

  m_pCommandBuffer->draw(uiVertexCount, 1, uiStartVertex, 0);
}

void ezGALCommandEncoderImplVulkan::DrawIndexedPlatform(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex)
{
  //FlushDeferredStateChanges();

#if 0 //EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  m_pCommandBuffer->drawIndexed(uiIndexCount, 1, uiStartIndex, 0, 0);

  // In debug builds, with a debugger attached, the engine will break on D3D errors
  // this can be very annoying when an error happens repeatedly
  // you can disable it at runtime, by using the debugger to set bChangeBreakPolicy to 'true', or dragging the
  // the instruction pointer into the if
  volatile bool bChangeBreakPolicy = false;
  if (bChangeBreakPolicy)
  {
    ezGALDeviceVulkan* pDevice = static_cast<ezGALDeviceVulkan*>(GetDevice());
    if (pDevice->m_pDebug)
    {
      ID3D11InfoQueue* pInfoQueue = nullptr;
      if (SUCCEEDED(pDevice->m_pDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&pInfoQueue)))
      {
        // modify these, if you want to keep certain things enabled
        static BOOL bBreakOnCorruption = FALSE;
        static BOOL bBreakOnError = FALSE;
        static BOOL bBreakOnWarning = FALSE;

        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, bBreakOnCorruption);
        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, bBreakOnError);
        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, bBreakOnWarning);
      }
    }
  }
#else
  m_pCommandBuffer->drawIndexed(uiIndexCount, 1, uiStartIndex, 0, 0);
#endif
}

void ezGALCommandEncoderImplVulkan::DrawIndexedInstancedPlatform(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex)
{
  //FlushDeferredStateChanges();

  m_pCommandBuffer->drawIndexed(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex, 0, 0);
}

void ezGALCommandEncoderImplVulkan::DrawIndexedInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  //FlushDeferredStateChanges();

  m_pCommandBuffer->drawIndexedIndirect(static_cast<const ezGALBufferVulkan*>(pIndirectArgumentBuffer)->GetVkBuffer(), uiArgumentOffsetInBytes, 1, 0);
}

void ezGALCommandEncoderImplVulkan::DrawInstancedPlatform(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex)
{
  //FlushDeferredStateChanges();

  m_pCommandBuffer->draw(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex, 0);
}

void ezGALCommandEncoderImplVulkan::DrawInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  //FlushDeferredStateChanges();

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
  // TODO index buffer needs to be in index buffer resource state

  if (pIndexBuffer != nullptr)
  {
    const ezGALBufferVulkan* pVulkanBuffer = static_cast<const ezGALBufferVulkan*>(pIndexBuffer);
    m_pCommandBuffer->bindIndexBuffer(pVulkanBuffer->GetVkBuffer(), 0, pVulkanBuffer->GetIndexType());
  }
  else
  {
    // TODO binding a null buffer is not allowed in vulkan
    EZ_ASSERT_NOT_IMPLEMENTED;
    //m_pCommandBuffer->bindIndexBuffer({}, 0, vk::IndexType::eUint16);
  }
}

void ezGALCommandEncoderImplVulkan::SetVertexBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pVertexBuffer)
{
  EZ_ASSERT_DEV(uiSlot < EZ_GAL_MAX_VERTEX_BUFFER_COUNT, "Invalid slot index");

  m_pBoundVertexBuffers[uiSlot] = pVertexBuffer != nullptr ? static_cast<const ezGALBufferVulkan*>(pVertexBuffer)->GetVkBuffer() : nullptr;
  m_VertexBufferStrides[uiSlot] = pVertexBuffer != nullptr ? pVertexBuffer->GetDescription().m_uiStructSize : 0;
  m_BoundVertexBuffersRange.SetToIncludeValue(uiSlot);
}

void ezGALCommandEncoderImplVulkan::SetVertexDeclarationPlatform(const ezGALVertexDeclaration* pVertexDeclaration)
{
  if (pVertexDeclaration)
  {
    m_pCurrentVertexLayout = &static_cast<const ezGALVertexDeclarationVulkan*>(pVertexDeclaration)->GetInputLayout();
    m_bPipelineStateDirty = true;
  }
  // TODO defer to pipeline update
  //m_pDXContext->IASetInputLayout(
  //  pVertexDeclaration != nullptr ? static_cast<const ezGALVertexDeclarationVulkan*>(pVertexDeclaration)->GetDXInputLayout() : nullptr);
}


static const vk::PrimitiveTopology GALTopologyToVulkan[ezGALPrimitiveTopology::ENUM_COUNT] = {
  vk::PrimitiveTopology::ePointList,
  vk::PrimitiveTopology::eLineList,
  vk::PrimitiveTopology::eTriangleList,
};

void ezGALCommandEncoderImplVulkan::SetPrimitiveTopologyPlatform(ezGALPrimitiveTopology::Enum Topology)
{
  m_currentPrimitiveTopology = GALTopologyToVulkan[Topology];
  m_bPipelineStateDirty = true;
}

void ezGALCommandEncoderImplVulkan::SetBlendStatePlatform(const ezGALBlendState* pBlendState, const ezColor& BlendFactor, ezUInt32 uiSampleMask)
{
  m_pCurrentBlendState = pBlendState != nullptr ? static_cast<const ezGALBlendStateVulkan*>(pBlendState) : nullptr;
  m_bPipelineStateDirty = true;
}

void ezGALCommandEncoderImplVulkan::SetDepthStencilStatePlatform(const ezGALDepthStencilState* pDepthStencilState, ezUInt8 uiStencilRefValue)
{
  m_pCurrentDepthStencilState = pDepthStencilState != nullptr ? static_cast<const ezGALDepthStencilStateVulkan*>(pDepthStencilState) : nullptr;
  m_bPipelineStateDirty = true;
}

void ezGALCommandEncoderImplVulkan::SetRasterizerStatePlatform(const ezGALRasterizerState* pRasterizerState)
{
  m_pCurrentRasterizerState = pRasterizerState != nullptr ? static_cast<const ezGALRasterizerStateVulkan*>(pRasterizerState) : nullptr;
  m_bPipelineStateDirty = true;
}

void ezGALCommandEncoderImplVulkan::SetViewportPlatform(const ezRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  vk::Viewport viewport = {};
  viewport.x = rect.x;
  viewport.y = rect.y;
  viewport.width = rect.width;
  viewport.height = rect.height;
  viewport.minDepth = fMinDepth;
  viewport.maxDepth = fMaxDepth;

  m_pCommandBuffer->setViewport(0, 1, &viewport);
}

void ezGALCommandEncoderImplVulkan::SetScissorRectPlatform(const ezRectU32& rect)
{
  vk::Rect2D scissor;
  scissor.offset.x = rect.x;
  scissor.offset.y = rect.y;
  scissor.extent.width = rect.width;
  scissor.extent.height = rect.height;

  m_pCommandBuffer->setScissor(0, 1, &scissor);
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
#if 0
  if (m_BoundVertexBuffersRange.IsValid())
  {
    // TODO vertex buffer needs to be in index buffer resource state
    const ezUInt32 uiStartSlot = m_BoundVertexBuffersRange.m_uiMin;
    const ezUInt32 uiNumSlots = m_BoundVertexBuffersRange.GetCount();

    m_pDXContext->IASetVertexBuffers(uiStartSlot, uiNumSlots, m_pBoundVertexBuffers + uiStartSlot, m_VertexBufferStrides + uiStartSlot,
      m_VertexBufferOffsets + uiStartSlot);

    m_BoundVertexBuffersRange.Reset();
  }

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (m_pBoundShaders[stage] != nullptr && m_BoundConstantBuffersRange[stage].IsValid())
    {
      const ezUInt32 uiStartSlot = m_BoundConstantBuffersRange[stage].m_uiMin;
      const ezUInt32 uiNumSlots = m_BoundConstantBuffersRange[stage].GetCount();

      SetConstantBuffers((ezGALShaderStage::Enum)stage, m_pDXContext, uiStartSlot, uiNumSlots, m_pBoundConstantBuffers + uiStartSlot);

      m_BoundConstantBuffersRange[stage].Reset();
    }
  }

  // Do UAV bindings before SRV since UAV are outputs which need to be unbound before they are potentially rebound as SRV again.
  if (m_pBoundUnoderedAccessViewsRange.IsValid())
  {
    const ezUInt32 uiStartSlot = m_pBoundUnoderedAccessViewsRange.m_uiMin;
    const ezUInt32 uiNumSlots = m_pBoundUnoderedAccessViewsRange.GetCount();
    m_pDXContext->CSSetUnorderedAccessViews(uiStartSlot, uiNumSlots, m_pBoundUnoderedAccessViews.GetData() + uiStartSlot,
      nullptr); // Todo: Count reset.

    m_pBoundUnoderedAccessViewsRange.Reset();
  }

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    // Need to do bindings even on inactive shader stages since we might miss unbindings otherwise!
    if (m_BoundShaderResourceViewsRange[stage].IsValid())
    {
      const ezUInt32 uiStartSlot = m_BoundShaderResourceViewsRange[stage].m_uiMin;
      const ezUInt32 uiNumSlots = m_BoundShaderResourceViewsRange[stage].GetCount();

      SetShaderResources((ezGALShaderStage::Enum)stage, m_pDXContext, uiStartSlot, uiNumSlots,
        m_pBoundShaderResourceViews[stage].GetData() + uiStartSlot);

      m_BoundShaderResourceViewsRange[stage].Reset();
    }

    // Don't need to unset sampler stages for unbound shader stages.
    if (m_pBoundShaders[stage] == nullptr)
      continue;

    if (m_BoundSamplerStatesRange[stage].IsValid())
    {
      const ezUInt32 uiStartSlot = m_BoundSamplerStatesRange[stage].m_uiMin;
      const ezUInt32 uiNumSlots = m_BoundSamplerStatesRange[stage].GetCount();

      SetSamplers((ezGALShaderStage::Enum)stage, m_pDXContext, uiStartSlot, uiNumSlots, m_pBoundSamplerStates[stage] + uiStartSlot);

      m_BoundSamplerStatesRange[stage].Reset();
    }
  }
#endif
}
