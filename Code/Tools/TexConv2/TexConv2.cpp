#include <PCH.h>

#include <TexConv2/TexConv2.h>

/* TODO LIST:

- cubemaps:
- from single cubemap dds
- from multiple cubemap dds
- from multiple 2d textures
- from single 2d texture

- flip horizontal for 2D
- decal atlas support
- target platform switch
- low res output
- thumbnail output
- compression mode
- BC7 compression support
- texture filtering
- texture wrap modes
- premultiply alpha
- hdr exposure bias
- ez texture output format
- volume textures
- normal map from heightmap ?
- mipmap mode
- min / max resolution
- downscale steps
- preserve mipmap coverage
- mipmap alpha threshold
- asset hash

- docs for params / help

*/


ezTexConv2::ezTexConv2()
    : ezApplication("TexConv2")
{
  {
    m_AllowedUsages.PushBack({"Auto", ezTexConvUsage::Auto}); // default value
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

  {
    m_AllowedMimapModes.PushBack({"Linear", ezTexConvMipmapMode::Linear}); // default value
    m_AllowedMimapModes.PushBack({"Kaiser", ezTexConvMipmapMode::Kaiser});
    m_AllowedMimapModes.PushBack({"None", ezTexConvMipmapMode::None});
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

ezResult ezTexConv2::ParseCommandLine()
{
  const auto pCmd = ezCommandLineUtils::GetGlobalInstance();

  if (pCmd->GetOptionIndex("-cubemap") >= 0)
  {
    m_Processor.m_Descriptor.m_OutputType = ezTexConvOutputType::TextureCube;
    ezLog::Info("Output type: Cubemap");
  }
  else
  {
    m_Processor.m_Descriptor.m_OutputType = ezTexConvOutputType::Texture2D;
    ezLog::Info("Output type: 2D Texture");
  }

  EZ_SUCCEED_OR_RETURN(ParseOutputFiles());
  EZ_SUCCEED_OR_RETURN(ParseUsage());
  EZ_SUCCEED_OR_RETURN(ParseMipmapMode());
  EZ_SUCCEED_OR_RETURN(ParseInputFiles());
  EZ_SUCCEED_OR_RETURN(ParseChannelMappings());

  return EZ_SUCCESS;
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
    return EZ_SUCCESS;
  }

  for (ezUInt32 i = 0; i < allowed.GetCount(); ++i)
  {
    if (sValue.IsEqual_NoCase(allowed[i].m_szKey))
    {
      iResult = allowed[i].m_iEnumValue;

      ezLog::Info("Selected {}: '{}' ({})", szOption, allowed[i].m_szKey, allowed[i].m_iEnumValue);
      return EZ_SUCCESS;
    }
  }

  ezLog::Error("Unknown {}: '{}'.", szOption, sValue);

  ezLog::Info("Available {} options are:", szOption);

  for (ezUInt32 i = 0; i < allowed.GetCount(); ++i)
  {
    ezLog::Info("  {}", allowed[i].m_szKey);
  }

  return EZ_FAILURE;
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
  ezInt32 value = -1;
  EZ_SUCCEED_OR_RETURN(ParseStringOption("-mipmaps", m_AllowedMimapModes, value));

  m_Processor.m_Descriptor.m_MipmapMode = static_cast<ezTexConvMipmapMode::Enum>(value);
  return EZ_SUCCESS;
}

ezApplication::ApplicationExecution ezTexConv2::Run()
{
  if (ParseCommandLine().Failed())
    return ezApplication::ApplicationExecution::Quit;

  if (m_Processor.Process().Failed())
    return ezApplication::ApplicationExecution::Quit;

  m_Processor.m_OutputImage.SaveTo(m_sOutputFile);


  return ezApplication::ApplicationExecution::Quit;
}

EZ_CONSOLEAPP_ENTRY_POINT(ezTexConv2);
