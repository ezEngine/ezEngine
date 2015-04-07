#pragma once

#include <RendererCore/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/IO/MemoryStream.h>
#include <CoreUtils/Image/Image.h>
#include <RendererFoundation/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

typedef ezResourceHandle<class ezTextureResource> ezTextureResourceHandle;

/// \brief Use this descriptor in calls to ezResourceManager::CreateResource<ezTextureResource> to create textures from data in memory.
struct ezTextureResourceDescriptor
{
  ezTextureResourceDescriptor()
  {
    m_uiQualityLevelsDiscardable = 0;
    m_uiQualityLevelsLoadable = 0;
  }

  /// Describes the texture format, etc.
  ezGALTextureCreationDescription m_DescGAL;

  /// How many quality levels can be discarded and reloaded. For created textures this can currently only be 0 or 1.
  ezUInt8 m_uiQualityLevelsDiscardable;

  /// How many additional quality levels can be loaded (typically from file).
  ezUInt8 m_uiQualityLevelsLoadable;

  /// One memory desc per (array * faces * mipmap) (in that order) (array is outer loop, mipmap is inner loop). Can be empty to not initialize data.
  ezArrayPtr<ezGALSystemMemoryDescription> m_InitialContent;
};

class EZ_RENDERERCORE_DLL ezTextureResource : public ezResource<ezTextureResource, ezTextureResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureResource);

public:
  ezTextureResource();

  /// \brief If enabled, textures are always loaded to full quality immediately. Mostly necessary for image comparison unit tests.
  static bool s_bForceFullQualityAlways;

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReaderBase* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(const ezTextureResourceDescriptor& descriptor) override;

private:
  friend class ezRenderContext;

  const ezGALResourceViewHandle& GetGALTextureView() const { return m_hGALTexView[m_uiLoadedTextures - 1]; }
  const ezGALTextureHandle& GetGALTexture() const { return m_hGALTexture[m_uiLoadedTextures - 1]; }
  const ezGALSamplerStateHandle& GetGALSamplerState() const { return m_hSamplerState; }

private:
  ezUInt8 m_uiLoadedTextures;
  ezGALTextureHandle m_hGALTexture[2];
  ezGALResourceViewHandle m_hGALTexView[2];
  ezUInt32 m_uiMemoryGPU[2];
  ezGALSamplerStateHandle m_hSamplerState; // HACK
};


class EZ_RENDERERCORE_DLL ezTextureResourceLoader : public ezResourceTypeLoader
{
public:

  struct LoadedData
  {
    LoadedData() : m_Reader(&m_Storage) { }

    ezMemoryStreamStorage m_Storage;
    ezMemoryStreamReader m_Reader;
    ezImage m_Image;
  };

  virtual ezResourceLoadData OpenDataStream(const ezResourceBase* pResource) override;
  virtual void CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData) override;
  virtual bool IsResourceOutdated(const ezResourceBase* pResource) const override;
};


