#include <RendererCore/PCH.h>
#include <RendererCore/Textures/TextureLoader.h>
#include <CoreUtils/Image/Formats/DdsFileFormat.h>
#include <CoreUtils/Assets/AssetFileHeader.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>

static ezTextureResourceLoader s_TextureResourceLoader;

EZ_BEGIN_SUBSYSTEM_DECLARATION(Graphics, TextureResource)

BEGIN_SUBSYSTEM_DEPENDENCIES
"Foundation",
"Core"
END_SUBSYSTEM_DEPENDENCIES

ON_CORE_STARTUP
{
  ezResourceManager::SetResourceTypeLoader<ezTexture2DResource>(&s_TextureResourceLoader);
  ezResourceManager::SetResourceTypeLoader<ezTextureCubeResource>(&s_TextureResourceLoader);
}

ON_CORE_SHUTDOWN
{
  ezResourceManager::SetResourceTypeLoader<ezTexture2DResource>(nullptr);
  ezResourceManager::SetResourceTypeLoader<ezTextureCubeResource>(nullptr);
}

ON_ENGINE_STARTUP
{
}

ON_ENGINE_SHUTDOWN
{
}

EZ_END_SUBSYSTEM_DECLARATION

ezResourceLoadData ezTextureResourceLoader::OpenDataStream(const ezResourceBase* pResource)
{
  LoadedData* pData = EZ_DEFAULT_NEW(LoadedData);

  ezResourceLoadData res;

  bool bSRGB = false;
  ezEnum<ezGALTextureAddressMode> addressModeU = ezGALTextureAddressMode::Wrap;
  ezEnum<ezGALTextureAddressMode> addressModeV = ezGALTextureAddressMode::Wrap;
  ezEnum<ezGALTextureAddressMode> addressModeW = ezGALTextureAddressMode::Wrap;
  ezEnum<ezTextureFilterSetting> textureFilter = ezTextureFilterSetting::Default;

  // Solid Color Textures
  if (ezPathUtils::HasExtension(pResource->GetResourceID(), "color"))
  {
    ezStringBuilder sName = pResource->GetResourceID();
    sName.RemoveFileExtension();

    bool bValidColor = false;
    const ezColorGammaUB color = ezConversionUtils::GetColorByName(sName, &bValidColor);

    if (!bValidColor)
    {
      ezLog::Error("'{0}' is not a valid color name. Using 'RebeccaPurple' as fallback.", sName.GetData());
    }

    bSRGB = true;
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
    bSRGB = (sName.EndsWith_NoCase("_D") || sName.EndsWith_NoCase("_SRGB") || sName.EndsWith_NoCase("_diff"));

    if (sAbsolutePath.HasExtension("ezTex"))
    {
      // read the hash, ignore it
      ezAssetFileHeader AssetHash;
      AssetHash.Read(File);

      // read the ezTex file format
      ezUInt8 uiTexFileFormatVersion = 0;
      File >> uiTexFileFormatVersion;

      if (uiTexFileFormatVersion < 2)
      {
        // There is no version 0 or 1, some idiot forgot to add a version number to the format.
        // However, the first byte in those versions is a bool, so either 0 or 1 and therefore we can detect
        // old versions by making the minimum file format version 2. I'm so clever! And handsome.

        bSRGB = (uiTexFileFormatVersion == 1);
        uiTexFileFormatVersion = 1;
      }
      else
      {
        File >> bSRGB;
      }

      File >> addressModeU;
      File >> addressModeV;
      File >> addressModeW;

      if (uiTexFileFormatVersion >= 2)
      {
        ezUInt8 uiFilter = 0;
        File >> uiFilter;
        textureFilter = (ezTextureFilterSetting::Enum) uiFilter;
      }

      ezDdsFileFormat fmt;
      if (fmt.ReadImage(File, pData->m_Image, ezGlobalLog::GetOrCreateInstance()).Failed())
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

        ezLog::Warning("Texture resource uses inefficient BGR format, converting to BGRX: '{0}'", sAbsolutePath.GetData());
        if (ezImageConversion::Convert(pData->m_Image, pData->m_Image, ezImageFormat::B8G8R8A8_UNORM).Failed())
          return res;
      }
    }
  }

  ezMemoryStreamWriter w(&pData->m_Storage);

  ezImage* pImage = &pData->m_Image;
  w.WriteBytes(&pImage, sizeof(ezImage*));

  w << bSRGB;
  w << addressModeU;
  w << addressModeV;
  w << addressModeW;
  w << textureFilter;

  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

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

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
  if (pResource->GetLoadedFileModificationTime().IsValid())
  {
    ezStringBuilder sAbs;
    if (ezFileSystem::ResolvePath(pResource->GetResourceID(), &sAbs, nullptr).Failed())
      return false;

    ezFileStats stat;
    if (ezOSFile::GetFileStats(sAbs, stat).Failed())
      return false;

    return !stat.m_LastModificationTime.Compare(pResource->GetLoadedFileModificationTime(), ezTimestamp::CompareMode::FileTimeEqual);
  }

#endif

  return true;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Textures_TextureResource);

