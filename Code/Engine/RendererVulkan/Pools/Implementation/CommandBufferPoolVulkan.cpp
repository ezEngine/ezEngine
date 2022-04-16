#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Pools/CommandBufferPoolVulkan.h>

vk::Device ezCommandBufferPoolVulkan::s_device;
vk::CommandPool ezCommandBufferPoolVulkan::s_commandPool;
ezHybridArray<vk::CommandBuffer, 4> ezCommandBufferPoolVulkan::s_CommandBuffers;


void ezCommandBufferPoolVulkan::Initialize(vk::Device device, ezUInt32 graphicsFamilyIndex)
{
  s_device = device;

  // Command buffer
  vk::CommandPoolCreateInfo commandPoolCreateInfo = {};
  commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
  commandPoolCreateInfo.queueFamilyIndex = graphicsFamilyIndex;

  s_commandPool = s_device.createCommandPool(commandPoolCreateInfo);
}

void ezCommandBufferPoolVulkan::DeInitialize()
{
  for (vk::CommandBuffer& commandBuffer : s_CommandBuffers)
  {
    s_device.freeCommandBuffers(s_commandPool, 1, &commandBuffer);
  }
  s_CommandBuffers.Clear();
  s_CommandBuffers.Compact();

  s_device.destroyCommandPool(s_commandPool);
  s_commandPool = nullptr;

  s_device = nullptr;
}

vk::CommandBuffer ezCommandBufferPoolVulkan::RequestCommandBuffer()
{
  EZ_ASSERT_DEBUG(s_device, "ezCommandBufferPoolVulkan::Initialize not called");
  if (!s_CommandBuffers.IsEmpty())
  {
    vk::CommandBuffer CommandBuffer = s_CommandBuffers.PeekBack();
    s_CommandBuffers.PopBack();
    return CommandBuffer;
  }
  else
  {
    vk::CommandBuffer commandBuffer;

    vk::CommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.commandBufferCount = 1;
    commandBufferAllocateInfo.commandPool = s_commandPool;
    commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;

    VK_ASSERT_DEV(s_device.allocateCommandBuffers(&commandBufferAllocateInfo, &commandBuffer));
    return commandBuffer;
  }
}

void ezCommandBufferPoolVulkan::ReclaimCommandBuffer(vk::CommandBuffer& commandBuffer)
{
  EZ_ASSERT_DEBUG(s_device, "ezCommandBufferPoolVulkan::Initialize not called");
  commandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
  s_CommandBuffers.PushBack(commandBuffer);
}
