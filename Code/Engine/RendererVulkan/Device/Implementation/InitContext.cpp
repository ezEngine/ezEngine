#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/Pools/CommandBufferPoolVulkan.h>
#include <RendererVulkan/Pools/StagingBufferPoolVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>
#include <RendererVulkan/Utils/PipelineBarrierVulkan.h>

ezInitContextVulkan::ezInitContextVulkan(ezGALDeviceVulkan* pDevice)
  : m_pDevice(pDevice)
{
  ezAllocator* pAllocator = m_pDevice->GetAllocator();
  m_pPipelineBarrier = EZ_NEW(pAllocator, ezPipelineBarrierVulkan, pAllocator);
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
  m_pPipelineBarrier.Clear();
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
    m_pPipelineBarrier->Submit();
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
    m_pPipelineBarrier->SetCommandBuffer(&m_currentCommandBuffer);
  }
}


void ezInitContextVulkan::InitTexture(const ezGALTextureVulkan* pTexture, vk::ImageCreateInfo& createInfo, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData)
{
  EZ_LOCK(m_Lock);

  EnsureCommandBufferExists();
  if (pTexture->GetDescription().m_pExisitingNativeObject == nullptr)
  {
    if (pTexture->GetDescription().m_SampleCount != ezGALMSAASampleCount::None)
    {
      // #TODO_VULKAN how do we clear a MS target to zero?
      m_pPipelineBarrier->SetInitialImageState(pTexture, vk::ImageLayout::eUndefined);
      m_pPipelineBarrier->EnsureImageLayout(pTexture, pTexture->GetPreferredLayout(), pTexture->GetUsedByPipelineStage(), pTexture->GetAccessMask(), true);
      return;
    }

    m_pPipelineBarrier->SetInitialImageState(pTexture, createInfo.initialLayout);

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

    m_pPipelineBarrier->EnsureImageLayout(pTexture, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite);
    // We need to flush here as all UploadTextureStaging calls will need to barrier on eTransferDstOptimal anyways.
    m_pPipelineBarrier->Flush();
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

        m_pDevice->UploadTextureStaging(m_pStagingBufferPool.Borrow(), m_pPipelineBarrier.Borrow(), m_currentCommandBuffer, pTexture, subresourceLayers, imageOffset, imageExtent, subResourceData);
      }
    }
  }
  else
  {
    // We don't actually know what the current state is of an existing native object. The only use case right now are back buffers created by swap chains so for now we just throw away the current content and barrier into the preferred layout.
    m_pPipelineBarrier->SetInitialImageState(pTexture, vk::ImageLayout::eUndefined);
    m_pPipelineBarrier->EnsureImageLayout(pTexture, pTexture->GetPreferredLayout(), pTexture->GetUsedByPipelineStage(), pTexture->GetAccessMask(), true);
  }
}

void ezInitContextVulkan::TextureDestroyed(const ezGALTextureVulkan* pTexture)
{
  EZ_LOCK(m_Lock);
  m_pPipelineBarrier->TextureDestroyed(pTexture);
}

void ezInitContextVulkan::InitBuffer(const ezGALBufferVulkan* pBuffer, ezConstByteArrayPtr pInitialData)
{
  EZ_LOCK(m_Lock);

  EnsureCommandBufferExists();

  // During initialization, there can't be any read/write hazard with the GPU so we can write to the memory directly if supported, e.g. unified memory.
  const ezVulkanAllocationInfo& allocInfo = pBuffer->GetAllocationInfo();
  if (allocInfo.m_pMappedData != nullptr)
  {
    ezMemoryUtils::Copy((ezUInt8*)allocInfo.m_pMappedData, pInitialData.GetPtr(), pInitialData.GetCount());

    m_pPipelineBarrier->AccessBuffer(pBuffer, 0, pInitialData.GetCount(), vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, pBuffer->GetUsedByPipelineStage(), pBuffer->GetAccessMask());
  }
  else
  {
    m_pDevice->UploadBufferStaging(m_pStagingBufferPool.Borrow(), m_pPipelineBarrier.Borrow(), m_currentCommandBuffer, pBuffer, pInitialData, 0);
  }
}

void ezInitContextVulkan::UpdateTexture(const ezGALTextureVulkan* pTexture, const ezGALTextureSubresource& subresource, const ezBoundingBoxu32& box, const ezGALSystemMemoryDescription& sourceData)
{
  EZ_LOCK(m_Lock);

  EnsureCommandBufferExists();

  const ezVec3U32 boxExtents = box.GetExtents();
  const vk::Offset3D imageOffset = {(ezInt32)box.m_vMin.x, (ezInt32)box.m_vMin.y, (ezInt32)box.m_vMin.z};
  const vk::Extent3D imageExtent = {boxExtents.x, boxExtents.y, boxExtents.z};

  vk::ImageSubresourceLayers subresourceLayers;
  subresourceLayers.aspectMask = ezConversionUtilsVulkan::IsDepthFormat(pTexture->GetImageFormat()) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  subresourceLayers.mipLevel = subresource.m_uiMipLevel;
  subresourceLayers.baseArrayLayer = subresource.m_uiArraySlice;
  subresourceLayers.layerCount = 1;

  m_pDevice->UploadTextureStaging(m_pStagingBufferPool.Borrow(), m_pPipelineBarrier.Borrow(), m_currentCommandBuffer, pTexture, subresourceLayers, imageOffset, imageExtent, sourceData);
}

void ezInitContextVulkan::UpdateBuffer(const ezGALBufferVulkan* pBuffer, ezUInt32 uiOffset, ezConstByteArrayPtr pSourceData)
{
  EZ_LOCK(m_Lock);

  EnsureCommandBufferExists();
  m_pDevice->UploadBufferStaging(m_pStagingBufferPool.Borrow(), m_pPipelineBarrier.Borrow(), m_currentCommandBuffer, pBuffer, pSourceData, uiOffset);
}

void ezInitContextVulkan::UpdateDynamicUniformBuffer(vk::Buffer gpuBuffer, vk::Buffer stagingBuffer, ezUInt32 uiOffset, ezUInt32 uiSize)
{
  EZ_LOCK(m_Lock);

  EnsureCommandBufferExists();

  if (stagingBuffer)
  {
    // gpuBuffer can't be accessed on the CPU, so the data is present in stagingBuffer and needs to be copied over.
    m_pPipelineBarrier->AddBufferBarrierInternal(stagingBuffer, uiOffset, uiSize, vk::PipelineStageFlagBits::eHost, vk::AccessFlagBits::eHostWrite, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferRead);

    m_pPipelineBarrier->Flush();

    vk::BufferCopy bufferCopy = {};
    bufferCopy.dstOffset = uiOffset;
    bufferCopy.srcOffset = uiOffset;
    bufferCopy.size = uiSize;
    m_currentCommandBuffer.copyBuffer(stagingBuffer, gpuBuffer, 1, &bufferCopy);

    m_pPipelineBarrier->AddBufferBarrierInternal(gpuBuffer, uiOffset, uiSize, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eVertexShader, vk::AccessFlagBits::eUniformRead);
  }
  else
  {
    // gpuBuffer is writable on the CPU and thus we only need to add a barrier.
    m_pPipelineBarrier->AddBufferBarrierInternal(gpuBuffer, uiOffset, uiSize, vk::PipelineStageFlagBits::eHost, vk::AccessFlagBits::eHostWrite, vk::PipelineStageFlagBits::eVertexShader, vk::AccessFlagBits::eUniformRead);
  }
}
