#include <PCH.h>

#include <TexConv2/TexConv2.h>

/* TODO LIST:

- cubemaps:
- from single cubemap dds
- from multiple cubemap dds
- from multiple 2d textures
- from single 2d texture

- flip horizontal for 2D
- premultiply alpha
- hdr exposure bias
- min / max resolution
- downscale steps
- preserve mipmap coverage
- mipmap alpha threshold
- low res output
- thumbnail output
- BC7 compression support

- ez texture output formats
- decal atlas support
- asset hash
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

ezResult ezTexConv2::ParseCommandLine()
{
  EZ_SUCCEED_OR_RETURN(ParseOutputFiles());
  EZ_SUCCEED_OR_RETURN(DetectOutputFormat());

  EZ_SUCCEED_OR_RETURN(ParseTargetPlatform());
  EZ_SUCCEED_OR_RETURN(ParseOutputType());
  EZ_SUCCEED_OR_RETURN(ParseCompressionMode());
  EZ_SUCCEED_OR_RETURN(ParseUsage());
  EZ_SUCCEED_OR_RETURN(ParseMipmapMode());
  EZ_SUCCEED_OR_RETURN(ParseWrapModes());
  EZ_SUCCEED_OR_RETURN(ParseFilterModes());
  EZ_SUCCEED_OR_RETURN(ParseInputFiles());
  EZ_SUCCEED_OR_RETURN(ParseChannelMappings());

  return EZ_SUCCESS;
}

ezResult ezTexConv2::ParseOutputType()
{
  ezInt32 value = -1;
  EZ_SUCCEED_OR_RETURN(ParseStringOption("-type", m_AllowedOutputTypes, value));

  m_Processor.m_Descriptor.m_OutputType = static_cast<ezTexConvOutputType::Enum>(value);

  if (m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::Texture2D)
  {
    if (!m_bOutputSupports2D)
    {
      ezLog::Error("2D textures are not supported by the chosen output file format.");
      return EZ_FAILURE;
    }
  }
  else if (m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::TextureCube)
  {
    if (!m_bOutputSupportsCube)
    {
      ezLog::Error("Cubemap textures are not supported by the chosen output file format.");
      return EZ_FAILURE;
    }
  }
  else
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
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

ezResult ezTexConv2::ParseInputFiles()
{
  ezStringBuilder tmp, res;
  const auto pCmd = ezCommandLineUtils::GetGlobalInstance();

  auto& files = m_Processor.m_Descriptor.m_InputFiles;

  for (ezUInt32 i = 0; i < 64; ++i)
  {
    tmp.Format("-in{0}", i);

    res = pCmd->GetStringOption(tmp);

    // stop once an option was not found
    if (res.IsEmpty())
      break;

    files.EnsureCount(i + 1);
    files[i] = res;
  }

  // if no numbered inputs were given, try '-in', ignore it otherwise
  if (files.IsEmpty())
  {
    // short version for -in1
    res = pCmd->GetStringOption("-in");

    if (!res.IsEmpty())
    {
      files.PushBack(res);
    }
  }

  if (m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::TextureCube)
  {
    // 0 = +X = Right
    // 1 = -X = Left
    // 2 = +Y = Top
    // 3 = -Y = Bottom
    // 4 = +Z = Front
    // 5 = -Z = Back

    if (files.IsEmpty() && (pCmd->GetOptionIndex("-right") != -1 || pCmd->GetOptionIndex("-px") != -1))
    {
      files.SetCount(6);

      files[0] = pCmd->GetStringOption("-right", 0, files[0]);
      files[1] = pCmd->GetStringOption("-left", 0, files[1]);
      files[2] = pCmd->GetStringOption("-top", 0, files[2]);
      files[3] = pCmd->GetStringOption("-bottom", 0, files[3]);
      files[4] = pCmd->GetStringOption("-front", 0, files[4]);
      files[5] = pCmd->GetStringOption("-back", 0, files[5]);

      files[0] = pCmd->GetStringOption("-px", 0, files[0]);
      files[1] = pCmd->GetStringOption("-nx", 0, files[1]);
      files[2] = pCmd->GetStringOption("-py", 0, files[2]);
      files[3] = pCmd->GetStringOption("-ny", 0, files[3]);
      files[4] = pCmd->GetStringOption("-pz", 0, files[4]);
      files[5] = pCmd->GetStringOption("-nz", 0, files[5]);
    }
  }

  for (ezUInt32 i = 0; i < files.GetCount(); ++i)
  {
    if (files[i].IsEmpty())
    {
      ezLog::Error("Input file {} is not specified", i);
      return EZ_FAILURE;
    }

    ezLog::Info("Input file {}: '{}'", i, files[i]);
  }

  if (m_Processor.m_Descriptor.m_InputFiles.IsEmpty())
  {
    ezLog::Error("No input files were specified. Use \'-in \"path/to/file\"' to specify an input file. Use '-in0', '-in1' etc. to specify "
                 "multiple input files.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezTexConv2::ParseOutputFiles()
{
  const auto pCmd = ezCommandLineUtils::GetGlobalInstance();

  m_sOutputFile = pCmd->GetStringOption("-out");
  ezLog::Info("Output file: '{}'", m_sOutputFile);

  m_sOutputThumbnailFile = pCmd->GetStringOption("-thumbnailOut");
  if (!m_sOutputThumbnailFile.IsEmpty())
  {
    ezLog::Info("Thumbnail output file: '{}'", m_sOutputThumbnailFile);

    auto& resolution = m_Processor.m_Descriptor.m_uiThumbnailOutputResolution;
    resolution = pCmd->GetIntOption("-thumbnailRes", 256);
    ezLog::Info("Thumbnail resolution: '{}'", resolution);

    if (resolution < 32 || resolution > 1024 || !ezMath::IsPowerOf2(resolution))
    {
      ezLog::Error("Thumbnail output dimension must be between 32 and 1024 and a power-of-two.");
      return EZ_FAILURE;
    }
  }

  m_sOutputLowResFile = pCmd->GetStringOption("-lowOut");
  if (!m_sOutputLowResFile.IsEmpty())
  {
    ezLog::Info("LowRes output file: '{}'", m_sOutputThumbnailFile);

    auto& resolution = m_Processor.m_Descriptor.m_uiLowResOutputResolution;
    resolution = pCmd->GetIntOption("-lowRes", 32);
    ezLog::Info("LowRes resolution: '{}'", resolution);

    if (resolution < 4 || resolution > 256 || !ezMath::IsPowerOf2(resolution))
    {
      ezLog::Error("LowRes output dimension must be between 4 and 256 and a power-of-two.");
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConv2::ParseStringOption(const char* szOption, const ezDynamicArray<KeyEnumValuePair>& allowed, ezInt32& iResult) const
{
  const auto pCmd = ezCommandLineUtils::GetGlobalInstance();
  const ezStringBuilder sValue = pCmd->GetStringOption(szOption, 0);

  if (sValue.IsEmpty())
  {
    iResult = allowed[0].m_iEnumValue;

    ezLog::Info("Using default '{}': '{}'", szOption, allowed[0].m_szKey);
    return EZ_SUCCESS;
  }

  for (ezUInt32 i = 0; i < allowed.GetCount(); ++i)
  {
    if (sValue.IsEqual_NoCase(allowed[i].m_szKey))
    {
      iResult = allowed[i].m_iEnumValue;

      ezLog::Info("Selected '{}': '{}'", szOption, allowed[i].m_szKey);
      return EZ_SUCCESS;
    }
  }

  ezLog::Error("Unknown value for option '{}': '{}'.", szOption, sValue);

  PrintOptionValues(szOption, allowed);

  return EZ_FAILURE;
}

void ezTexConv2::PrintOptionValues(const char* szOption, const ezDynamicArray<KeyEnumValuePair>& allowed) const
{
  ezLog::Info("Valid values for option '{}' are:", szOption);

  for (ezUInt32 i = 0; i < allowed.GetCount(); ++i)
  {
    ezLog::Info("  {}", allowed[i].m_szKey);
  }
}

ezResult ezTexConv2::ParseUsage()
{
  ezInt32 value = -1;
  EZ_SUCCEED_OR_RETURN(ParseStringOption("-usage", m_AllowedUsages, value));

  m_Processor.m_Descriptor.m_Usage = static_cast<ezTexConvUsage::Enum>(value);
  return EZ_SUCCESS;
}

ezResult ezTexConv2::ParseMipmapMode()
{
  // if (m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::Texture3D)
  //  return EZ_SUCCESS;

  if (!m_bOutputSupportsMipmaps)
  {
    m_Processor.m_Descriptor.m_MipmapMode = ezTexConvMipmapMode::None;
    return EZ_SUCCESS;
  }

  ezInt32 value = -1;
  EZ_SUCCEED_OR_RETURN(ParseStringOption("-mipmaps", m_AllowedMimapModes, value));

  m_Processor.m_Descriptor.m_MipmapMode = static_cast<ezTexConvMipmapMode::Enum>(value);
  return EZ_SUCCESS;
}

ezResult ezTexConv2::ParseTargetPlatform()
{
  ezInt32 value = -1;
  EZ_SUCCEED_OR_RETURN(ParseStringOption("-platform", m_AllowedPlatforms, value));

  m_Processor.m_Descriptor.m_TargetPlatform = static_cast<ezTexConvTargetPlatform::Enum>(value);
  return EZ_SUCCESS;
}

ezResult ezTexConv2::ParseCompressionMode()
{
  if (!m_bOutputSupportsCompression)
  {
    m_Processor.m_Descriptor.m_CompressionMode = ezTexConvCompressionMode::Uncompressed;
    return EZ_SUCCESS;
  }

  ezInt32 value = -1;
  EZ_SUCCEED_OR_RETURN(ParseStringOption("-compression", m_AllowedCompressionModes, value));

  m_Processor.m_Descriptor.m_CompressionMode = static_cast<ezTexConvCompressionMode::Enum>(value);
  return EZ_SUCCESS;
}

ezResult ezTexConv2::ParseWrapModes()
{
  // cubemaps do not require any wrap mode settings
  if (m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::TextureCube)
    return EZ_SUCCESS;

  {
    ezInt32 value = -1;
    EZ_SUCCEED_OR_RETURN(ParseStringOption("-wrapU", m_AllowedWrapModes, value));
    m_Processor.m_Descriptor.m_WrapModes[0] = static_cast<ezTexConvWrapMode::Enum>(value);
  }
  {
    ezInt32 value = -1;
    EZ_SUCCEED_OR_RETURN(ParseStringOption("-wrapV", m_AllowedWrapModes, value));
    m_Processor.m_Descriptor.m_WrapModes[1] = static_cast<ezTexConvWrapMode::Enum>(value);
  }

  // if (m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::Texture3D)
  //{
  //  ezInt32 value = -1;
  //  EZ_SUCCEED_OR_RETURN(ParseStringOption("-wrapW", m_AllowedWrapModes, value));
  //  m_Processor.m_Descriptor.m_WrapModes[2] = static_cast<ezTexConvWrapMode::Enum>(value);
  //}

  return EZ_SUCCESS;
}

ezResult ezTexConv2::ParseFilterModes()
{
  if (!m_bOutputSupportsFiltering)
    return EZ_SUCCESS;

  ezInt32 value = -1;
  EZ_SUCCEED_OR_RETURN(ParseStringOption("-filter", m_AllowedFilterModes, value));

  m_Processor.m_Descriptor.m_FilterMode = static_cast<ezTexConvFilterMode::Enum>(value);
  return EZ_SUCCESS;
}

ezApplication::ApplicationExecution ezTexConv2::Run()
{
  if (ParseCommandLine().Failed())
    return ezApplication::ApplicationExecution::Quit;

  if (m_Processor.Process().Failed())
    return ezApplication::ApplicationExecution::Quit;

  if (m_Processor.m_OutputImage.SaveTo(m_sOutputFile).Failed())
  {
    ezLog::Error("Failed to write result to output file '{}'", m_sOutputFile);
    return ezApplication::ApplicationExecution::Quit;
  }
  else
  {
    ezLog::Success("Wrote output '{}'", m_sOutputFile);
  }


  return ezApplication::ApplicationExecution::Quit;
}

EZ_CONSOLEAPP_ENTRY_POINT(ezTexConv2);
