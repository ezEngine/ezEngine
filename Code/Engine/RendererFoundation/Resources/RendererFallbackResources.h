#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class ezGALDevice;
class ezGALTextureResourceView;
class ezGALBufferResourceView;
class ezGALTextureUnorderedAccessView;
class ezGALBufferUnorderedAccessView;

/// \brief Creates fallback resources in case the high-level renderer did not map a resource to a descriptor slot.
class EZ_RENDERERFOUNDATION_DLL ezGALRendererFallbackResources
{
public:
  /// Returns a fallback resource for the given shader resource type.
  /// \param descriptorType The shader resource descriptor for which a compatible fallback resource is requested.
  /// \param textureType In case descriptorType is a texture, this specifies the texture type.
  /// \param bDepth Whether the shader resource is using a depth sampler.
  /// \return
  static const ezGALTextureResourceView* GetFallbackTextureResourceView(ezGALShaderResourceType::Enum descriptorType, ezGALShaderTextureType::Enum textureType, bool bDepth);
  static const ezGALBufferResourceView* GetFallbackBufferResourceView(ezGALShaderResourceType::Enum descriptorType);
  static const ezGALTextureUnorderedAccessView* GetFallbackTextureUnorderedAccessView(ezGALShaderResourceType::Enum descriptorType, ezGALShaderTextureType::Enum textureType);
  static const ezGALBufferUnorderedAccessView* GetFallbackBufferUnorderedAccessView(ezGALShaderResourceType::Enum descriptorType);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererFoundation, FallbackResources)
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
  static ezHashTable<Key, ezGALTextureUnorderedAccessViewHandle, KeyHash> m_TextureUAVs;
  static ezHashTable<ezEnum<ezGALShaderResourceType>, ezGALBufferUnorderedAccessViewHandle, KeyHash> m_BufferUAVs;

  static ezDynamicArray<ezGALBufferHandle> m_Buffers;
  static ezDynamicArray<ezGALTextureHandle> m_Textures;
};
