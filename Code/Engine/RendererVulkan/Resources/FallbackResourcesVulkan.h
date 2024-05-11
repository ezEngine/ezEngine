#pragma once
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

#include <vulkan/vulkan.hpp>

class ezGALDeviceVulkan;
class ezGALTextureResourceViewVulkan;
class ezGALBufferResourceViewVulkan;
class ezGALUnorderedAccessViewVulkan;

/// \brief Creates fallback resources in case the high-level renderer did not map a resource to a descriptor slot.
/// #TODO_VULKAN: Although the class has 'Vulkan' in the name, it could be made GAL agnostic by just returning the base class of the resource views and then it will work for any device type so it could be moved to RendererFoundation if needed for another GAL implementation.
class ezFallbackResourcesVulkan
{
public:
  /// Returns a fallback resource for the given shader resource type.
  /// \param descriptorType The shader resource descriptor for which a compatible fallback resource is requested.
  /// \param textureType In case descriptorType is a texture, this specifies the texture type.
  /// \param bDepth Whether the shader resource is using a depth sampler.
  /// \return
  static const ezGALTextureResourceViewVulkan* GetFallbackTextureResourceView(ezGALShaderResourceType::Enum descriptorType, ezGALShaderTextureType::Enum textureType, bool bDepth);
  static const ezGALBufferResourceViewVulkan* GetFallbackBufferResourceView(ezGALShaderResourceType::Enum descriptorType);
  static const ezGALUnorderedAccessViewVulkan* GetFallbackUnorderedAccessView(ezGALShaderResourceType::Enum descriptorType, ezGALShaderTextureType::Enum textureType);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererVulkan, FallbackResourcesVulkan)
  static void GALDeviceEventHandler(const ezGALDeviceEvent& e);
  static void Initialize();
  static void DeInitialize();

  static ezGALDevice* s_pDevice;
  static ezEventSubscriptionID s_EventID;

  struct Key
  {
    EZ_DECLARE_POD_TYPE();
    ezEnum<ezGALShaderResourceType> m_ResourceType;
    ezEnum<ezGALShaderTextureType> m_ezType;
    bool m_bDepth = false;
  };

  struct KeyHash
  {
    static ezUInt32 Hash(const Key& a);
    static bool Equal(const Key& a, const Key& b);

    static ezUInt32 Hash(const ezEnum<ezGALShaderResourceType>& a);
    static bool Equal(const ezEnum<ezGALShaderResourceType>& a, const ezEnum<ezGALShaderResourceType>& b);
  };

  static ezHashTable<Key, ezGALTextureResourceViewHandle, KeyHash> m_TextureResourceViews;
  static ezHashTable<ezEnum<ezGALShaderResourceType>, ezGALBufferResourceViewHandle, KeyHash> m_BufferResourceViews;
  static ezHashTable<Key, ezGALUnorderedAccessViewHandle, KeyHash> m_UAVs;

  static ezDynamicArray<ezGALBufferHandle> m_Buffers;
  static ezDynamicArray<ezGALTextureHandle> m_Textures;
};
