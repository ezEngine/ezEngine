#pragma once

#include <RendererCore/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Texture/Image/Image.h>
#include <RendererFoundation/Basics.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <Texture/ezTexFormat/ezTexFormat.h>

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
    ezTexFormat m_TexFormat;
  };

  virtual ezResourceLoadData OpenDataStream(const ezResource* pResource) override;
  virtual void CloseDataStream(const ezResource* pResource, const ezResourceLoadData& LoaderData) override;
  virtual bool IsResourceOutdated(const ezResource* pResource) const override;

  static ezResult LoadTexFile(ezStreamReader& stream, LoadedData& data);
  static void WriteTextureLoadStream(ezStreamWriter& stream, const LoadedData& data);
};


