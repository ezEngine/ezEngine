#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Texture/Image/Image.h>
#include <RendererCore/Basics.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererFoundation/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

typedef ezTypedResourceHandle<class ezTexture2DResource> ezTexture2DResourceHandle;

/// \brief Use this descriptor in calls to ezResourceManager::CreateResource<ezTexture2DResource> to create textures from data in memory.
struct EZ_RENDERERCORE_DLL ezTexture2DResourceDescriptor
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

class EZ_RENDERERCORE_DLL ezTexture2DResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTexture2DResource, ezResource);

  EZ_RESOURCE_DECLARE_COMMON_CODE(ezTexture2DResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezTexture2DResource, ezTexture2DResourceDescriptor);

public:
  ezTexture2DResource();

  EZ_ALWAYS_INLINE ezGALResourceFormat::Enum GetFormat() const { return m_Format; }
  EZ_ALWAYS_INLINE ezUInt32 GetWidth() const { return m_uiWidth; }
  EZ_ALWAYS_INLINE ezUInt32 GetHeight() const { return m_uiHeight; }
  EZ_ALWAYS_INLINE ezGALTextureType::Enum GetType() const { return m_Type; }

  static void FillOutDescriptor(ezTexture2DResourceDescriptor& td, const ezImage* pImage, bool bSRGB, ezUInt32 uiNumMipLevels,
                                ezUInt32& out_MemoryUsed, ezHybridArray<ezGALSystemMemoryDescription, 32>& initData);

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

protected:
  friend class ezRenderContext;

  const ezGALTextureHandle& GetGALTexture() const { return m_hGALTexture[m_uiLoadedTextures - 1]; }
  const ezGALSamplerStateHandle& GetGALSamplerState() const { return m_hSamplerState; }

protected:
  ezUInt8 m_uiLoadedTextures;
  ezGALTextureHandle m_hGALTexture[2];
  ezUInt32 m_uiMemoryGPU[2];

  ezGALTextureType::Enum m_Type;
  ezGALResourceFormat::Enum m_Format;
  ezUInt32 m_uiWidth;
  ezUInt32 m_uiHeight;

  ezGALSamplerStateHandle m_hSamplerState;
};

//////////////////////////////////////////////////////////////////////////

typedef ezTypedResourceHandle<class ezRenderToTexture2DResource> ezRenderToTexture2DResourceHandle;

struct EZ_RENDERERCORE_DLL ezRenderToTexture2DResourceDescriptor
{
  ezUInt32 m_uiWidth = 0;
  ezUInt32 m_uiHeight = 0;
  ezEnum<ezGALMSAASampleCount> m_SampleCount;
  ezEnum<ezGALResourceFormat> m_Format;
  ezGALSamplerStateCreationDescription m_SamplerDesc;
  ezArrayPtr<ezGALSystemMemoryDescription> m_InitialContent;
};

class EZ_RENDERERCORE_DLL ezRenderToTexture2DResource : public ezTexture2DResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderToTexture2DResource, ezTexture2DResource);

  EZ_RESOURCE_DECLARE_COMMON_CODE(ezRenderToTexture2DResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezRenderToTexture2DResource, ezRenderToTexture2DResourceDescriptor);

public:
  ezGALRenderTargetViewHandle GetRenderTargetView() const;
  void AddRenderView(ezViewHandle hView);
  void RemoveRenderView(ezViewHandle hView);
  const ezDynamicArray<ezViewHandle>& GetAllRenderViews() const;

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

protected:

  // other views that use this texture as their target
  ezDynamicArray<ezViewHandle> m_RenderViews;

};
