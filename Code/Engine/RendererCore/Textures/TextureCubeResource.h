#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Texture/Image/Image.h>
#include <RendererCore/Basics.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererFoundation/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

typedef ezTypedResourceHandle<class ezTextureCubeResource> ezTextureCubeResourceHandle;

/// \brief Use this descriptor in calls to ezResourceManager::CreateResource<ezTextureCubeResource> to create textures from data in memory.
struct ezTextureCubeResourceDescriptor
{
  ezTextureCubeResourceDescriptor()
  {
    m_uiQualityLevelsDiscardable = 0;
    m_uiQualityLevelsLoadable = 0;
  }

  /// Describes the texture format, etc.
  ezGALTextureCreationDescription m_DescGAL;
  ezGALSamplerStateCreationDescription m_SamplerDesc;

  /// How many quality levels can be discarded and reloaded. For created textures this can currently only be 0 or 1.
  ezUInt8 m_uiQualityLevelsDiscardable;

  /// How many additional quality levels can be loaded (typically from file).
  ezUInt8 m_uiQualityLevelsLoadable;

  /// One memory desc per (array * faces * mipmap) (in that order) (array is outer loop, mipmap is inner loop). Can be empty to not
  /// initialize data.
  ezArrayPtr<ezGALSystemMemoryDescription> m_InitialContent;
};

class EZ_RENDERERCORE_DLL ezTextureCubeResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureCubeResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezTextureCubeResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezTextureCubeResource, ezTextureCubeResourceDescriptor);

public:
  ezTextureCubeResource();

  EZ_ALWAYS_INLINE ezGALResourceFormat::Enum GetFormat() const { return m_Format; }
  EZ_ALWAYS_INLINE ezUInt32 GetWidthAndHeight() const { return m_uiWidthAndHeight; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  friend class ezRenderContext;

  const ezGALTextureHandle& GetGALTexture() const { return m_hGALTexture[m_uiLoadedTextures - 1]; }
  const ezGALSamplerStateHandle& GetGALSamplerState() const { return m_hSamplerState; }

private:
  ezUInt8 m_uiLoadedTextures;
  ezGALTextureHandle m_hGALTexture[2];
  ezUInt32 m_uiMemoryGPU[2];

  ezGALResourceFormat::Enum m_Format;
  ezUInt32 m_uiWidthAndHeight;

  ezGALSamplerStateHandle m_hSamplerState;
};

