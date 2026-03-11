#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class ezGALDevice;

/// \brief Creates fallback resources in case the high-level renderer did not map a resource to a binding slot.
class EZ_RENDERERFOUNDATION_DLL ezGALRendererFallbackResources
{
public:
  static const ezGALBufferHandle GetFallbackBuffer(ezEnum<ezGALShaderResourceType> resourceType);
  static const ezGALTextureHandle GetFallbackTexture(ezEnum<ezGALShaderResourceType> resourceType, ezEnum<ezGALShaderTextureType> textureType, bool bDepth);

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

  static ezHashTable<Key, ezGALTextureHandle, KeyHash> s_TextureResourceViews;
  static ezHashTable<ezEnum<ezGALShaderResourceType>, ezGALBufferHandle, KeyHash> s_BufferResourceViews;

  static ezDynamicArray<ezGALBufferHandle> s_Buffers;
  static ezDynamicArray<ezGALTextureHandle> s_Textures;
};
