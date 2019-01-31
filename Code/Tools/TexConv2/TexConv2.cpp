#include <PCH.h>

#include <TexConv2/TexConv2.h>

/* TODO LIST:

- cubemaps:
- from single cubemap dds
- from multiple cubemap dds
- from multiple 2d textures
- from single 2d texture

- BC7 compression support

- ez texture output formats
- decal atlas support
- uint32: asset hash
- render target

- volume textures
- normal map from heightmap ?

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
    m_AllowedUsages.PushBack({"Color_Hdr", ezTexConvUsage::Color_Hdr});
    m_AllowedUsages.PushBack({"Grayscale", ezTexConvUsage::Grayscale});
    m_AllowedUsages.PushBack({"Grayscale_Hdr", ezTexConvUsage::Grayscale_Hdr});
    m_AllowedUsages.PushBack({"NormalMap", ezTexConvUsage::NormalMap});
    m_AllowedUsages.PushBack({"NormalMap_Inverted", ezTexConvUsage::NormalMap_Inverted});
    m_AllowedUsages.PushBack({"Compressed_1_Channel", ezTexConvUsage::Compressed_1_Channel});
    m_AllowedUsages.PushBack({"Compressed_2_Channel", ezTexConvUsage::Compressed_2_Channel});
    m_AllowedUsages.PushBack({"Compressed_4_Channel", ezTexConvUsage::Compressed_4_Channel});
    m_AllowedUsages.PushBack({"Compressed_4_Channel_sRGB", ezTexConvUsage::Compressed_4_Channel_sRGB});
    m_AllowedUsages.PushBack({"Compressed_Hdr_3_Channel", ezTexConvUsage::Compressed_Hdr_3_Channel});
    m_AllowedUsages.PushBack({"Uncompressed_8_Bit_UNorm_1_Channel", ezTexConvUsage::Uncompressed_8_Bit_UNorm_1_Channel});
    m_AllowedUsages.PushBack({"Uncompressed_8_Bit_UNorm_2_Channel", ezTexConvUsage::Uncompressed_8_Bit_UNorm_2_Channel});
    m_AllowedUsages.PushBack({"Uncompressed_8_Bit_UNorm_4_Channel", ezTexConvUsage::Uncompressed_8_Bit_UNorm_4_Channel});
    m_AllowedUsages.PushBack({"Uncompressed_8_Bit_UNorm_4_Channel_SRGB", ezTexConvUsage::Uncompressed_8_Bit_UNorm_4_Channel_SRGB});
    m_AllowedUsages.PushBack({"Uncompressed_16_Bit_UNorm_1_Channel", ezTexConvUsage::Uncompressed_16_Bit_UNorm_1_Channel});
    m_AllowedUsages.PushBack({"Uncompressed_16_Bit_UNorm_2_Channel", ezTexConvUsage::Uncompressed_16_Bit_UNorm_2_Channel});
    m_AllowedUsages.PushBack({"Uncompressed_16_Bit_UNorm_4_Channel", ezTexConvUsage::Uncompressed_16_Bit_UNorm_4_Channel});
    m_AllowedUsages.PushBack({"Uncompressed_16_Bit_Float_1_Channel", ezTexConvUsage::Uncompressed_16_Bit_Float_1_Channel});
    m_AllowedUsages.PushBack({"Uncompressed_16_Bit_Float_2_Channel", ezTexConvUsage::Uncompressed_16_Bit_Float_2_Channel});
    m_AllowedUsages.PushBack({"Uncompressed_16_Bit_Float_4_Channel", ezTexConvUsage::Uncompressed_16_Bit_Float_4_Channel});
    m_AllowedUsages.PushBack({"Uncompressed_32_Bit_Float_1_Channel", ezTexConvUsage::Uncompressed_32_Bit_Float_1_Channel});
    m_AllowedUsages.PushBack({"Uncompressed_32_Bit_Float_2_Channel", ezTexConvUsage::Uncompressed_32_Bit_Float_2_Channel});
    m_AllowedUsages.PushBack({"Uncompressed_32_Bit_Float_3_Channel", ezTexConvUsage::Uncompressed_32_Bit_Float_3_Channel});
    m_AllowedUsages.PushBack({"Uncompressed_32_Bit_Float_4_Channel", ezTexConvUsage::Uncompressed_32_Bit_Float_4_Channel});
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
    m_AllowedCompressionModes.PushBack({"Quality", ezTexConvCompressionMode::OptimizeForQuality});
    m_AllowedCompressionModes.PushBack({"Size", ezTexConvCompressionMode::OptimizeForSize});
    m_AllowedCompressionModes.PushBack({"None", ezTexConvCompressionMode::Uncompressed});
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


ezApplication::ApplicationExecution ezTexConv2::Run()
{
  if (ParseCommandLine().Failed())
    return ezApplication::ApplicationExecution::Quit;

  if (m_Processor.Process().Failed())
    return ezApplication::ApplicationExecution::Quit;

  if (!m_sOutputFile.IsEmpty())
  {
    if (m_Processor.m_OutputImage.SaveTo(m_sOutputFile).Failed())
    {
      ezLog::Error("Failed to write main result to '{}'", m_sOutputFile);
      return ezApplication::ApplicationExecution::Quit;
    }
    else
    {
      ezLog::Success("Wrote main result to '{}'", m_sOutputFile);
    }
  }

  if (!m_sOutputThumbnailFile.IsEmpty())
  {
    if (m_Processor.m_ThumbnailOutputImage.SaveTo(m_sOutputThumbnailFile).Failed())
    {
      ezLog::Error("Failed to write thumbnail result to '{}'", m_sOutputThumbnailFile);
      return ezApplication::ApplicationExecution::Quit;
    }
    else
    {
      ezLog::Success("Wrote thumbnail to '{}'", m_sOutputThumbnailFile);
    }
  }

  if (!m_sOutputLowResFile.IsEmpty() && m_Processor.m_LowResOutputImage.GetNumMipLevels() > 0)
  {
    if (m_Processor.m_LowResOutputImage.SaveTo(m_sOutputLowResFile).Failed())
    {
      ezLog::Error("Failed to write low-res result to '{}'", m_sOutputLowResFile);
      return ezApplication::ApplicationExecution::Quit;
    }
    else
    {
      ezLog::Success("Wrote low-res result to '{}'", m_sOutputLowResFile);
    }
  }

  return ezApplication::ApplicationExecution::Quit;
}

EZ_CONSOLEAPP_ENTRY_POINT(ezTexConv2);
