#include <RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/ImageConversion.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererCore/Textures/TextureLoader.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <Texture/ezTexFormat/ezTexFormat.h>

static ezTextureResourceLoader s_TextureResourceLoader;

ezCVarFloat CVarTextureLoadingDelay("r_TextureLoadDelay", 0.0f, ezCVarFlags::Save, "Artificial texture loading slowdown");

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, TextureResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezResourceManager::SetResourceTypeLoader<ezTexture2DResource>(&s_TextureResourceLoader);
    ezResourceManager::SetResourceTypeLoader<ezTextureCubeResource>(&s_TextureResourceLoader);
    ezResourceManager::SetResourceTypeLoader<ezRenderToTexture2DResource>(&s_TextureResourceLoader);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezResourceManager::SetResourceTypeLoader<ezTexture2DResource>(nullptr);
    ezResourceManager::SetResourceTypeLoader<ezTextureCubeResource>(nullptr);
    ezResourceManager::SetResourceTypeLoader<ezRenderToTexture2DResource>(nullptr);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezResourceLoadData ezTextureResourceLoader::OpenDataStream(const ezResource* pResource)
{
  LoadedData* pData = EZ_DEFAULT_NEW(LoadedData);

  ezResourceLoadData res;

  // Solid Color Textures
  if (ezPathUtils::HasExtension(pResource->GetResourceID(), "color"))
  {
    ezStringBuilder sName = pResource->GetResourceID();
    sName.RemoveFileExtension();

    bool bValidColor = false;
    const ezColorGammaUB color = ezConversionUtils::GetColorByName(sName, &bValidColor);

    if (!bValidColor)
    {
      ezLog::Error("'{0}' is not a valid color name. Using 'RebeccaPurple' as fallback.", sName);
    }

    pData->m_TexFormat.m_bSRGB = true;

    ezImageHeader header;
    header.SetWidth(4);
    header.SetHeight(4);
    header.SetDepth(1);
    header.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM_SRGB);
    header.SetNumMipLevels(1);
    header.SetNumFaces(1);
    pData->m_Image.ResetAndAlloc(header);
    ezUInt8* pPixels = pData->m_Image.GetPixelPointer<ezUInt8>();

    for (ezUInt32 px = 0; px < 4 * 4 * 4; px += 4)
    {
      pPixels[px + 0] = color.r;
      pPixels[px + 1] = color.g;
      pPixels[px + 2] = color.b;
      pPixels[px + 3] = color.a;
    }
  }
  else
  {
    ezFileReader File;
    if (File.Open(pResource->GetResourceID().GetData()).Failed())
      return res;

    const ezStringBuilder sAbsolutePath = File.GetFilePathAbsolute();
    res.m_sResourceDescription = File.GetFilePathRelative().GetData();

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
    {
      ezFileStats stat;
      if (ezFileSystem::GetFileStats(sAbsolutePath, stat).Succeeded())
      {
        res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
      }
    }
#endif

    /// In case this is not a proper asset (ezTextureXX format), this is a hack to get the SRGB information for the texture
    const ezStringBuilder sName = ezPathUtils::GetFileName(sAbsolutePath);
    pData->m_TexFormat.m_bSRGB = (sName.EndsWith_NoCase("_D") || sName.EndsWith_NoCase("_SRGB") || sName.EndsWith_NoCase("_diff"));

    if (sAbsolutePath.HasExtension("ezTexture2D") || sAbsolutePath.HasExtension("ezTextureCube") ||
        sAbsolutePath.HasExtension("ezRenderTarget"))
    {
      if (LoadTexFile(File, *pData).Failed())
        return res;
    }
    else
    {
      // read whatever format, as long as ezImage supports it
      File.Close();

      if (pData->m_Image.LoadFrom(pResource->GetResourceID()).Failed())
        return res;

      if (pData->m_Image.GetImageFormat() == ezImageFormat::B8G8R8_UNORM)
      {
        /// \todo A conversion to B8G8R8X8_UNORM currently fails

        ezLog::Warning("Texture resource uses inefficient BGR format, converting to BGRX: '{0}'", sAbsolutePath);
        if (ezImageConversion::Convert(pData->m_Image, pData->m_Image, ezImageFormat::B8G8R8A8_UNORM).Failed())
          return res;
      }
    }
  }

  ezMemoryStreamWriter w(&pData->m_Storage);

  WriteTextureLoadStream(w, *pData);

  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  if (CVarTextureLoadingDelay > 0)
  {
    ezThreadUtils::Sleep(ezTime::Seconds(CVarTextureLoadingDelay));
  }

  return res;
}

void ezTextureResourceLoader::CloseDataStream(const ezResource* pResource, const ezResourceLoadData& LoaderData)
{
  LoadedData* pData = (LoadedData*)LoaderData.m_pCustomLoaderData;

  EZ_DEFAULT_DELETE(pData);
}

bool ezTextureResourceLoader::IsResourceOutdated(const ezResource* pResource) const
{
  // solid color textures are never outdated
  if (ezPathUtils::HasExtension(pResource->GetResourceID(), "color"))
    return false;

  // don't try to reload a file that cannot be found
  ezStringBuilder sAbs;
  if (ezFileSystem::ResolvePath(pResource->GetResourceID(), &sAbs, nullptr).Failed())
    return false;

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)

  if (pResource->GetLoadedFileModificationTime().IsValid())
  {
    ezFileStats stat;
    if (ezFileSystem::GetFileStats(sAbs, stat).Failed())
      return false;

    return !stat.m_LastModificationTime.Compare(pResource->GetLoadedFileModificationTime(), ezTimestamp::CompareMode::FileTimeEqual);
  }

#endif

  return true;
}

ezResult ezTextureResourceLoader::LoadTexFile(ezStreamReader& stream, LoadedData& data)
{
  // read the hash, ignore it
  ezAssetFileHeader AssetHash;
  AssetHash.Read(stream);

  data.m_TexFormat.ReadHeader(stream);

  if (data.m_TexFormat.m_iRenderTargetResolutionX == 0)
  {
    ezDdsFileFormat fmt;
    return fmt.ReadImage(stream, data.m_Image, ezLog::GetThreadLocalLogSystem(), "dds");
  }
  else
  {
    return EZ_SUCCESS;
  }
}

void ezTextureResourceLoader::WriteTextureLoadStream(ezStreamWriter& w, const LoadedData& data)
{
  const ezImage* pImage = &data.m_Image;
  w.WriteBytes(&pImage, sizeof(ezImage*));

  w << data.m_bIsFallback;
  data.m_TexFormat.WriteRenderTargetHeader(w);
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Textures_TextureLoader);

