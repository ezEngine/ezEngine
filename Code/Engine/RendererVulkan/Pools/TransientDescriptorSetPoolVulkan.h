#pragma once

#include <RendererVulkan/Device/DeclarationsVulkan.h>
#include <RendererVulkan/RendererVulkanDLL.h>

EZ_DEFINE_AS_POD_TYPE(vk::DescriptorType);

template <>
struct ezHashHelper<vk::DescriptorType>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(vk::DescriptorType value) { return ezHashHelper<ezUInt32>::Hash(ezUInt32(value)); }
  EZ_ALWAYS_INLINE static bool Equal(vk::DescriptorType a, vk::DescriptorType b) { return a == b; }
};

/// \brief Creates descriptor sets that are only valid for a single frame.
/// Descriptors are created from pools that are put in the frame deletion queue once full. The frame deletion queue on the device will then call ReclaimPool and the entire pool is reset and reused. Pools are only destroyed on shutdown.
class EZ_RENDERERVULKAN_DLL ezTransientDescriptorSetPoolVulkan
{
public:
  static void Initialize(vk::Device device);
  static void DeInitialize();
  static ezHashTable<vk::DescriptorType, float>& AccessDescriptorPoolWeights();

  static vk::DescriptorSet CreateTransientDescriptorSet(vk::DescriptorSetLayout layout);
  static void UpdateDescriptorSet(vk::DescriptorSet descriptorSet, ezArrayPtr<vk::WriteDescriptorSet> update);
  static void ReclaimPool(vk::DescriptorPool& descriptorPool);

private:
  static constexpr ezUInt32 s_uiPoolBaseSize = 1024;

  static vk::DescriptorPool GetNewTransientPool();

  static vk::DescriptorPool s_currentTransientPool;
  static ezHybridArray<vk::DescriptorPool, 4> s_freeTransientPools;

  static vk::Device s_device;
  static ezHashTable<vk::DescriptorType, float> s_descriptorWeights;
};