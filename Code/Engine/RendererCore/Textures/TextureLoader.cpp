#include <PCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Image/Formats/DdsFileFormat.h>
#include <Foundation/Image/ImageConversion.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererCore/Textures/TextureLoader.h>
#include <RendererCore/Textures/TextureUtils.h>

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
}

ON_CORESYSTEMS_SHUTDOWN
{
  ezResourceManager::SetResourceTypeLoader<ezTexture2DResource>(nullptr);
  ezResourceManager::SetResourceTypeLoader<ezTextureCubeResource>(nullptr);
}

ON_HIGHLEVELSYSTEMS_STARTUP
{
}

ON_HIGHLEVELSYSTEMS_SHUTDOWN
{
}

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezResourceLoadData ezTextureResourceLoader::OpenDataStream(const ezResourceBase* pResource)
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

    pData->m_bSRGB = true;
    pData->m_Image.SetWidth(4);
    pData->m_Image.SetHeight(4);
    pData->m_Image.SetDepth(1);
    pData->m_Image.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM_SRGB);
    pData->m_Image.SetNumMipLevels(1);
    pData->m_Image.SetNumFaces(1);
    pData->m_Image.AllocateImageData();
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
      if (ezOSFile::GetFileStats(sAbsolutePath, stat).Succeeded())
      {
        res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
      }
    }
#endif

    /// In case this is not a proper asset (ezTex format), this is a hack to get the SRGB information for the texture
    const ezStringBuilder sName = ezPathUtils::GetFileName(sAbsolutePath);
    pData->m_bSRGB = (sName.EndsWith_NoCase("_D") || sName.EndsWith_NoCase("_SRGB") || sName.EndsWith_NoCase("_diff"));

    if (sAbsolutePath.HasExtension("ezTex"))
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

void ezTextureResourceLoader::CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData)
{
  LoadedData* pData = (LoadedData*)LoaderData.m_pCustomLoaderData;

  EZ_DEFAULT_DELETE(pData);
}

bool ezTextureResourceLoader::IsResourceOutdated(const ezResourceBase* pResource) const
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
    if (ezOSFile::GetFileStats(sAbs, stat).Failed())
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

  // read the ezTex file format
  ezUInt8 uiTexFileFormatVersion = 0;
  stream >> uiTexFileFormatVersion;

  if (uiTexFileFormatVersion < 2)
  {
    // There is no version 0 or 1, some idiot forgot to add a version number to the format.
    // However, the first byte in those versions is a bool, so either 0 or 1 and therefore we can detect
    // old versions by making the minimum file format version 2. I'm so clever! And handsome.

    data.m_bSRGB = (uiTexFileFormatVersion == 1);
    uiTexFileFormatVersion = 1;
  }
  else
  {
    stream >> data.m_bSRGB;
  }

  stream >> data.m_addressModeU;
  stream >> data.m_addressModeV;
  stream >> data.m_addressModeW;

  if (uiTexFileFormatVersion >= 2)
  {
    ezUInt8 uiFilter = 0;
    stream >> uiFilter;
    data.m_textureFilter = (ezTextureFilterSetting::Enum)uiFilter;
  }

  if (uiTexFileFormatVersion >= 3)
  {
    stream >> data.m_iRenderTargetResolutionX;
    stream >> data.m_iRenderTargetResolutionY;
  }

  if (uiTexFileFormatVersion >= 4)
  {
    stream >> data.m_fResolutionScale;
  }

  data.m_GalRenderTargetFormat = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
  if (uiTexFileFormatVersion >= 5)
  {
    stream >> data.m_GalRenderTargetFormat;
  }

  if (data.m_iRenderTargetResolutionX == 0)
  {
    ezDdsFileFormat fmt;
    return fmt.ReadImage(stream, data.m_Image, ezLog::GetThreadLocalLogSystem());
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
  w << data.m_bSRGB;
  w << data.m_addressModeU;
  w << data.m_addressModeV;
  w << data.m_addressModeW;
  w << data.m_textureFilter;
  w << data.m_iRenderTargetResolutionX;
  w << data.m_iRenderTargetResolutionY;
  w << data.m_fResolutionScale;
  w << data.m_GalRenderTargetFormat;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Textures_TextureLoader);
