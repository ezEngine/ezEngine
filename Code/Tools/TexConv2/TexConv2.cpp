#include <TexConv2PCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <TexConv2/TexConv2.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/ezTexFormat/ezTexFormat.h>

ezTexConv2::ezTexConv2()
  : ezApplication("TexConv2")
{
  // texture types
  {
    m_AllowedOutputTypes.PushBack({"2D", ezTexConvOutputType::Texture2D});
    m_AllowedOutputTypes.PushBack({"Volume", ezTexConvOutputType::Volume});
    m_AllowedOutputTypes.PushBack({"Cubemap", ezTexConvOutputType::Cubemap});
    m_AllowedOutputTypes.PushBack({"Atlas", ezTexConvOutputType::Atlas});
  }

  // texture usages
  {
    m_AllowedUsages.PushBack({"Auto", ezTexConvUsage::Auto});
    m_AllowedUsages.PushBack({"Color", ezTexConvUsage::Color});
    m_AllowedUsages.PushBack({"Linear", ezTexConvUsage::Linear});
    m_AllowedUsages.PushBack({"Hdr", ezTexConvUsage::Hdr});
    m_AllowedUsages.PushBack({"NormalMap", ezTexConvUsage::NormalMap});
    m_AllowedUsages.PushBack({"NormalMap_Inverted", ezTexConvUsage::NormalMap_Inverted});
    m_AllowedUsages.PushBack({"BumpMap", ezTexConvUsage::BumpMap});
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
    m_AllowedWrapModes.PushBack({"Repeat", ezImageAddressMode::Repeat});
    m_AllowedWrapModes.PushBack({"Clamp", ezImageAddressMode::Clamp});
    m_AllowedWrapModes.PushBack({"ClampBorder", ezImageAddressMode::ClampBorder});
    m_AllowedWrapModes.PushBack({"Mirror", ezImageAddressMode::Mirror});
  }

  // filter modes
  {
    m_AllowedFilterModes.PushBack({"Default", ezTextureFilterSetting::DefaultQuality});
    m_AllowedFilterModes.PushBack({"Lowest", ezTextureFilterSetting::LowestQuality});
    m_AllowedFilterModes.PushBack({"Low", ezTextureFilterSetting::LowQuality});
    m_AllowedFilterModes.PushBack({"High", ezTextureFilterSetting::HighQuality});
    m_AllowedFilterModes.PushBack({"Highest", ezTextureFilterSetting::HighestQuality});

    m_AllowedFilterModes.PushBack({"Nearest", ezTextureFilterSetting::FixedNearest});
    m_AllowedFilterModes.PushBack({"Bilinear", ezTextureFilterSetting::FixedBilinear});
    m_AllowedFilterModes.PushBack({"Trilinear", ezTextureFilterSetting::FixedTrilinear});
    m_AllowedFilterModes.PushBack({"Aniso2x", ezTextureFilterSetting::FixedAnisotropic2x});
    m_AllowedFilterModes.PushBack({"Aniso4x", ezTextureFilterSetting::FixedAnisotropic4x});
    m_AllowedFilterModes.PushBack({"Aniso8x", ezTextureFilterSetting::FixedAnisotropic8x});
    m_AllowedFilterModes.PushBack({"Aniso16x", ezTextureFilterSetting::FixedAnisotropic16x});
  }

  // bump map modes
  {
    m_AllowedBumpMapFilters.PushBack({"Finite", ezTexConvBumpMapFilter::Finite});
    m_AllowedBumpMapFilters.PushBack({"Sobel", ezTexConvBumpMapFilter::Sobel});
    m_AllowedBumpMapFilters.PushBack({"Scharr", ezTexConvBumpMapFilter::Scharr});
  }
}

ezResult ezTexConv2::BeforeCoreSystemsStartup()
{
  ezStartup::AddApplicationTag("tool");
  ezStartup::AddApplicationTag("texconv");

  return SUPER::BeforeCoreSystemsStartup();
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
  if (m_sOutputFile.IsEmpty())
  {
    m_Processor.m_Descriptor.m_OutputType = ezTexConvOutputType::None;
    return EZ_SUCCESS;
  }

  ezStringBuilder sExt = ezPathUtils::GetFileExtension(m_sOutputFile);
  sExt.ToUpper();

  if (sExt == "DDS")
  {
    m_bOutputSupports2D = true;
    m_bOutputSupports3D = true;
    m_bOutputSupportsCube = true;
    m_bOutputSupportsAtlas = false;
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
    m_bOutputSupportsAtlas = false;
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
    m_bOutputSupportsAtlas = false;
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
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = true;
    m_bOutputSupportsCompression = true;
    return EZ_SUCCESS;
  }
  if (sExt == "EZTEXTURECUBE")
  {
    m_bOutputSupports2D = false;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = true;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = true;
    m_bOutputSupportsCompression = true;
    return EZ_SUCCESS;
  }
  if (sExt == "EZTEXTUREATLAS")
  {
    m_bOutputSupports2D = false;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsAtlas = true;
    m_bOutputSupportsMipmaps = true;
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
  asset.SetFileHashAndVersion(m_Processor.m_Descriptor.m_uiAssetHash, m_Processor.m_Descriptor.m_uiAssetVersion);

  asset.Write(stream);

  ezTexFormat texFormat;
  texFormat.m_bSRGB = ezImageFormat::IsSrgb(image.GetImageFormat());
  texFormat.m_AddressModeU = m_Processor.m_Descriptor.m_AddressModeU;
  texFormat.m_AddressModeV = m_Processor.m_Descriptor.m_AddressModeV;
  texFormat.m_AddressModeW = m_Processor.m_Descriptor.m_AddressModeW;
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
  SetReturnCode(-1);

  if (ParseCommandLine().Failed())
    return ezApplication::ApplicationExecution::Quit;

  if (m_Processor.Process().Failed())
    return ezApplication::ApplicationExecution::Quit;

  if (m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::Atlas)
  {
    ezDeferredFileWriter file;
    file.SetOutput(m_sOutputFile);

    ezAssetFileHeader header;
    header.SetFileHashAndVersion(m_Processor.m_Descriptor.m_uiAssetHash, m_Processor.m_Descriptor.m_uiAssetVersion);

    header.Write(file);

    file.WriteBytes(m_Processor.m_TextureAtlas.GetData(), m_Processor.m_TextureAtlas.GetStorageSize());

    return ezApplication::ApplicationExecution::Quit;
  }

  if (!m_sOutputFile.IsEmpty() && m_Processor.m_OutputImage.IsValid())
  {
    if (WriteOutputFile(m_sOutputFile, m_Processor.m_OutputImage).Failed())
    {
      ezLog::Error("Failed to write main result to '{}'", m_sOutputFile);
      return ezApplication::ApplicationExecution::Quit;
    }

    ezLog::Success("Wrote main result to '{}'", m_sOutputFile);
  }

  if (!m_sOutputThumbnailFile.IsEmpty() && m_Processor.m_ThumbnailOutputImage.IsValid())
  {
    if (m_Processor.m_ThumbnailOutputImage.SaveTo(m_sOutputThumbnailFile).Failed())
    {
      ezLog::Error("Failed to write thumbnail result to '{}'", m_sOutputThumbnailFile);
      return ezApplication::ApplicationExecution::Quit;
    }

    ezLog::Success("Wrote thumbnail to '{}'", m_sOutputThumbnailFile);
  }

  if (!m_sOutputLowResFile.IsEmpty())
  {
    // the image may not exist, if we do not have enough mips, so make sure any old low-res file is cleaned up
    ezOSFile::DeleteFile(m_sOutputLowResFile);

    if (m_Processor.m_LowResOutputImage.IsValid())
    {
      if (WriteOutputFile(m_sOutputLowResFile, m_Processor.m_LowResOutputImage).Failed())
      {
        ezLog::Error("Failed to write low-res result to '{}'", m_sOutputLowResFile);
        return ezApplication::ApplicationExecution::Quit;
      }

      ezLog::Success("Wrote low-res result to '{}'", m_sOutputLowResFile);
    }
  }

  SetReturnCode(0);
  return ezApplication::ApplicationExecution::Quit;
}

EZ_CONSOLEAPP_ENTRY_POINT(ezTexConv2);
