#pragma once

#include <RendererCore/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Image/Image.h>

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
    ezEnum<ezGALTextureAddressMode> m_addressModeU = ezGALTextureAddressMode::Wrap;
    ezEnum<ezGALTextureAddressMode> m_addressModeV = ezGALTextureAddressMode::Wrap;
    ezEnum<ezGALTextureAddressMode> m_addressModeW = ezGALTextureAddressMode::Wrap;
    ezEnum<ezTextureFilterSetting> m_textureFilter = ezTextureFilterSetting::Default;
  };

  virtual ezResourceLoadData OpenDataStream(const ezResourceBase* pResource) override;
  virtual void CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData) override;
  virtual bool IsResourceOutdated(const ezResourceBase* pResource) const override;

  static ezResult LoadTexFile(ezStreamReader& stream, LoadedData& data);
  static void WriteTextureLoadStream(ezStreamWriter& stream, const LoadedData& data);
};


