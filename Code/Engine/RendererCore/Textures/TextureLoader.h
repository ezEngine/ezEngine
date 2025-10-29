#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <Texture/Image/Image.h>
#include <Texture/ezTexFormat/ezTexFormat.h>

/// Resource loader for texture resources.
///
/// Loads textures from .ezTex files which contain compressed texture data and metadata.
class EZ_RENDERERCORE_DLL ezTextureResourceLoader : public ezResourceTypeLoader
{
public:
  /// Data structure for loaded texture information.
  struct LoadedData
  {
    LoadedData()
      : m_Reader(&m_Storage)
    {
    }

    ezContiguousMemoryStreamStorage m_Storage;
    ezMemoryStreamReader m_Reader;
    ezImage m_Image;

    bool m_bIsFallback = false;
    ezTexFormat m_TexFormat; ///< Texture format information from the .ezTex file.
  };

  virtual ezResourceLoadData OpenDataStream(const ezResource* pResource) override;
  virtual void CloseDataStream(const ezResource* pResource, const ezResourceLoadData& loaderData) override;
  virtual bool IsResourceOutdated(const ezResource* pResource) const override;

  /// Loads texture data from a .ezTex file stream.
  static ezResult LoadTexFile(ezStreamReader& inout_stream, LoadedData& ref_data);

  /// Writes texture data to a stream in the loader's internal format.
  static void WriteTextureLoadStream(ezStreamWriter& inout_stream, const LoadedData& data);
};
