#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/Pools/CommandBufferPoolVulkan.h>
#include <RendererVulkan/Pools/StagingBufferPoolVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/PipelineBarrierVulkan.h>


ezInitContextVulkan::ezInitContextVulkan(ezGALDeviceVulkan* pDevice)
  : m_pDevice(pDevice)
{
  ezProxyAllocator& allocator = m_pDevice->GetAllocator();
  m_pPipelineBarrier = EZ_NEW(&allocator, ezPipelineBarrierVulkan);
  m_pCommandBufferPool = EZ_NEW(&allocator, ezCommandBufferPoolVulkan);
  m_pCommandBufferPool->Initialize(m_pDevice->GetVulkanDevice(), m_pDevice->GetGraphicsQueue().m_uiQueueFamily);
  m_pStagingBufferPool = EZ_NEW(&allocator, ezStagingBufferPoolVulkan);
  m_pStagingBufferPool->Initialize(m_pDevice);
}

ezInitContextVulkan::~ezInitContextVulkan()
{
  m_pCommandBufferPool->DeInitialize();
  m_pStagingBufferPool->DeInitialize();
}

vk::CommandBuffer ezInitContextVulkan::GetFinishedCommandBuffer()
{
  EZ_LOCK(m_Lock);
  if (m_currentCommandBuffer)
  {
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
    m_pPipelineBarrier->SetInitialImageState(pTexture, createInfo.initialLayout);
    if (!pInitialData.IsEmpty())
    {
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
          subresourceLayers.aspectMask = ezGALResourceFormat::IsDepthFormat(pTexture->GetDescription().m_Format) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
          subresourceLayers.mipLevel = uiMipLevel;
          subresourceLayers.baseArrayLayer = uiLayer;
          subresourceLayers.layerCount = 1;

          m_pDevice->UploadTextureStaging(m_pStagingBufferPool.Borrow(), m_pPipelineBarrier.Borrow(), m_currentCommandBuffer, pTexture, subresourceLayers, subResourceData);
        }
      }
    }
    else
    {
      // Barrier the entire texture into it's default layout.
      m_pPipelineBarrier->EnsureImageLayout(pTexture, pTexture->GetPreferredLayout(), pTexture->GetUsedByPipelineStage(), pTexture->GetAccessMask(), true);
    }
  }
  else
  {
    // We don't actually know what the current state is of an existing native object. The only use case right now are back buffers created by swap chains so for now we just throw away the current content and barrier into the preferred layout.
    m_pPipelineBarrier->SetInitialImageState(pTexture, vk::ImageLayout::eUndefined);
    m_pPipelineBarrier->EnsureImageLayout(pTexture, pTexture->GetPreferredLayout(), pTexture->GetUsedByPipelineStage(), pTexture->GetAccessMask(), true);
  }
}
