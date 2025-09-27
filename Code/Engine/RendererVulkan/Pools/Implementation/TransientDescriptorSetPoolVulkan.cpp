#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Pools/TransientDescriptorSetPoolVulkan.h>

vk::DescriptorPool ezTransientDescriptorSetPoolVulkan::s_currentTransientPool;
ezHybridArray<vk::DescriptorPool, 4> ezTransientDescriptorSetPoolVulkan::s_freeTransientPools;
vk::Device ezTransientDescriptorSetPoolVulkan::s_device;
ezHashTable<vk::DescriptorType, float> ezTransientDescriptorSetPoolVulkan::s_descriptorWeights;

void ezTransientDescriptorSetPoolVulkan::Initialize(vk::Device device)
{
  s_device = device;
  s_descriptorWeights[vk::DescriptorType::eSampler] = 0.5f;              // Image sampler.
  s_descriptorWeights[vk::DescriptorType::eSampledImage] = 2.0f;         // Read-only image view.
  s_descriptorWeights[vk::DescriptorType::eStorageImage] = 1.0f;         // Read / write image view.
  s_descriptorWeights[vk::DescriptorType::eUniformBuffer] = 1.0f;        // Read-only struct (constant buffer)
  s_descriptorWeights[vk::DescriptorType::eStorageBuffer] = 1.0f;        // Read / write struct (UAV).
  s_descriptorWeights[vk::DescriptorType::eUniformTexelBuffer] = 0.5f;   // Read-only linear texel buffer with view.
  s_descriptorWeights[vk::DescriptorType::eCombinedImageSampler] = 2.0f; // Read-only image view with image sampler.

  // Not used by EZ so far.
  s_descriptorWeights[vk::DescriptorType::eStorageTexelBuffer] = 0.5f;   // Read / write linear texel buffer with view.
  s_descriptorWeights[vk::DescriptorType::eUniformBufferDynamic] = 1.0f; // Same as eUniformBuffer but allows updating the memory offset into the buffer dynamically.
  s_descriptorWeights[vk::DescriptorType::eStorageBufferDynamic] = 0.0f; // Same as eStorageBuffer but allows updating the memory offset into the buffer dynamically.

  // Not supported by EZ so far.
  s_descriptorWeights[vk::DescriptorType::eInputAttachment] = 0.0f; // frame-buffer local read-only image view.

  s_descriptorWeights[vk::DescriptorType::eInlineUniformBlock] = 0.0f;
  s_descriptorWeights[vk::DescriptorType::eAccelerationStructureKHR] = 0.0f;
  s_descriptorWeights[vk::DescriptorType::eAccelerationStructureNV] = 0.0f;
  s_descriptorWeights[vk::DescriptorType::eMutableVALVE] = 0.0f;
}

void ezTransientDescriptorSetPoolVulkan::DeInitialize()
{
  s_descriptorWeights.Clear();
  s_descriptorWeights.Compact();

  for (vk::DescriptorPool& pool : s_freeTransientPools)
  {
    s_device.destroyDescriptorPool(pool, nullptr);
  }
  s_freeTransientPools.Clear();
  s_freeTransientPools.Compact();
  if (s_currentTransientPool)
  {
    s_device.resetDescriptorPool(s_currentTransientPool);
    s_device.destroyDescriptorPool(s_currentTransientPool, nullptr);
    s_currentTransientPool = nullptr;
  }

  s_device = nullptr;
}

ezHashTable<vk::DescriptorType, float>& ezTransientDescriptorSetPoolVulkan::AccessDescriptorPoolWeights()
{
  return s_descriptorWeights;
}

vk::DescriptorSet ezTransientDescriptorSetPoolVulkan::CreateTransientDescriptorSet(vk::DescriptorSetLayout layout)
{
  vk::DescriptorSet set;
  if (!s_currentTransientPool)
  {
    s_currentTransientPool = GetNewTransientPool();
  }

  vk::DescriptorSetAllocateInfo allocateInfo;
  allocateInfo.pSetLayouts = &layout;
  allocateInfo.descriptorPool = s_currentTransientPool;
  allocateInfo.descriptorSetCount = 1;

  vk::Result res = s_device.allocateDescriptorSets(&allocateInfo, &set);
  bool bPoolExhausted = false;

  switch (res)
  {
    case vk::Result::eSuccess:
      break;
    case vk::Result::eErrorFragmentedPool:
    case vk::Result::eErrorOutOfPoolMemory:
      bPoolExhausted = true;
      break;
    default:
      VK_ASSERT_DEV(res);
      break;
  }

  if (bPoolExhausted)
  {
    ezGALDeviceVulkan* pDevice = static_cast<ezGALDeviceVulkan*>(ezGALDevice::GetDefaultDevice());
    pDevice->ReclaimLater(s_currentTransientPool);
    s_currentTransientPool = GetNewTransientPool();
    allocateInfo.descriptorPool = s_currentTransientPool;
    VK_ASSERT_DEV(s_device.allocateDescriptorSets(&allocateInfo, &set));
  }

  return set;
}

void ezTransientDescriptorSetPoolVulkan::UpdateDescriptorSet(vk::DescriptorSet descriptorSet, ezArrayPtr<vk::WriteDescriptorSet> update)
{
  s_device.updateDescriptorSets(update.GetCount(), update.GetPtr(), 0, nullptr);
}

void ezTransientDescriptorSetPoolVulkan::ReclaimPool(vk::DescriptorPool& descriptorPool)
{
  s_device.resetDescriptorPool(descriptorPool);
  s_freeTransientPools.PushBack(descriptorPool);
}

vk::DescriptorPool ezTransientDescriptorSetPoolVulkan::GetNewTransientPool()
{
  if (s_freeTransientPools.IsEmpty())
  {
    ezHybridArray<vk::DescriptorPoolSize, 20> poolSizes;
    for (auto weight : s_descriptorWeights)
    {
      if (static_cast<ezUInt32>(weight.Value() * s_uiPoolBaseSize) > 0)
        poolSizes.PushBack(vk::DescriptorPoolSize(weight.Key(), static_cast<ezUInt32>(weight.Value() * s_uiPoolBaseSize)));
    }
    vk::DescriptorPoolCreateInfo poolCreateInfo;
    poolCreateInfo.flags = {};
    poolCreateInfo.maxSets = s_uiPoolBaseSize;
    poolCreateInfo.poolSizeCount = poolSizes.GetCount();
    poolCreateInfo.pPoolSizes = poolSizes.GetData();

    vk::DescriptorPool descriptorPool;
    VK_ASSERT_DEV(s_device.createDescriptorPool(&poolCreateInfo, nullptr, &descriptorPool));
    return descriptorPool;
  }
  else
  {
    vk::DescriptorPool pool = s_freeTransientPools.PeekBack();
    s_freeTransientPools.PopBack();
    return pool;
  }
}