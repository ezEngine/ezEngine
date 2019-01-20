#pragma once

#include <RendererCore/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Image/Image.h>
#include <RendererFoundation/Basics.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>

class EZ_RENDERERCORE_DLL ezTextureResourceLoader : public ezResourceTypeLoader
{
public:

  struct LoadedData
  {
    LoadedData() : m_Reader(&m_Storage) { }

    ezMemoryStreamStorage m_Storage;
    ezMemoryStreamReader m_Reader;
    ezImage m_Image;

    bool m_bIsFallback = false;
    bool m_bSRGB = false;
    ezInt16 m_iRenderTargetResolutionX = 0;
    ezInt16 m_iRenderTargetResolutionY = 0;
    float m_fResolutionScale = 1.0f;
    int m_GalRenderTargetFormat = 0;
    ezEnum<ezGALTextureAddressMode> m_addressModeU = ezGALTextureAddressMode::Wrap;
    ezEnum<ezGALTextureAddressMode> m_addressModeV = ezGALTextureAddressMode::Wrap;
    ezEnum<ezGALTextureAddressMode> m_addressModeW = ezGALTextureAddressMode::Wrap;
    ezEnum<ezTextureFilterSetting> m_textureFilter = ezTextureFilterSetting::Default;
  };

  virtual ezResourceLoadData OpenDataStream(const ezResource* pResource) override;
  virtual void CloseDataStream(const ezResource* pResource, const ezResourceLoadData& LoaderData) override;
  virtual bool IsResourceOutdated(const ezResource* pResource) const override;

  static ezResult LoadTexFile(ezStreamReader& stream, LoadedData& data);
  static void WriteTextureLoadStream(ezStreamWriter& stream, const LoadedData& data);
};


