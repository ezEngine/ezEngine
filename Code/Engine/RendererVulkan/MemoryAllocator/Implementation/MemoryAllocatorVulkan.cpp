#include <RendererVulkan/RendererVulkanPCH.h>


VKAPI_ATTR void VKAPI_CALL vkGetDeviceBufferMemoryRequirements(
  VkDevice device,
  const VkDeviceBufferMemoryRequirements* pInfo,
  VkMemoryRequirements2* pMemoryRequirements)
{
  EZ_REPORT_FAILURE("FIXME: Added to prevent the error: The procedure entry point vkGetDeviceBufferMemoryRequirements could not be located in the dynamic link library ezRendererVulkan.dll.");
}

#include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#include <Foundation/Types/UniquePtr.h>

#define VMA_VULKAN_VERSION 1001000
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_STATS_STRING_ENABLED 1


//
// #define VMA_DEBUG_LOG(format, ...)   \
//  do                                 \
//  {                                  \
//    ezStringBuilder tmp;             \
//    tmp.Printf(format, __VA_ARGS__); \
//    ezLog::Error("{}", tmp);         \
//  } while (false)

#include <RendererVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>

#define VMA_IMPLEMENTATION

#ifndef VA_IGNORE_THIS_FILE
#  define VA_INCLUDE_HIDDEN <vma/vk_mem_alloc.h>
#else
#  define VA_INCLUDE_HIDDEN ""
#endif

#include VA_INCLUDE_HIDDEN

static_assert(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT == (ezUInt32)ezVulkanAllocationCreateFlags::DedicatedMemory);
static_assert(VMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT == (ezUInt32)ezVulkanAllocationCreateFlags::NeverAllocate);
static_assert(VMA_ALLOCATION_CREATE_MAPPED_BIT == (ezUInt32)ezVulkanAllocationCreateFlags::Mapped);
static_assert(VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT == (ezUInt32)ezVulkanAllocationCreateFlags::CanAlias);
static_assert(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT == (ezUInt32)ezVulkanAllocationCreateFlags::HostAccessSequentialWrite);
static_assert(VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT == (ezUInt32)ezVulkanAllocationCreateFlags::HostAccessRandom);
static_assert(VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT == (ezUInt32)ezVulkanAllocationCreateFlags::AllowTransferInstead);
static_assert(VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT == (ezUInt32)ezVulkanAllocationCreateFlags::StrategyMinMemory);
static_assert(VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT == (ezUInt32)ezVulkanAllocationCreateFlags::StrategyMinTime);

static_assert(VMA_MEMORY_USAGE_UNKNOWN == (ezUInt32)ezVulkanMemoryUsage::Unknown);
static_assert(VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED == (ezUInt32)ezVulkanMemoryUsage::GpuLazilyAllocated);
static_assert(VMA_MEMORY_USAGE_AUTO == (ezUInt32)ezVulkanMemoryUsage::Auto);
static_assert(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE == (ezUInt32)ezVulkanMemoryUsage::AutoPreferDevice);
static_assert(VMA_MEMORY_USAGE_AUTO_PREFER_HOST == (ezUInt32)ezVulkanMemoryUsage::AutoPreferHost);

static_assert(sizeof(ezVulkanAllocation) == sizeof(VmaAllocation));

static_assert(sizeof(ezVulkanAllocationInfo) == sizeof(VmaAllocationInfo));

EZ_DEFINE_AS_POD_TYPE(VkExportMemoryAllocateInfo);

namespace
{
  struct ExportedSharedPool
  {
    VmaPool m_pool = nullptr;
    ezUniquePtr<vk::ExportMemoryAllocateInfo> m_exportInfo; // must outlive the pool and remain at the same address.
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    ezUniquePtr<vk::ExportMemoryWin32HandleInfoKHR> m_exportInfoWin32;
#endif
  };
} // namespace

struct ezMemoryAllocatorVulkan::Impl
{
  VmaAllocator m_allocator;
  ezMutex m_exportedSharedPoolsMutex;
  ezHashTable<uint32_t, ExportedSharedPool> m_exportedSharedPools;
};

ezMemoryAllocatorVulkan::Impl* ezMemoryAllocatorVulkan::m_pImpl = nullptr;

vk::Result ezMemoryAllocatorVulkan::Initialize(vk::PhysicalDevice physicalDevice, vk::Device device, vk::Instance instance)
{
  EZ_ASSERT_DEV(m_pImpl == nullptr, "ezMemoryAllocatorVulkan::Initialize was already called");
  m_pImpl = EZ_DEFAULT_NEW(Impl);

  VmaVulkanFunctions vulkanFunctions = {};
  vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
  vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

  VmaAllocatorCreateInfo allocatorCreateInfo = {};
  allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_1;
  allocatorCreateInfo.physicalDevice = physicalDevice;
  allocatorCreateInfo.device = device;
  allocatorCreateInfo.instance = instance;
  allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

  vk::Result res = (vk::Result)vmaCreateAllocator(&allocatorCreateInfo, &m_pImpl->m_allocator);
  if (res != vk::Result::eSuccess)
  {
    EZ_DEFAULT_DELETE(m_pImpl);
  }

  return res;
}

void ezMemoryAllocatorVulkan::DeInitialize()
{
  EZ_ASSERT_DEV(m_pImpl != nullptr, "ezMemoryAllocatorVulkan is not initialized.");

  for (auto it : m_pImpl->m_exportedSharedPools)
  {
    vmaDestroyPool(m_pImpl->m_allocator, it.Value().m_pool);
  }
  m_pImpl->m_exportedSharedPools.Clear();

  // Uncomment below to debug leaks in VMA.

  char* pStats = nullptr;
  vmaBuildStatsString(m_pImpl->m_allocator, &pStats, true);

  vmaDestroyAllocator(m_pImpl->m_allocator);
  EZ_DEFAULT_DELETE(m_pImpl);
}

vk::Result ezMemoryAllocatorVulkan::CreateImage(const vk::ImageCreateInfo& imageCreateInfo, const ezVulkanAllocationCreateInfo& allocationCreateInfo, vk::Image& out_image, ezVulkanAllocation& out_alloc, ezVulkanAllocationInfo* pAllocInfo)
{
  VmaAllocationCreateInfo allocCreateInfo = {};
  allocCreateInfo.usage = (VmaMemoryUsage)allocationCreateInfo.m_usage.GetValue();
  allocCreateInfo.flags = allocationCreateInfo.m_flags.GetValue() | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
  allocCreateInfo.pUserData = (void*)allocationCreateInfo.m_pUserData;

  if (allocationCreateInfo.m_bExportSharedAllocation)
  {
    allocCreateInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    EZ_LOCK(m_pImpl->m_exportedSharedPoolsMutex);

    uint32_t memoryTypeIndex = 0;
    if (auto res = vmaFindMemoryTypeIndexForImageInfo(m_pImpl->m_allocator, reinterpret_cast<const VkImageCreateInfo*>(&imageCreateInfo), &allocCreateInfo, &memoryTypeIndex); res != VK_SUCCESS)
    {
      return (vk::Result)res;
    }

    ExportedSharedPool* pool = m_pImpl->m_exportedSharedPools.GetValue(memoryTypeIndex);
    if (pool == nullptr)
    {
      ExportedSharedPool newPool;
      {
        newPool.m_exportInfo = EZ_DEFAULT_NEW(vk::ExportMemoryAllocateInfo);
        vk::ExportMemoryAllocateInfo& exportInfo = *newPool.m_exportInfo.Borrow();
#if EZ_ENABLED(EZ_PLATFORM_LINUX)
        exportInfo.handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd;
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS)
        newPool.m_exportInfoWin32 = EZ_DEFAULT_NEW(vk::ExportMemoryWin32HandleInfoKHR);
        vk::ExportMemoryWin32HandleInfoKHR& exportInfoWin = *newPool.m_exportInfoWin32.Borrow();
        exportInfoWin.dwAccess = GENERIC_ALL;

        exportInfo.handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32;
        exportInfo.pNext = &exportInfoWin;
#else
        EZ_ASSERT_NOT_IMPLEMENTED
#endif
      }

      VmaPoolCreateInfo poolCreateInfo = {};
      poolCreateInfo.memoryTypeIndex = memoryTypeIndex;
      poolCreateInfo.pMemoryAllocateNext = newPool.m_exportInfo.Borrow();

      if (auto res = vmaCreatePool(m_pImpl->m_allocator, &poolCreateInfo, &newPool.m_pool); res != VK_SUCCESS)
      {
        return (vk::Result)res;
      }
      m_pImpl->m_exportedSharedPools.Insert(memoryTypeIndex, std::move(newPool));
      pool = m_pImpl->m_exportedSharedPools.GetValue(memoryTypeIndex);
    }

    allocCreateInfo.pool = pool->m_pool;
  }

  return (vk::Result)vmaCreateImage(m_pImpl->m_allocator, reinterpret_cast<const VkImageCreateInfo*>(&imageCreateInfo), &allocCreateInfo, reinterpret_cast<VkImage*>(&out_image), reinterpret_cast<VmaAllocation*>(&out_alloc), reinterpret_cast<VmaAllocationInfo*>(pAllocInfo));
}

void ezMemoryAllocatorVulkan::DestroyImage(vk::Image& image, ezVulkanAllocation& alloc)
{
  vmaSetAllocationUserData(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), nullptr);
  vmaDestroyImage(m_pImpl->m_allocator, reinterpret_cast<VkImage&>(image), reinterpret_cast<VmaAllocation&>(alloc));
  image = nullptr;
  alloc = nullptr;
}

vk::Result ezMemoryAllocatorVulkan::CreateBuffer(const vk::BufferCreateInfo& bufferCreateInfo, const ezVulkanAllocationCreateInfo& allocationCreateInfo, vk::Buffer& out_buffer, ezVulkanAllocation& out_alloc, ezVulkanAllocationInfo* pAllocInfo)
{
  VmaAllocationCreateInfo allocCreateInfo = {};
  allocCreateInfo.usage = (VmaMemoryUsage)allocationCreateInfo.m_usage.GetValue();
  allocCreateInfo.flags = allocationCreateInfo.m_flags.GetValue() | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
  allocCreateInfo.pUserData = (void*)allocationCreateInfo.m_pUserData;

  return (vk::Result)vmaCreateBuffer(m_pImpl->m_allocator, reinterpret_cast<const VkBufferCreateInfo*>(&bufferCreateInfo), &allocCreateInfo, reinterpret_cast<VkBuffer*>(&out_buffer), reinterpret_cast<VmaAllocation*>(&out_alloc), reinterpret_cast<VmaAllocationInfo*>(pAllocInfo));
}

void ezMemoryAllocatorVulkan::DestroyBuffer(vk::Buffer& buffer, ezVulkanAllocation& alloc)
{
  vmaSetAllocationUserData(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), nullptr);
  vmaDestroyBuffer(m_pImpl->m_allocator, reinterpret_cast<VkBuffer&>(buffer), reinterpret_cast<VmaAllocation&>(alloc));
  buffer = nullptr;
  alloc = nullptr;
}

ezVulkanAllocationInfo ezMemoryAllocatorVulkan::GetAllocationInfo(ezVulkanAllocation alloc)
{
  VmaAllocationInfo info;
  vmaGetAllocationInfo(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), &info);

  return reinterpret_cast<ezVulkanAllocationInfo&>(info);
}

vk::MemoryPropertyFlags ezMemoryAllocatorVulkan::GetAllocationFlags(ezVulkanAllocation alloc)
{
  VkMemoryPropertyFlags memPropFlags;
  vmaGetAllocationMemoryProperties(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), &memPropFlags);
  return reinterpret_cast<vk::MemoryPropertyFlags&>(memPropFlags);
}

void ezMemoryAllocatorVulkan::SetAllocationUserData(ezVulkanAllocation alloc, const char* pUserData)
{
  vmaSetAllocationUserData(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), (void*)pUserData);
}

vk::Result ezMemoryAllocatorVulkan::MapMemory(ezVulkanAllocation alloc, void** pData)
{
  return (vk::Result)vmaMapMemory(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), pData);
}

void ezMemoryAllocatorVulkan::UnmapMemory(ezVulkanAllocation alloc)
{
  vmaUnmapMemory(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc));
}

vk::Result ezMemoryAllocatorVulkan::FlushAllocation(ezVulkanAllocation alloc, vk::DeviceSize offset, vk::DeviceSize size)
{
  return (vk::Result)vmaFlushAllocation(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), offset, size);
}

vk::Result ezMemoryAllocatorVulkan::InvalidateAllocation(ezVulkanAllocation alloc, vk::DeviceSize offset, vk::DeviceSize size)
{
  return (vk::Result)vmaInvalidateAllocation(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), offset, size);
}
