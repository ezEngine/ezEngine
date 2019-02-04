#include <PCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <TexConv2/TexConv2.h>
#include <Texture/ezTexFormat/ezTexFormat.h>

/* TODO LIST:

- cubemaps:
- from single cubemap dds
- from multiple cubemap dds
- from multiple 2d textures
- from single 2d texture

- decal atlas support

- volume textures

- docs for params / help
*/


ezTexConv2::ezTexConv2()
  : ezApplication("TexConv2")
{
  // texture types
  {
    m_AllowedOutputTypes.PushBack({"2D", ezTexConvOutputType::Texture2D});
    m_AllowedOutputTypes.PushBack({"cubemap", ezTexConvOutputType::TextureCube});
  }

  // texture usages
  {
    m_AllowedUsages.PushBack({"Auto", ezTexConvUsage::Auto});
    m_AllowedUsages.PushBack({"Color", ezTexConvUsage::Color});
    m_AllowedUsages.PushBack({"Linear", ezTexConvUsage::Linear});
    m_AllowedUsages.PushBack({"Hdr", ezTexConvUsage::Hdr});
    m_AllowedUsages.PushBack({"NormalMap", ezTexConvUsage::NormalMap});
    m_AllowedUsages.PushBack({"NormalMap_Inverted", ezTexConvUsage::NormalMap_Inverted});
  }

  // mipmap modes
  {
    m_AllowedMimapModes.PushBack({"Linear", ezTexConvMipmapMode::Linear});
    m_AllowedMimapModes.PushBack({"Kaiser", ezTexConvMipmapMode::Kaiser});
    m_AllowedMimapModes.PushBack({"None", ezTexConvMipmapMode::None});
  }

  // platforms
  {
    m_AllowedPlatforms.PushBack({"PC", ezTexConvTargetPlatform::PC});
    m_AllowedPlatforms.PushBack({"Android", ezTexConvTargetPlatform::Android});
  }

  // compression modes
  {
    m_AllowedCompressionModes.PushBack({"Medium", ezTexConvCompressionMode::Medium});
    m_AllowedCompressionModes.PushBack({"High", ezTexConvCompressionMode::High});
    m_AllowedCompressionModes.PushBack({"None", ezTexConvCompressionMode::None});
  }

  // wrap modes
  {
    m_AllowedWrapModes.PushBack({"Repeat", ezTexConvWrapMode::Repeat});
    m_AllowedWrapModes.PushBack({"Clamp", ezTexConvWrapMode::Clamp});
    m_AllowedWrapModes.PushBack({"Mirror", ezTexConvWrapMode::Mirror});
  }

  // filter modes
  {
    m_AllowedFilterModes.PushBack({"Default", ezTexConvFilterMode::DefaultQuality});
    m_AllowedFilterModes.PushBack({"Lowest", ezTexConvFilterMode::LowestQuality});
    m_AllowedFilterModes.PushBack({"Low", ezTexConvFilterMode::LowQuality});
    m_AllowedFilterModes.PushBack({"High", ezTexConvFilterMode::HighQuality});
    m_AllowedFilterModes.PushBack({"Highest", ezTexConvFilterMode::HighestQuality});

    m_AllowedFilterModes.PushBack({"Nearest", ezTexConvFilterMode::FixedNearest});
    m_AllowedFilterModes.PushBack({"Bilinear", ezTexConvFilterMode::FixedBilinear});
    m_AllowedFilterModes.PushBack({"Trilinear", ezTexConvFilterMode::FixedTrilinear});
    m_AllowedFilterModes.PushBack({"Aniso2x", ezTexConvFilterMode::FixedAnisotropic2x});
    m_AllowedFilterModes.PushBack({"Aniso4x", ezTexConvFilterMode::FixedAnisotropic4x});
    m_AllowedFilterModes.PushBack({"Aniso8x", ezTexConvFilterMode::FixedAnisotropic8x});
    m_AllowedFilterModes.PushBack({"Aniso16x", ezTexConvFilterMode::FixedAnisotropic16x});
  }
}

void ezTexConv2::BeforeCoreSystemsStartup()
{
  ezStartup::AddApplicationTag("tool");
  ezStartup::AddApplicationTag("texconv");

  SUPER::BeforeCoreSystemsStartup();
}

void ezTexConv2::AfterCoreSystemsStartup()
{
  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  ezFileSystem::AddDataDirectory("", "App", ":", ezFileSystem::AllowWrites);

  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
}

void ezTexConv2::BeforeCoreSystemsShutdown()
{
  ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  SUPER::BeforeCoreSystemsShutdown();
}

ezResult ezTexConv2::DetectOutputFormat()
{
  ezStringBuilder sExt = ezPathUtils::GetFileExtension(m_sOutputFile);
  sExt.ToUpper();

  if (sExt == "DDS")
  {
    m_bOutputSupports2D = true;
    m_bOutputSupports3D = true;
    m_bOutputSupportsCube = true;
    m_bOutputSupportsRenderTarget = false;
    m_bOutputSupportsDecal = false;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = false;
    m_bOutputSupportsCompression = true;
    return EZ_SUCCESS;
  }
  if (sExt == "TGA")
  {
    m_bOutputSupports2D = true;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsRenderTarget = false;
    m_bOutputSupportsDecal = false;
    m_bOutputSupportsMipmaps = false;
    m_bOutputSupportsFiltering = false;
    m_bOutputSupportsCompression = false;
    return EZ_SUCCESS;
  }
  if (sExt == "EZTEXTURE2D")
  {
    m_bOutputSupports2D = true;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsRenderTarget = false;
    m_bOutputSupportsDecal = false;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = true;
    m_bOutputSupportsCompression = true;
    return EZ_SUCCESS;
  }
  if (sExt == "EZTEXTURE3D")
  {
    m_bOutputSupports2D = false;
    m_bOutputSupports3D = true;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsRenderTarget = false;
    m_bOutputSupportsDecal = false;
    m_bOutputSupportsMipmaps = false;
    m_bOutputSupportsFiltering = true;
    m_bOutputSupportsCompression = true;
    return EZ_SUCCESS;
  }
  if (sExt == "EZTEXTURECUBE")
  {
    m_bOutputSupports2D = false;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = true;
    m_bOutputSupportsRenderTarget = false;
    m_bOutputSupportsDecal = false;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = true;
    m_bOutputSupportsCompression = true;
    return EZ_SUCCESS;
  }
  if (sExt == "EZRENDERTARGET")
  {
    m_bOutputSupports2D = false;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsRenderTarget = true;
    m_bOutputSupportsDecal = false;
    m_bOutputSupportsMipmaps = false;
    m_bOutputSupportsFiltering = true;
    m_bOutputSupportsCompression = false;
    return EZ_SUCCESS;
  }
  if (sExt == "EZDECALATLAS")
  {
    m_bOutputSupports2D = false;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsRenderTarget = false;
    m_bOutputSupportsDecal = true;
    m_bOutputSupportsMipmaps = false;
    m_bOutputSupportsFiltering = true;
    m_bOutputSupportsCompression = true;
    return EZ_SUCCESS;
  }

  ezLog::Error("Output file uses unsupported file format '{}'", sExt);
  return EZ_FAILURE;
}


bool ezTexConv2::IsTexFormat() const
{
  const ezStringView ext = ezPathUtils::GetFileExtension(m_sOutputFile);

  return ext.StartsWith_NoCase("ez");
}

ezResult ezTexConv2::WriteTexFile(ezStreamWriter& stream, const ezImage& image)
{
  ezAssetFileHeader asset;
  asset.SetFileHashAndVersion(m_uiEzFormatAssetHash, m_uiEzFormatAssetVersion);

  asset.Write(stream);

  ezTexFormat texFormat;
  texFormat.m_bSRGB = ezImageFormat::IsSrgb(image.GetImageFormat());
  texFormat.m_WrapModeU = m_Processor.m_Descriptor.m_WrapModes[0];
  texFormat.m_WrapModeV = m_Processor.m_Descriptor.m_WrapModes[1];
  texFormat.m_WrapModeW = m_Processor.m_Descriptor.m_WrapModes[2];
  texFormat.m_TextureFilter = m_Processor.m_Descriptor.m_FilterMode;

  texFormat.WriteTextureHeader(stream);

  ezDdsFileFormat ddsWriter;
  if (ddsWriter.WriteImage(stream, image, ezLog::GetThreadLocalLogSystem(), "dds").Failed())
  {
    ezLog::Error("Failed to write DDS image chunk to ezTex file.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezTexConv2::WriteOutputFile(const char* szFile, const ezImage& image)
{
  if (IsTexFormat())
  {
    ezDeferredFileWriter file;
    file.SetOutput(szFile);

    WriteTexFile(file, image);

    return file.Close();
  }
  else
  {
    return image.SaveTo(szFile);
  }
}

ezApplication::ApplicationExecution ezTexConv2::Run()
{
  if (ParseCommandLine().Failed())
    return ezApplication::ApplicationExecution::Quit;

  if (m_Processor.Process().Failed())
    return ezApplication::ApplicationExecution::Quit;

  if (!m_sOutputFile.IsEmpty())
  {
    if (WriteOutputFile(m_sOutputFile, m_Processor.m_OutputImage).Failed())
    {
      ezLog::Error("Failed to write main result to '{}'", m_sOutputFile);
      return ezApplication::ApplicationExecution::Quit;
    }

    ezLog::Success("Wrote main result to '{}'", m_sOutputFile);
  }

  if (!m_sOutputThumbnailFile.IsEmpty())
  {
    if (m_Processor.m_ThumbnailOutputImage.SaveTo(m_sOutputThumbnailFile).Failed())
    {
      ezLog::Error("Failed to write thumbnail result to '{}'", m_sOutputThumbnailFile);
      return ezApplication::ApplicationExecution::Quit;
    }

    ezLog::Success("Wrote thumbnail to '{}'", m_sOutputThumbnailFile);
  }

  if (!m_sOutputLowResFile.IsEmpty() && m_Processor.m_LowResOutputImage.GetNumMipLevels() > 0)
  {
    if (WriteOutputFile(m_sOutputLowResFile, m_Processor.m_LowResOutputImage).Failed())
    {
      ezLog::Error("Failed to write low-res result to '{}'", m_sOutputLowResFile);
      return ezApplication::ApplicationExecution::Quit;
    }

    ezLog::Success("Wrote low-res result to '{}'", m_sOutputLowResFile);
  }

  return ezApplication::ApplicationExecution::Quit;
}

EZ_CONSOLEAPP_ENTRY_POINT(ezTexConv2);
