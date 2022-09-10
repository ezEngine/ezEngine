#pragma once
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

#include <vulkan/vulkan.hpp>

class ezGALDeviceVulkan;
class ezGALResourceViewVulkan;
class ezGALUnorderedAccessViewVulkan;

/// \brief Creates fallback resources in case the high-level renderer did not map a resource to a descriptor slot.
class ezFallbackResourcesVulkan
{
public:
  static void Initialize(ezGALDeviceVulkan* pDevice);
  static void DeInitialize();

  static const ezGALResourceViewVulkan* GetFallbackResourceView(vk::DescriptorType descriptorType, ezShaderResourceType::Enum ezType, bool bDepth);
  static const ezGALUnorderedAccessViewVulkan* GetFallbackUnorderedAccessView(vk::DescriptorType descriptorType, ezShaderResourceType::Enum ezType);

private:
  static void GALDeviceEventHandler(const ezGALDeviceEvent& e);

  static ezGALDeviceVulkan* s_pDevice;
  static ezEventSubscriptionID s_EventID;

  struct Key
  {
    EZ_DECLARE_POD_TYPE();
    vk::DescriptorType m_descriptorType;
    ezShaderResourceType::Enum m_ezType;
    bool m_bDepth = false;
  };

  struct KeyHash
  {
    static ezUInt32 Hash(const Key& a);
    static bool Equal(const Key& a, const Key& b);
  };

  static ezHashTable<Key, ezGALResourceViewHandle, KeyHash> m_ResourceViews;
  static ezHashTable<Key, ezGALUnorderedAccessViewHandle, KeyHash> m_UAVs;

  static ezDynamicArray<ezGALBufferHandle> m_Buffers;
  static ezDynamicArray<ezGALTextureHandle> m_Textures;
};
