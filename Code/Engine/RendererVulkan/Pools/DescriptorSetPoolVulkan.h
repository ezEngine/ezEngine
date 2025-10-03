#pragma once

#include <RendererVulkan/Device/DeclarationsVulkan.h>
#include <RendererVulkan/RendererVulkanDLL.h>

class ezGALDeviceVulkan;

/// \brief Creates persistent descriptor sets.
/// Each bind group layout requests a pool via GetPool. There is one pool for each resource usage distribution. This is efficient because the spec says: "Additionally, if all sets allocated from the pool since it was created or most recently reset use the same number of descriptors (of each type) and the requested allocation also uses that same number of descriptors (of each type), then fragmentation must not cause an allocation failure."
/// Each pool will create sub-pools in increasing size once all current pools are full.
class EZ_RENDERERVULKAN_DLL ezDescriptorSetPoolVulkan : public ezRefCounted
{
public:
  static void Initialize(ezGALDeviceVulkan* pDevice);
  static void DeInitialize();
  static ezSharedPtr<ezDescriptorSetPoolVulkan> GetPool(const ezBindGroupLayoutResourceUsageVulkan& resourceUsage);
  static ezUInt32 GetPoolCount();
  static void MarkPoolDirty(ezDescriptorSetPoolVulkan* pPool);
  static void BeginFrame();

public:
  ~ezDescriptorSetPoolVulkan();

  struct Allocation
  {
    ezUInt32 m_uiPoolIndex = 0;
  };

  /// \brief Creates a descriptor set for the given layout.
  /// \param hBindGroupLayout The layout for which to create the descriptor set.
  /// \param out_Allocation Filled out by the call. Needs to be passed in ReclaimDescriptorSet once the descriptor set is no longer used by the GPU.
  vk::DescriptorSet CreateDescriptorSet(ezGALBindGroupLayoutHandle hBindGroupLayout, Allocation& out_Allocation);

  /// \brief Reclaims a descriptor set for reuse.
  /// This function should only be called by ezGALDeviceVulkan when it is safe to reuse the descriptor set. To reclaim a descriptor set created via CreateDescriptorSet, call ezGALDeviceVulkan->ReclaimLater(vk::DescriptorSet, ezDescriptorSetPoolVulkan*, Allocation::m_uiPoolIndex);
  void ReclaimDescriptorSet(vk::DescriptorSet descriptorSet, Allocation allocation);

private:
  struct ResourceUsageHash
  {
    EZ_ALWAYS_INLINE static ezUInt32 Hash(const ezBindGroupLayoutResourceUsageVulkan& a);
    EZ_ALWAYS_INLINE static bool Equal(const ezBindGroupLayoutResourceUsageVulkan& a, const ezBindGroupLayoutResourceUsageVulkan& b);
  };

  static ezGALDeviceVulkan* s_pDevice;
  static ezMutex s_Mutex;
  static ezHashTable<ezBindGroupLayoutResourceUsageVulkan, ezSharedPtr<ezDescriptorSetPoolVulkan>, ResourceUsageHash> s_Pools; ///< Cache of all pools
  static ezHashSet<ezDescriptorSetPoolVulkan*> s_DirtyPools;                                                                   ///< Pools that need to free resources and can potentially be destroyed.

private:
  struct DescriptorSubPool
  {
    vk::DescriptorPool m_DescriptorPool;
    ezUInt32 m_uiPoolSize = 0;
    ezUInt32 m_uiAllocatedSets = 0;
    ezDynamicArray<vk::DescriptorSet> m_Reclaim;
  };

  ezDescriptorSetPoolVulkan(ezGALDeviceVulkan* pDevice, const ezBindGroupLayoutResourceUsageVulkan& resourceUsage);
  ezUInt32 GetFreePoolIndex();

  /// \brief Reclaims resources previously added via ReclaimDescriptorSet.
  /// \return Returns true if the pool no longer holds any descriptor sets and can thus be safely deleted.
  bool ReclaimResources();

private:
  ezMutex m_Mutex;
  ezGALDeviceVulkan* m_pDevice = nullptr;
  ezBindGroupLayoutResourceUsageVulkan m_ResourceUsage;        ///< How many resources of each type each descriptor set uses.
  ezUInt32 m_uiNextPoolSize = 64;                              ///< When the current pool runs out of data, allocate next pool with this size and double the value.
  ezUInt32 m_uiTotalAllocations = 0;                           ///< Total allocations across all sub-pools.

  ezUInt32 m_uiActivePool = ezInvalidIndex;                    ///< The current pool with free allocations.
  ezHybridArray<ezUniquePtr<DescriptorSubPool>, 1> m_SubPools; ///< Available descriptor sub-pools.
  ezHashSet<DescriptorSubPool*> m_DirtySubPools;               ///< Descriptor sub-pools that have sets that need to be freed.
};