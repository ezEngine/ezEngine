#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

EZ_DEFINE_AS_POD_TYPE(vk::DescriptorType);

template <>
struct ezHashHelper<vk::DescriptorType>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(vk::DescriptorType value) { return ezHashHelper<ezUInt32>::Hash(ezUInt32(value)); }
  EZ_ALWAYS_INLINE static bool Equal(vk::DescriptorType a, vk::DescriptorType b) { return a == b; }
};

class EZ_RENDERERVULKAN_DLL ezDescriptorSetPoolVulkan
{
public:
  static void Initialize(vk::Device device);
  static void DeInitialize();
  static ezHashTable<vk::DescriptorType, float>& AccessDescriptorPoolWeights();

  static vk::DescriptorSet CreateDescriptorSet(vk::DescriptorSetLayout layout);
  static void UpdateDescriptorSet(vk::DescriptorSet descriptorSet, ezArrayPtr<vk::WriteDescriptorSet> update);
  static void ReclaimPool(vk::DescriptorPool& descriptorPool);

private:
  static constexpr ezUInt32 s_uiPoolBaseSize = 1024;

  static vk::DescriptorPool GetNewPool();

  static vk::DescriptorPool s_currentPool;
  static ezHybridArray<vk::DescriptorPool, 4> s_freePools;
  static vk::Device s_device;
  static ezHashTable<vk::DescriptorType, float> s_descriptorWeights;
};
