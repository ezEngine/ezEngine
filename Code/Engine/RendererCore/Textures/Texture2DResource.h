#pragma once

#include <RendererCore/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Image/Image.h>
#include <RendererFoundation/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>

typedef ezTypedResourceHandle<class ezTexture2DResource> ezTexture2DResourceHandle;

/// \brief Use this descriptor in calls to ezResourceManager::CreateResource<ezTexture2DResource> to create textures from data in memory.
struct ezTexture2DResourceDescriptor
{
  ezTexture2DResourceDescriptor()
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

  /// One memory desc per (array * faces * mipmap) (in that order) (array is outer loop, mipmap is inner loop). Can be empty to not initialize data.
  ezArrayPtr<ezGALSystemMemoryDescription> m_InitialContent;
};

class EZ_RENDERERCORE_DLL ezTexture2DResource : public ezResource<ezTexture2DResource, ezTexture2DResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTexture2DResource, ezResourceBase);

public:
  ezTexture2DResource();

  EZ_ALWAYS_INLINE ezGALResourceFormat::Enum GetFormat() const { return m_Format; }
  EZ_ALWAYS_INLINE ezUInt32 GetWidth() const { return m_uiWidth; }
  EZ_ALWAYS_INLINE ezUInt32 GetHeight() const { return m_uiHeight; }
  EZ_ALWAYS_INLINE ezGALTextureType::Enum GetType() const { return m_Type; }

  static void FillOutDescriptor(ezTexture2DResourceDescriptor& td, const ezImage* pImage, bool bSRGB, ezUInt32 uiNumMipLevels, ezUInt32& out_MemoryUsed, ezHybridArray<ezGALSystemMemoryDescription, 32>& initData);

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(const ezTexture2DResourceDescriptor& descriptor) override;

private:
  friend class ezRenderContext;

  const ezGALTextureHandle& GetGALTexture() const { return m_hGALTexture[m_uiLoadedTextures - 1]; }
  const ezGALSamplerStateHandle& GetGALSamplerState() const { return m_hSamplerState; }

private:
  ezUInt8 m_uiLoadedTextures;
  ezGALTextureHandle m_hGALTexture[2];
  ezUInt32 m_uiMemoryGPU[2];

  ezGALTextureType::Enum m_Type;
  ezGALResourceFormat::Enum m_Format;
  ezUInt32 m_uiWidth;
  ezUInt32 m_uiHeight;

  ezGALSamplerStateHandle m_hSamplerState;
};



