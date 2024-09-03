#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Pools/SemaphorePoolVulkan.h>

vk::Device ezSemaphorePoolVulkan::s_device;
ezHybridArray<vk::Semaphore, 4> ezSemaphorePoolVulkan::s_semaphores;

void ezSemaphorePoolVulkan::Initialize(vk::Device device)
{
  s_device = device;
}

void ezSemaphorePoolVulkan::DeInitialize()
{
  for (vk::Semaphore& semaphore : s_semaphores)
  {
    s_device.destroySemaphore(semaphore, nullptr);
  }
  s_semaphores.Clear();
  s_semaphores.Compact();

  s_device = nullptr;
}

vk::Semaphore ezSemaphorePoolVulkan::RequestSemaphore()
{
  EZ_ASSERT_DEBUG(s_device, "ezSemaphorePoolVulkan::Initialize not called");
  if (!s_semaphores.IsEmpty())
  {
    vk::Semaphore semaphore = s_semaphores.PeekBack();
    s_semaphores.PopBack();
    return semaphore;
  }
  else
  {
    vk::Semaphore semaphore;
    vk::SemaphoreCreateInfo semaphoreCreateInfo;
    VK_ASSERT_DEV(s_device.createSemaphore(&semaphoreCreateInfo, nullptr, &semaphore));
    return semaphore;
  }
}

void ezSemaphorePoolVulkan::ReclaimSemaphore(vk::Semaphore& semaphore)
{
  EZ_ASSERT_DEBUG(s_device, "ezSemaphorePoolVulkan::Initialize not called");
  s_semaphores.PushBack(semaphore);
}


