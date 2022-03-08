#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

/// \brief Cut down version of a vk::ImageMemoryBarrier with better defaults. Remaining data is retrieved from the view handle passed into ezCommandBufferUtilsVulkan::CmdImageMemoryBarrier.
struct ezImageMemoryBarrierVulkan
{
  vk::PipelineStageFlags src_stage_mask = vk::PipelineStageFlagBits::eBottomOfPipe;
  vk::PipelineStageFlags dst_stage_mask = vk::PipelineStageFlagBits::eTopOfPipe;
  vk::AccessFlags src_access_mask = {};
  vk::AccessFlags dst_access_mask = {};
  vk::ImageLayout old_layout = vk::ImageLayout::eUndefined;
  vk::ImageLayout new_layout = vk::ImageLayout::eUndefined;
  ezUInt32 old_queue_family = VK_QUEUE_FAMILY_IGNORED;
  ezUInt32 new_queue_family = VK_QUEUE_FAMILY_IGNORED;
};

/// \brief Helper functions to insert commands into a command buffer.
class EZ_RENDERERVULKAN_DLL ezCommandBufferUtilsVulkan
{
public:
  /// \brief Inserts a memory barrier into a command buffer.
  /// \param commandBuffer Where to insert the barrier.
  /// \param hView Subresource range and texture is extracted from this.
  /// \param memoryBarrier Barrier settings.
  static void CmdImageMemoryBarrier(vk::CommandBuffer commandBuffer, ezGALRenderTargetViewHandle hView, const ezImageMemoryBarrierVulkan& memoryBarrier);

  /// \brief Inserts a memory barrier into a command buffer.
  /// \param commandBuffer Where to insert the barrier.
  /// \param hView Subresource range and texture is extracted from this.
  /// \param memoryBarrier Barrier settings.
  static void CmdImageMemoryBarrier(vk::CommandBuffer commandBuffer, ezGALResourceViewHandle hView, const ezImageMemoryBarrierVulkan& memoryBarrier);
};
