
#include <RendererVulkan/CommandEncoder/CommandEncoderVulkan.h>
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

template <typename Base>
ezGALCommandEncoderVulkan<Base>::ezGALCommandEncoderVulkan(ezGALDevice& device)
  : Base(device)
{
  m_vkDevice = static_cast<ezGALDeviceVulkan&>(device).GetVulkanDevice();
}

template <typename Base>
ezGALCommandEncoderVulkan<Base>::~ezGALCommandEncoderVulkan() = default;

// State setting functions

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::SetShaderPlatform(const ezGALShader* pShader)
{
  if (pShader != nullptr)
  {
    m_pCurrentShader = static_cast<const ezGALShaderVulkan*>(pShader);
    m_bPipelineStateDirty = true;
  }
}

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::SetConstantBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pBuffer)
{
  // \todo Check if the device supports the slot index?
  m_pBoundConstantBuffers[uiSlot] = pBuffer != nullptr ? static_cast<const ezGALBufferVulkan*>(pBuffer) : nullptr;

  // The GAL doesn't care about stages for constant buffer, but we need to handle this internaly.
  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    m_BoundConstantBuffersRange[stage].SetToIncludeValue(uiSlot);

  m_bDescriptorsDirty = true;
}

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::SetSamplerStatePlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALSamplerState* pSamplerState)
{
  // \todo Check if the device supports the stage / the slot index
  m_pBoundSamplerStates[Stage][uiSlot] = pSamplerState != nullptr ? static_cast<const ezGALSamplerStateVulkan*>(pSamplerState) : nullptr;
  m_BoundSamplerStatesRange[Stage].SetToIncludeValue(uiSlot);

  m_bDescriptorsDirty = true;
}

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::SetResourceViewPlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALResourceView* pResourceView)
{
  auto& boundShaderResourceViews = m_pBoundShaderResourceViews[Stage];
  boundShaderResourceViews.EnsureCount(uiSlot + 1);
  boundShaderResourceViews[uiSlot] = pResourceView != nullptr ? static_cast<const ezGALResourceViewVulkan*>(pResourceView) : nullptr;
  m_BoundShaderResourceViewsRange[Stage].SetToIncludeValue(uiSlot);

  m_bDescriptorsDirty = true;
}

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::SetUnorderedAccessViewPlatform(ezUInt32 uiSlot, const ezGALUnorderedAccessView* pUnorderedAccessView)
{
  m_pBoundUnoderedAccessViews.EnsureCount(uiSlot + 1);
  m_pBoundUnoderedAccessViews[uiSlot] = pUnorderedAccessView != nullptr ? static_cast<const ezGALUnorderedAccessViewVulkan*>(pUnorderedAccessView) : nullptr;
  m_pBoundUnoderedAccessViewsRange.SetToIncludeValue(uiSlot);

  m_bDescriptorsDirty = true;
}

// Fence & Query functions

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::InsertFencePlatform(const ezGALFence* pFence)
{
  auto& vulkanDevice = static_cast<ezGALDeviceVulkan&>(this->GetDevice());
  auto pVulkanFence = static_cast<const ezGALFenceVulkan*>(pFence);

  vk::Queue queue = vulkanDevice.GetQueue();

  m_pCommandBuffer->end();
  vk::SubmitInfo submitInfo = {};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = m_pCommandBuffer;

  queue.submit(1, &submitInfo, pVulkanFence->GetFence());
}

template <typename Base>
bool ezGALCommandEncoderVulkan<Base>::IsFenceReachedPlatform(const ezGALFence* pFence)
{
  auto pVulkanFence = static_cast<const ezGALFenceVulkan*>(pFence);
  vk::Result fenceStatus = m_vkDevice.getFenceStatus(pVulkanFence->GetFence());

  EZ_ASSERT_DEV(fenceStatus != vk::Result::eErrorDeviceLost, "Device lost during fence status query!");

  return fenceStatus == vk::Result::eSuccess;
}

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::WaitForFencePlatform(const ezGALFence* pFence)
{
  /*while (!IsFenceReachedPlatform(pFence))
  {
    ezThreadUtils::YieldTimeSlice();
  }*/

   auto pVulkanFence = static_cast<const ezGALFenceVulkan*>(pFence);
  m_vkDevice.waitForFences(1, &pVulkanFence->GetFence(), true, 1000000000ui64);
}

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::BeginQueryPlatform(const ezGALQuery* pQuery)
{
  auto pVulkanQuery = static_cast<const ezGALQueryVulkan*>(pQuery);

  // TODO how to decide the query type etc in Vulkan?

  m_pCommandBuffer->beginQuery(pVulkanQuery->GetPool(), pVulkanQuery->GetID(), {});
}

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::EndQueryPlatform(const ezGALQuery* pQuery)
{
  auto pVulkanQuery = static_cast<const ezGALQueryVulkan*>(pQuery);

  m_pCommandBuffer->endQuery(pVulkanQuery->GetPool(), pVulkanQuery->GetID());
}

template <typename Base>
ezResult ezGALCommandEncoderVulkan<Base>::GetQueryResultPlatform(const ezGALQuery* pQuery, ezUInt64& uiQueryResult)
{
  auto pVulkanQuery = static_cast<const ezGALQueryVulkan*>(pQuery);
  vk::Result result = m_vkDevice.getQueryPoolResults(pVulkanQuery->GetPool(), pVulkanQuery->GetID(), 1u, sizeof(ezUInt64), &uiQueryResult, 0, vk::QueryResultFlagBits::e64);

  return result == vk::Result::eSuccess ? EZ_SUCCESS : EZ_FAILURE;
}

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::InsertTimestampPlatform(ezGALTimestampHandle hTimestamp)
{
  // TODO how to implement this in Vulkan?
  //ID3D11Query* pDXQuery = static_cast<ezGALDeviceVulkan*>(GetDevice())->GetTimestamp(hTimestamp);
  //
  //m_pDXContext->End(pDXQuery);

  EZ_ASSERT_NOT_IMPLEMENTED;
}

// Resource update functions

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::ClearUnorderedAccessViewPlatform(const ezGALUnorderedAccessView* pUnorderedAccessView, ezVec4 clearValues)
{
  // this looks to require custom code, either using buffer copies or
  // clearing via a compute shader

  EZ_ASSERT_NOT_IMPLEMENTED;
}

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::ClearUnorderedAccessViewPlatform(const ezGALUnorderedAccessView* pUnorderedAccessView, ezVec4U32 clearValues)
{
  // Same as the other clearing variant

  EZ_ASSERT_NOT_IMPLEMENTED;
}

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::CopyBufferPlatform(const ezGALBuffer* pDestination, const ezGALBuffer* pSource)
{
  vk::Buffer destination = static_cast<const ezGALBufferVulkan*>(pDestination)->GetVkBuffer();
  vk::Buffer source = static_cast<const ezGALBufferVulkan*>(pSource)->GetVkBuffer();

  EZ_ASSERT_DEV(pSource->GetSize() != pDestination->GetSize(), "Source and destination buffer sizes mismatch!");

  // TODO do this in an immediate command buffer?
  vk::BufferCopy bufferCopy = {};
  bufferCopy.size = pSource->GetSize();

  m_pCommandBuffer->copyBuffer(source, destination, 1, &bufferCopy);
}

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::CopyBufferRegionPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, const ezGALBuffer* pSource,
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

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::UpdateBufferPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> pSourceData,
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
    auto& vulkanDevice = static_cast<ezGALDeviceVulkan&>(this->GetDevice());

    if (updateMode == ezGALUpdateMode::CopyToTempStorage)
    {
      if (ezGALBufferVulkan* tmpBuffer = vulkanDevice.FindTempBuffer(pSourceData.GetCount()))
      {
        EZ_ASSERT_DEV(tmpBuffer->GetSize() >= pSourceData.GetCount(), "Source data is too big to copy staged!");

        void* pData = m_vkDevice.mapMemory(tmpBuffer->GetMemory(), tmpBuffer->GetMemoryOffset(), tmpBuffer->GetSize());

        EZ_ASSERT_DEV(pData, "Implementation error");

        ezMemoryUtils::Copy((ezUInt8*)pData, pSourceData.GetPtr(), pSourceData.GetCount());

        m_vkDevice.unmapMemory(tmpBuffer->GetMemory());

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

      void* pData = m_vkDevice.mapMemory(pVulkanDestination->GetMemory(), pVulkanDestination->GetMemoryOffset(), pVulkanDestination->GetSize());

      if (pData)
      {
        ezMemoryUtils::Copy((ezUInt8*)pData, pSourceData.GetPtr(), pSourceData.GetCount());

        m_vkDevice.unmapMemory(pVulkanDestination->GetMemory());
      }
    }
  }
}

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::CopyTexturePlatform(const ezGALTexture* pDestination, const ezGALTexture* pSource)
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

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::CopyTextureRegionPlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
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

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::UpdateTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
  const ezBoundingBoxu32& DestinationBox, const ezGALSystemMemoryDescription& pSourceData)
{
  ezUInt32 uiWidth = ezMath::Max(DestinationBox.m_vMax.x - DestinationBox.m_vMin.x, 1u);
  ezUInt32 uiHeight = ezMath::Max(DestinationBox.m_vMax.y - DestinationBox.m_vMin.y, 1u);
  ezUInt32 uiDepth = ezMath::Max(DestinationBox.m_vMax.z - DestinationBox.m_vMin.z, 1u);
  ezGALResourceFormat::Enum format = pDestination->GetDescription().m_Format;

  auto& vulkanDevice = static_cast<ezGALDeviceVulkan&>(this->GetDevice());

  if (ezGALTextureVulkan* pTempTexture = vulkanDevice.FindTempTexture(uiWidth, uiHeight, uiDepth, format))
  {
    void* pData = m_vkDevice.mapMemory(pTempTexture->GetMemory(), pTempTexture->GetMemoryOffset(), pTempTexture->GetMemorySize());
    EZ_ASSERT_DEV(pData, "Implementation error");

    ezUInt32 uiRowPitch = uiWidth * ezGALResourceFormat::GetBitsPerElement(format) / 8;
    ezUInt32 uiSlicePitch = uiRowPitch * uiHeight;
    EZ_ASSERT_DEV(pSourceData.m_uiRowPitch == uiRowPitch, "Invalid row pitch. Expected {0} got {1}", uiRowPitch, pSourceData.m_uiRowPitch);
    EZ_ASSERT_DEV(pSourceData.m_uiSlicePitch == 0 || pSourceData.m_uiSlicePitch == uiSlicePitch,
      "Invalid slice pitch. Expected {0} got {1}", uiSlicePitch, pSourceData.m_uiSlicePitch);

    ezMemoryUtils::RawByteCopy(pData, pSourceData.m_pData, uiSlicePitch * uiDepth);

    m_vkDevice.unmapMemory(pTempTexture->GetMemory());

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

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::ResolveTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
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

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::ReadbackTexturePlatform(const ezGALTexture* pTexture)
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

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::CopyTextureReadbackResultPlatform(const ezGALTexture* pTexture,
  const ezArrayPtr<ezGALSystemMemoryDescription>* pData)
{
  auto pVulkanTexture = static_cast<const ezGALTextureVulkan*>(pTexture);

  const ezGALBufferVulkan* pStagingBuffer = pVulkanTexture->GetStagingBuffer();

  EZ_ASSERT_DEV(pStagingBuffer, "No staging resource available for read-back");

  // Data should be tightly packed in the staging buffer already, so
  // just map the memory and copy it over
  void* pSrcData = m_vkDevice.mapMemory(pStagingBuffer->GetMemory(), pStagingBuffer->GetMemoryOffset(), pStagingBuffer->GetSize());
  if (pSrcData)
  {
    // TODO size of the buffer could missmatch the texture data size necessary
    ezMemoryUtils::RawByteCopy(pData->GetPtr()->m_pData, pSrcData, pStagingBuffer->GetSize());

    m_vkDevice.unmapMemory(pStagingBuffer->GetMemory());
  }
}

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::GenerateMipMapsPlatform(const ezGALResourceView* pResourceView)
{
  const ezGALResourceViewVulkan* pDXResourceView = static_cast<const ezGALResourceViewVulkan*>(pResourceView);

  // TODO texture blit based approach
}

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::FlushPlatform()
{
  FlushDeferredStateChanges();
}

// Debug helper functions

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::PushMarkerPlatform(const char* szMarker)
{
  // TODO early out if device doesn't support debug markers

  constexpr float markerColor[4] = {1, 1, 1, 1};
  vk::DebugMarkerMarkerInfoEXT markerInfo = {};
  ezMemoryUtils::Copy(markerInfo.color.data(), markerColor, EZ_ARRAY_SIZE(markerColor));
  markerInfo.pMarkerName = szMarker;
  m_pCommandBuffer->debugMarkerBeginEXT(markerInfo);
}

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::PopMarkerPlatform()
{
  m_pCommandBuffer->debugMarkerEndEXT();
}

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::InsertEventMarkerPlatform(const char* szMarker)
{
  constexpr float markerColor[4] = {1, 1, 1, 1};
  vk::DebugMarkerMarkerInfoEXT markerInfo = {};
  ezMemoryUtils::Copy(markerInfo.color.data(), markerColor, EZ_ARRAY_SIZE(markerColor));
  markerInfo.pMarkerName = szMarker;
  m_pCommandBuffer->debugMarkerInsertEXT(markerInfo);
}

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

template <typename Base>
void ezGALCommandEncoderVulkan<Base>::FlushDeferredStateChanges()
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
