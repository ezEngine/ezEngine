#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/IO/MemoryStream.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>

class ezImage;

typedef ezTypedResourceHandle<class ezTexture3DResource> ezTexture3DResourceHandle;

/// \brief Use this descriptor in calls to ezResourceManager::CreateResource<ezTexture3DResource> to create textures from data in memory.
struct EZ_RENDERERCORE_DLL ezTexture3DResourceDescriptor
{
  /// Describes the texture format, etc.
  ezGALTextureCreationDescription m_DescGAL;
  ezGALSamplerStateCreationDescription m_SamplerDesc;

  /// How many quality levels can be discarded and reloaded. For created textures this can currently only be 0 or 1.
  ezUInt8 m_uiQualityLevelsDiscardable = 0;

  /// How many additional quality levels can be loaded (typically from file).
  ezUInt8 m_uiQualityLevelsLoadable = 0;

  /// One memory desc per (array * faces * mipmap) (in that order) (array is outer loop, mipmap is inner loop). Can be empty to not
  /// initialize data.
  ezArrayPtr<ezGALSystemMemoryDescription> m_InitialContent;
};

class EZ_RENDERERCORE_DLL ezTexture3DResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTexture3DResource, ezResource);

  EZ_RESOURCE_DECLARE_COMMON_CODE(ezTexture3DResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezTexture3DResource, ezTexture3DResourceDescriptor);

public:
  ezTexture3DResource();

  EZ_ALWAYS_INLINE ezGALResourceFormat::Enum GetFormat() const { return m_Format; }
  EZ_ALWAYS_INLINE ezUInt32 GetWidth() const { return m_uiWidth; }
  EZ_ALWAYS_INLINE ezUInt32 GetHeight() const { return m_uiHeight; }
  EZ_ALWAYS_INLINE ezUInt32 GetDepth() const { return m_uiDepth; }
  EZ_ALWAYS_INLINE ezGALTextureType::Enum GetType() const { return m_Type; }

  static void FillOutDescriptor(ezTexture3DResourceDescriptor& td, const ezImage* pImage, bool bSRGB, ezUInt32 uiNumMipLevels,
                                ezUInt32& out_MemoryUsed, ezHybridArray<ezGALSystemMemoryDescription, 32>& initData);

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

protected:
  friend class ezRenderContext;

  ezTexture3DResource(DoUpdate ResourceUpdateThread);

  const ezGALTextureHandle& GetGALTexture() const { return m_hGALTexture[m_uiLoadedTextures - 1]; }
  const ezGALSamplerStateHandle& GetGALSamplerState() const { return m_hSamplerState; }

protected:
  ezUInt8 m_uiLoadedTextures = 0;
  ezGALTextureHandle m_hGALTexture[2];
  ezUInt32 m_uiMemoryGPU[2] = { 0, 0 };

  ezGALTextureType::Enum m_Type = ezGALTextureType::Invalid;
  ezGALResourceFormat::Enum m_Format = ezGALResourceFormat::Invalid;
  ezUInt32 m_uiWidth = 0;
  ezUInt32 m_uiHeight = 0;
  ezUInt32 m_uiDepth = 0;

  ezGALSamplerStateHandle m_hSamplerState;
};
