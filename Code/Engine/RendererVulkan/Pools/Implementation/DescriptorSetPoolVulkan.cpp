#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Pools/DescriptorSetPoolVulkan.h>
#include <RendererVulkan/Pools/TransientDescriptorSetPoolVulkan.h>
#include <RendererVulkan/Shader/BindGroupLayoutVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

//////////////////////////////////////////////////////////////////////

ezGALDeviceVulkan* ezDescriptorSetPoolVulkan::s_pDevice = nullptr;
ezMutex ezDescriptorSetPoolVulkan::s_Mutex;
ezHashTable<ezBindGroupLayoutResourceUsageVulkan, ezSharedPtr<ezDescriptorSetPoolVulkan>, ezDescriptorSetPoolVulkan::ResourceUsageHash> ezDescriptorSetPoolVulkan::s_Pools;
ezHashSet<ezDescriptorSetPoolVulkan*> ezDescriptorSetPoolVulkan::s_DirtyPools;

ezUInt32 ezDescriptorSetPoolVulkan::ResourceUsageHash::Hash(const ezBindGroupLayoutResourceUsageVulkan& a)
{
  return ezHashingUtils::xxHash32(&a.m_Usage[0], ezGALShaderResourceType::COUNT);
}

bool ezDescriptorSetPoolVulkan::ResourceUsageHash::Equal(const ezBindGroupLayoutResourceUsageVulkan& a, const ezBindGroupLayoutResourceUsageVulkan& b)
{
  for (ezUInt32 i = 0; i < ezGALShaderResourceType::COUNT; ++i)
  {
    if (a.m_Usage[i] != b.m_Usage[i])
      return false;
  }

  return true;
}

void ezDescriptorSetPoolVulkan::Initialize(ezGALDeviceVulkan* pDevice)
{
  s_pDevice = pDevice;
}

void ezDescriptorSetPoolVulkan::DeInitialize()
{
  // Destroy all pools, make sure ref count is zero.
  for (auto it : s_Pools)
  {
    EZ_ASSERT_DEBUG(it.Value()->GetRefCount() == 1, "Pool still referenced. A bind group layout probably leaked");
  }
  s_Pools.Clear();
  s_Pools.Compact();
  s_DirtyPools.Clear();
  s_DirtyPools.Compact();
  s_pDevice = nullptr;
}

ezSharedPtr<ezDescriptorSetPoolVulkan> ezDescriptorSetPoolVulkan::GetPool(const ezBindGroupLayoutResourceUsageVulkan& resourceUsage)
{
  EZ_LOCK(s_Mutex);
  auto it = s_Pools.Find(resourceUsage);
  if (it.IsValid())
  {
    return it.Value();
  }

  ezSharedPtr<ezDescriptorSetPoolVulkan> pPool = EZ_DEFAULT_NEW(ezDescriptorSetPoolVulkan, s_pDevice, resourceUsage);
  s_Pools[resourceUsage] = pPool;

  return pPool;
}

ezUInt32 ezDescriptorSetPoolVulkan::GetPoolCount()
{
  return s_Pools.GetCount();
}

void ezDescriptorSetPoolVulkan::MarkPoolDirty(ezDescriptorSetPoolVulkan* pPool)
{
  EZ_LOCK(s_Mutex);
  s_DirtyPools.Insert(pPool);
}

void ezDescriptorSetPoolVulkan::BeginFrame()
{
  EZ_LOCK(s_Mutex);
  for (ezDescriptorSetPoolVulkan* pPool : s_DirtyPools)
  {
    if (pPool->ReclaimResources())
    {
      // #TODO_VULKAN: We should delete the pool to free resources.
    }
  }
  s_DirtyPools.Clear();
}

ezDescriptorSetPoolVulkan::ezDescriptorSetPoolVulkan(ezGALDeviceVulkan* pDevice, const ezBindGroupLayoutResourceUsageVulkan& resourceUsage)
  : m_pDevice(pDevice)
  , m_ResourceUsage(resourceUsage)
{
}

ezDescriptorSetPoolVulkan::~ezDescriptorSetPoolVulkan()
{
  ReclaimResources();
  EZ_ASSERT_DEBUG(m_uiTotalAllocations == 0, "A descriptor set has leaked, probably caused by a bind group not being freed before shutting down the GAL device");
  for (ezUniquePtr<DescriptorSubPool>& pool : m_SubPools)
  {
    m_pDevice->GetVulkanDevice().destroyDescriptorPool(pool->m_DescriptorPool);
  }
  m_SubPools.Clear();
}

vk::DescriptorSet ezDescriptorSetPoolVulkan::CreateDescriptorSet(ezGALBindGroupLayoutHandle hBindGroupLayout, ezDescriptorSetPoolVulkan::Allocation& out_Allocation)
{
  EZ_LOCK(m_Mutex);
  ezUInt32 uiPoolIndex = GetFreePoolIndex();
  DescriptorSubPool* pSubPool = m_SubPools[uiPoolIndex].Borrow();

  const ezGALBindGroupLayoutVulkan* pLayout = static_cast<const ezGALBindGroupLayoutVulkan*>(m_pDevice->GetBindGroupLayout(hBindGroupLayout));

  vk::DescriptorSet set;
  vk::DescriptorSetAllocateInfo allocateInfo;
  allocateInfo.pSetLayouts = &pLayout->GetDescriptorSetLayout();
  allocateInfo.descriptorPool = pSubPool->m_DescriptorPool;
  allocateInfo.descriptorSetCount = 1;

  VK_ASSERT_DEV(m_pDevice->GetVulkanDevice().allocateDescriptorSets(&allocateInfo, &set));

  ++m_uiTotalAllocations;
  ++pSubPool->m_uiAllocatedSets;

  out_Allocation.m_uiPoolIndex = uiPoolIndex;

  return set;
}

void ezDescriptorSetPoolVulkan::ReclaimDescriptorSet(vk::DescriptorSet descriptorSet, Allocation allocation)
{
  bool bMarkPoolDirty = false;
  {
    EZ_LOCK(m_Mutex);
    DescriptorSubPool* pPool = m_SubPools[allocation.m_uiPoolIndex].Borrow();
    pPool->m_Reclaim.PushBack(descriptorSet);
    if (!m_DirtySubPools.Contains(pPool))
    {
      m_DirtySubPools.Insert(pPool);
      bMarkPoolDirty = true;
    }
  }

  if (bMarkPoolDirty)
  {
    ezDescriptorSetPoolVulkan::MarkPoolDirty(this);
  }
}

ezUInt32 ezDescriptorSetPoolVulkan::GetFreePoolIndex()
{
  if (m_uiActivePool != ezInvalidIndex && m_SubPools[m_uiActivePool]->m_uiAllocatedSets < m_SubPools[m_uiActivePool]->m_uiPoolSize)
  {
    return m_uiActivePool;
  }

  for (ezInt32 poolIndex = (ezInt32)m_SubPools.GetCount() - 1; poolIndex >= 0; --poolIndex)
  {
    if (m_SubPools[poolIndex]->m_uiAllocatedSets < m_SubPools[poolIndex]->m_uiPoolSize)
    {
      m_uiActivePool = poolIndex;
      return m_uiActivePool;
    }
  }

  ezUniquePtr<DescriptorSubPool> pSubPool = EZ_DEFAULT_NEW(DescriptorSubPool);
  pSubPool->m_uiPoolSize = m_uiNextPoolSize;
  {
    // Create Vulkan descriptor pool
    ezHybridArray<vk::DescriptorPoolSize, ezGALShaderResourceType::COUNT> poolSizes;
    for (ezUInt32 i = 0; i < ezGALShaderResourceType::COUNT; ++i)
    {
      if (m_ResourceUsage.m_Usage[i] > 0)
        poolSizes.PushBack(vk::DescriptorPoolSize(ezConversionUtilsVulkan::GetDescriptorType((ezGALShaderResourceType::Enum)i), m_ResourceUsage.m_Usage[i] * m_uiNextPoolSize));
    }

    vk::DescriptorPoolCreateInfo poolCreateInfo;
    poolCreateInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    poolCreateInfo.maxSets = m_uiNextPoolSize;
    poolCreateInfo.poolSizeCount = poolSizes.GetCount();
    poolCreateInfo.pPoolSizes = poolSizes.GetData();

    VK_ASSERT_DEV(m_pDevice->GetVulkanDevice().createDescriptorPool(&poolCreateInfo, nullptr, &pSubPool->m_DescriptorPool));
  }
  m_SubPools.PushBack(std::move(pSubPool));
  m_uiActivePool = m_SubPools.GetCount() - 1;

  m_uiNextPoolSize *= 2;

  return m_uiActivePool;
}

bool ezDescriptorSetPoolVulkan::ReclaimResources()
{
  EZ_LOCK(m_Mutex);

  for (DescriptorSubPool* pSubPool : m_DirtySubPools)
  {
    const ezUInt32 uiReclaimCount = pSubPool->m_Reclaim.GetCount();
    m_pDevice->GetVulkanDevice().freeDescriptorSets(pSubPool->m_DescriptorPool, uiReclaimCount, pSubPool->m_Reclaim.GetData());
    pSubPool->m_uiAllocatedSets -= uiReclaimCount;
    m_uiTotalAllocations -= uiReclaimCount;
    pSubPool->m_Reclaim.Clear();
  }
  m_DirtySubPools.Clear();

  return m_uiTotalAllocations == 0;
}
