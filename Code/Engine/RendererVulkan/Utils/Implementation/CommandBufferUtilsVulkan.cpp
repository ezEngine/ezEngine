#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/ResourceView.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/CommandBufferUtilsVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

namespace
{
  void CmdImageMemoryBarrierInternal(vk::CommandBuffer commandBuffer, vk::Image image, vk::ImageSubresourceRange range, const ezImageMemoryBarrierVulkan& memoryBarrier)
  {
    vk::ImageMemoryBarrier image_memory_barrier;
    {
      image_memory_barrier.oldLayout = memoryBarrier.old_layout;
      image_memory_barrier.newLayout = memoryBarrier.new_layout;
      image_memory_barrier.image = image;
      image_memory_barrier.subresourceRange = range;
      image_memory_barrier.srcAccessMask = memoryBarrier.src_access_mask;
      image_memory_barrier.dstAccessMask = memoryBarrier.dst_access_mask;
      image_memory_barrier.srcQueueFamilyIndex = memoryBarrier.old_queue_family;
      image_memory_barrier.dstQueueFamilyIndex = memoryBarrier.new_queue_family;
    }
    commandBuffer.pipelineBarrier(memoryBarrier.src_stage_mask, memoryBarrier.dst_stage_mask, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &image_memory_barrier);
  }
} // namespace

void ezCommandBufferUtilsVulkan::CmdImageMemoryBarrier(vk::CommandBuffer commandBuffer, ezGALRenderTargetViewHandle hView, const ezImageMemoryBarrierVulkan& memoryBarrier)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  const ezGALRenderTargetView* pView = pDevice->GetRenderTargetView(hView);
  const ezGALRenderTargetViewCreationDescription& desc = pView->GetDescription();
  const auto* pTex = static_cast<const ezGALTextureVulkan*>(pView->GetTexture());

  CmdImageMemoryBarrierInternal(commandBuffer, pTex->GetImage(), ezConversionUtilsVulkan::GetSubresourceRange(pTex->GetDescription(), desc), memoryBarrier);
}

void ezCommandBufferUtilsVulkan::CmdImageMemoryBarrier(vk::CommandBuffer commandBuffer, ezGALResourceViewHandle hView, const ezImageMemoryBarrierVulkan& memoryBarrier)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  const ezGALResourceView* pView = pDevice->GetResourceView(hView);
  const ezGALResourceViewCreationDescription& desc = pView->GetDescription();
  if (!desc.m_hTexture.IsInvalidated())
  {
    const auto* pTex = static_cast<const ezGALTextureVulkan*>(pView->GetResource());

    CmdImageMemoryBarrierInternal(commandBuffer, pTex->GetImage(), ezConversionUtilsVulkan::GetSubresourceRange(pTex->GetDescription(), desc), memoryBarrier);
  }
  else
  {
    //#TODO_VULKAN Implement
    EZ_ASSERT_NOT_IMPLEMENTED;
  }
}
