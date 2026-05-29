#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/Pools/CommandBufferPoolVulkan.h>
#include <RendererVulkan/Pools/StagingBufferPoolVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/BarrierUtilsVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

ezInitContextVulkan::ezInitContextVulkan(ezGALDeviceVulkan* pDevice)
  : m_pDevice(pDevice)
{
  ezAllocator* pAllocator = m_pDevice->GetAllocator();
  m_pCommandBufferPool = EZ_NEW(pAllocator, ezCommandBufferPoolVulkan, pAllocator);
  m_pCommandBufferPool->Initialize(m_pDevice->GetVulkanDevice(), m_pDevice->GetGraphicsQueue().m_uiQueueFamily);
  m_pStagingBufferPool = EZ_NEW(pAllocator, ezStagingBufferPoolVulkan);
  m_pStagingBufferPool->Initialize(m_pDevice, 50 * 1024 * 1024);
}

ezInitContextVulkan::~ezInitContextVulkan()
{
  EZ_ASSERT_DEBUG(!m_currentCommandBuffer, "GetFinishedCommandBuffer should have been called before destruction.");

  m_pCommandBufferPool->DeInitialize();
  m_pStagingBufferPool->DeInitialize();
  m_pCommandBufferPool.Clear();
  m_pStagingBufferPool.Clear();
}


void ezInitContextVulkan::AfterBeginFrame()
{
  EZ_LOCK(m_Lock);
  m_pStagingBufferPool->AfterBeginFrame();
}

vk::CommandBuffer ezInitContextVulkan::GetFinishedCommandBuffer()
{
  EZ_LOCK(m_Lock);
  if (m_currentCommandBuffer)
  {
    m_pStagingBufferPool->BeforeCommandBufferSubmit();
    if (m_pDevice->GetExtensions().m_bDebugUtilsMarkers)
    {
      m_currentCommandBuffer.endDebugUtilsLabelEXT(m_pDevice->GetDispatchContext());
    }
    vk::CommandBuffer res = m_currentCommandBuffer;
    res.end();

    m_pDevice->ReclaimLater(m_currentCommandBuffer, m_pCommandBufferPool.Borrow());
    return res;
  }
  return nullptr;
}

void ezInitContextVulkan::EnsureCommandBufferExists()
{
  if (!m_currentCommandBuffer)
  {
    // Restart new command buffer if none is active already.
    m_currentCommandBuffer = m_pCommandBufferPool->RequestCommandBuffer();
    vk::CommandBufferBeginInfo beginInfo;
    m_currentCommandBuffer.begin(&beginInfo);
    if (m_pDevice->GetExtensions().m_bDebugUtilsMarkers)
    {
      constexpr float markerColor[4] = {0, 0, 0, 0};
      vk::DebugUtilsLabelEXT markerInfo = {};
      ezMemoryUtils::Copy(markerInfo.color.data(), markerColor, EZ_ARRAY_SIZE(markerColor));
      markerInfo.pLabelName = "InitContext";

      m_currentCommandBuffer.beginDebugUtilsLabelEXT(markerInfo, m_pDevice->GetDispatchContext());
    }
  }
}


void ezInitContextVulkan::InitTexture(const ezGALTextureVulkan* pTexture, vk::ImageCreateInfo& createInfo, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData)
{
  EZ_LOCK(m_Lock);

  EnsureCommandBufferExists();

  const ezBitflags<ezGALResourceState> defaultState = pTexture->GetDescription().GetDefaultState();

  ezBarrierUtilsVulkan barriers(*m_pDevice, m_currentCommandBuffer);

  if (pTexture->GetDescription().m_pExisitingNativeObject == nullptr)
  {
    if (pTexture->GetDescription().m_SampleCount != ezGALMSAASampleCount::None)
    {
      // #TODO_VULKAN how do we clear a MS target to zero?
      // Transition MSAA target from undefined to its default state.
      barriers.TextureBarrier(pTexture->GetImage(), pTexture->GetFullRange(),
        ezGALResourceState::Unknown, defaultState);
      return;
    }

    ezDynamicArray<ezGALSystemMemoryDescription> initialData;
    if (pInitialData.IsEmpty())
    {
      // If we don't have any initial data to initialize the texture to zero memory to match DX11 behavior.
      const vk::Format format = pTexture->GetImageFormat();
      const ezUInt8 uiBlockSize = vk::blockSize(format);
      const auto blockExtent = vk::blockExtent(format);
      {
        // Compute max size of the temp buffer.
        const vk::Extent3D imageExtent = pTexture->GetMipLevelSize(0);
        const VkExtent3D blockCount = {
          (imageExtent.width + blockExtent[0] - 1) / blockExtent[0],
          (imageExtent.height + blockExtent[1] - 1) / blockExtent[1],
          (imageExtent.depth + blockExtent[2] - 1) / blockExtent[2]};
        const ezUInt32 uiTotalSize = uiBlockSize * blockCount.width * blockCount.height * blockCount.depth;
        if (m_TempData.GetCount() < uiTotalSize)
          m_TempData.SetCount(uiTotalSize, 0);
      }

      for (ezUInt32 uiLayer = 0; uiLayer < createInfo.arrayLayers; uiLayer++)
      {
        for (ezUInt32 uiMipLevel = 0; uiMipLevel < createInfo.mipLevels; uiMipLevel++)
        {
          const ezUInt32 uiSubresourceIndex = uiMipLevel + uiLayer * createInfo.mipLevels;
          EZ_ASSERT_DEBUG(initialData.GetCount() == uiSubresourceIndex, "");

          const vk::Extent3D imageExtent = pTexture->GetMipLevelSize(uiMipLevel);
          const VkExtent3D blockCount = {
            (imageExtent.width + blockExtent[0] - 1) / blockExtent[0],
            (imageExtent.height + blockExtent[1] - 1) / blockExtent[1],
            (imageExtent.depth + blockExtent[2] - 1) / blockExtent[2]};

          ezGALSystemMemoryDescription data;
          data.m_pData = m_TempData.GetByteArrayPtr();
          data.m_uiRowPitch = uiBlockSize * blockCount.width;
          data.m_uiSlicePitch = data.m_uiRowPitch * blockCount.height;
          initialData.PushBack(data);
        }
      }
      pInitialData = initialData.GetArrayPtr();
    }

    // Transition entire image from undefined to transfer destination.
    barriers.TextureBarrier(pTexture->GetImage(), pTexture->GetFullRange(),
      ezGALResourceState::Unknown, ezGALResourceState::CopyDestination);

    for (ezUInt32 uiLayer = 0; uiLayer < createInfo.arrayLayers; uiLayer++)
    {
      for (ezUInt32 uiMipLevel = 0; uiMipLevel < createInfo.mipLevels; uiMipLevel++)
      {
        const ezUInt32 uiSubresourceIndex = uiMipLevel + uiLayer * createInfo.mipLevels;
        EZ_ASSERT_DEBUG(uiSubresourceIndex < pInitialData.GetCount(), "Not all data provided in the intial texture data.");
        const ezGALSystemMemoryDescription& subResourceData = pInitialData[uiSubresourceIndex];

        vk::ImageSubresourceLayers subresourceLayers;
        // We do not support stencil uploads right now.
        subresourceLayers.aspectMask = ezConversionUtilsVulkan::IsDepthFormat(pTexture->GetImageFormat()) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
        subresourceLayers.mipLevel = uiMipLevel;
        subresourceLayers.baseArrayLayer = uiLayer;
        subresourceLayers.layerCount = 1;

        const vk::Offset3D imageOffset = {0, 0, 0};
        const vk::Extent3D imageExtent = pTexture->GetMipLevelSize(uiMipLevel);

        // Image is already in CopyDestination, so skip before/after image transitions inside UploadTextureStaging.
        m_pDevice->UploadTextureStaging(*m_pDevice, m_pStagingBufferPool.Borrow(), m_currentCommandBuffer, pTexture, subresourceLayers, imageOffset, imageExtent, subResourceData);
      }
    }

    // Transition entire image from transfer destination to default state.
    barriers.TextureBarrier(pTexture->GetImage(), pTexture->GetFullRange(),
      ezGALResourceState::CopyDestination, defaultState);
  }
  else
  {
    // We don't actually know what the current state is of an existing native object. The only use case right now are back buffers created by swap chains so for now we just throw away the current content and barrier into the preferred layout.
    barriers.TextureBarrier(pTexture->GetImage(), pTexture->GetFullRange(),
      ezGALResourceState::Unknown, defaultState);
  }
}

void ezInitContextVulkan::InitBuffer(const ezGALBufferVulkan* pBuffer, ezConstByteArrayPtr pInitialData)
{
  EZ_LOCK(m_Lock);

  EnsureCommandBufferExists();

  ezBarrierUtilsVulkan barriers(*m_pDevice, m_currentCommandBuffer);
  const ezBitflags<ezGALResourceState> defaultState = pBuffer->GetDescription().GetDefaultState();

  // During initialization, there can't be any read/write hazard with the GPU so we can write to the memory directly if supported, e.g. unified memory.
  const ezVulkanAllocationInfo& allocInfo = pBuffer->GetAllocationInfo();
  if (allocInfo.m_pMappedData != nullptr)
  {
    ezMemoryUtils::Copy((ezUInt8*)allocInfo.m_pMappedData, pInitialData.GetPtr(), pInitialData.GetCount());

    barriers.BufferBarrier(pBuffer->GetVkBuffer(),
      ezGALResourceState::CpuWrite, defaultState);
  }
  else
  {
    barriers.BufferBarrier(pBuffer->GetVkBuffer(), ezGALResourceState::Unknown, ezGALResourceState::CopyDestination);

    m_pDevice->UploadBufferStaging(*m_pDevice, m_pStagingBufferPool.Borrow(), m_currentCommandBuffer, pBuffer, pInitialData, 0);

    barriers.BufferBarrier(pBuffer->GetVkBuffer(), ezGALResourceState::CopyDestination, defaultState);
  }
}

void ezInitContextVulkan::UpdateTexture(const ezGALTextureVulkan* pTexture, const ezGALTextureSubresource& subresource, const ezBoundingBoxu32& box, const ezGALSystemMemoryDescription& sourceData)
{
  EZ_LOCK(m_Lock);

  EnsureCommandBufferExists();

  const ezBitflags<ezGALResourceState> defaultState = pTexture->GetDescription().GetDefaultState();

  const ezVec3U32 boxExtents = box.GetExtents();
  const vk::Offset3D imageOffset = {(ezInt32)box.m_vMin.x, (ezInt32)box.m_vMin.y, (ezInt32)box.m_vMin.z};
  const vk::Extent3D imageExtent = {boxExtents.x, boxExtents.y, boxExtents.z};

  vk::ImageSubresourceLayers subresourceLayers;
  subresourceLayers.aspectMask = ezConversionUtilsVulkan::IsDepthFormat(pTexture->GetImageFormat()) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  subresourceLayers.mipLevel = subresource.m_uiMipLevel;
  subresourceLayers.baseArrayLayer = subresource.m_uiArraySlice;
  subresourceLayers.layerCount = 1;

  vk::ImageSubresourceRange subresourceRange = ezConversionUtilsVulkan::GetSubresourceRange(subresourceLayers);

  ezBarrierUtilsVulkan barriers(*m_pDevice, m_currentCommandBuffer);
  barriers.TextureBarrier(pTexture->GetImage(), subresourceRange, defaultState, ezGALResourceState::CopyDestination);

  m_pDevice->UploadTextureStaging(*m_pDevice, m_pStagingBufferPool.Borrow(), m_currentCommandBuffer, pTexture, subresourceLayers, imageOffset, imageExtent, sourceData);

  barriers.TextureBarrier(pTexture->GetImage(), subresourceRange, ezGALResourceState::CopyDestination, defaultState);
}

void ezInitContextVulkan::UpdateBuffer(const ezGALBufferVulkan* pBuffer, ezUInt32 uiOffset, ezConstByteArrayPtr pSourceData)
{
  EZ_LOCK(m_Lock);

  EnsureCommandBufferExists();
  const ezBitflags<ezGALResourceState> defaultState = pBuffer->GetDescription().GetDefaultState();
  ezBarrierUtilsVulkan barriers(*m_pDevice, m_currentCommandBuffer);
  barriers.BufferBarrier(pBuffer->GetVkBuffer(), defaultState, ezGALResourceState::CopyDestination);

  m_pDevice->UploadBufferStaging(*m_pDevice, m_pStagingBufferPool.Borrow(), m_currentCommandBuffer, pBuffer, pSourceData, uiOffset);

  barriers.BufferBarrier(pBuffer->GetVkBuffer(), ezGALResourceState::CopyDestination, defaultState);
}

void ezInitContextVulkan::UpdateDynamicUniformBuffer(vk::Buffer gpuBuffer, vk::Buffer stagingBuffer, ezUInt32 uiOffset, ezUInt32 uiSize)
{
  EZ_LOCK(m_Lock);

  EnsureCommandBufferExists();

  ezBarrierUtilsVulkan barriers(*m_pDevice, m_currentCommandBuffer);

  if (stagingBuffer)
  {
    // gpuBuffer can't be accessed on the CPU, so the data is present in stagingBuffer and needs to be copied over.
    barriers.BufferBarrier(stagingBuffer,
      ezGALResourceState::CpuWrite, ezGALResourceState::CopySource);

    vk::BufferCopy bufferCopy = {};
    bufferCopy.dstOffset = uiOffset;
    bufferCopy.srcOffset = uiOffset;
    bufferCopy.size = uiSize;
    m_currentCommandBuffer.copyBuffer(stagingBuffer, gpuBuffer, 1, &bufferCopy);

    barriers.BufferBarrier(gpuBuffer,
      ezGALResourceState::CopyDestination, ezGALResourceState::ConstantBuffer);
  }
  else
  {
    // gpuBuffer is writable on the CPU and thus we only need to add a barrier.
    barriers.BufferBarrier(gpuBuffer,
      ezGALResourceState::CpuWrite, ezGALResourceState::ConstantBuffer);
  }
}

void ezInitContextVulkan::ExecutePendingCopies(ezArrayPtr<ezPendingBufferCopyVulkan> buffers, ezArrayPtr<ezPendingTextureCopyVulkan> textures)
{
  if (buffers.IsEmpty() && textures.IsEmpty())
    return;

  auto getRange = [](const vk::ImageSubresourceLayers& layers) -> vk::ImageSubresourceRange
  {
    vk::ImageSubresourceRange range;
    range.aspectMask = layers.aspectMask;
    range.baseMipLevel = layers.mipLevel;
    range.levelCount = 1;
    range.baseArrayLayer = layers.baseArrayLayer;
    range.layerCount = layers.layerCount;
    return range;
  };

  EZ_LOCK(m_Lock);
  EnsureCommandBufferExists();

  ezBarrierUtilsVulkan barriers(*m_pDevice, m_currentCommandBuffer);

  // Transition resources to copy-compatible states.
  for (const ezPendingBufferCopyVulkan& bufferCopy : buffers)
  {
    const ezBitflags<ezGALResourceState> defaultState = bufferCopy.m_pDstBuffer->GetDescription().GetDefaultState();

    barriers.BufferBarrier(bufferCopy.m_SrcBuffer.m_buffer,
      ezGALResourceState::CpuWrite, ezGALResourceState::CopySource);

    barriers.BufferBarrier(bufferCopy.m_pDstBuffer->GetVkBuffer(),
      defaultState, ezGALResourceState::CopyDestination);
  }
  for (const ezPendingTextureCopyVulkan& textureCopy : textures)
  {
    const ezBitflags<ezGALResourceState> defaultState = textureCopy.m_pDstTexture->GetDescription().GetDefaultState();

    barriers.TextureBarrier(textureCopy.m_pDstTexture->GetImage(), getRange(textureCopy.m_Region.imageSubresource),
      defaultState, ezGALResourceState::CopyDestination);

    barriers.BufferBarrier(textureCopy.m_SrcBuffer.m_buffer,
      ezGALResourceState::CpuWrite, ezGALResourceState::CopySource);
  }

  // Execute the actual copies.
  for (const ezPendingBufferCopyVulkan& bufferCopy : buffers)
  {
    m_currentCommandBuffer.copyBuffer(bufferCopy.m_SrcBuffer.m_buffer, bufferCopy.m_pDstBuffer->GetVkBuffer(), 1, &bufferCopy.m_Region);
  }
  for (const ezPendingTextureCopyVulkan& textureCopy : textures)
  {
    m_currentCommandBuffer.copyBufferToImage(textureCopy.m_SrcBuffer.m_buffer, textureCopy.m_pDstTexture->GetImage(), vk::ImageLayout::eTransferDstOptimal, 1, &textureCopy.m_Region);
  }

  // Transition resources back to their default states.
  for (const ezPendingBufferCopyVulkan& bufferCopy : buffers)
  {
    const ezBitflags<ezGALResourceState> defaultState = bufferCopy.m_pDstBuffer->GetDescription().GetDefaultState();

    barriers.BufferBarrier(bufferCopy.m_pDstBuffer->GetVkBuffer(),
      ezGALResourceState::CopyDestination, defaultState);
  }
  for (const ezPendingTextureCopyVulkan& textureCopy : textures)
  {
    const ezBitflags<ezGALResourceState> defaultState = textureCopy.m_pDstTexture->GetDescription().GetDefaultState();

    barriers.TextureBarrier(textureCopy.m_pDstTexture->GetImage(), getRange(textureCopy.m_Region.imageSubresource),
      ezGALResourceState::CopyDestination, defaultState);
  }
}
