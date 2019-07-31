#include <TexConv2PCH.h>

#include <TexConv2/TexConv2.h>

ezResult ezTexConv2::ParseCommandLine()
{
  auto* pCmd = ezCommandLineUtils::GetGlobalInstance();
  if (pCmd->GetBoolOption("-help") || pCmd->GetBoolOption("--help") || pCmd->GetBoolOption("-h"))
  {
    ezLog::Info("ezTexConv Help:");
    ezLog::Info("");
    ezLog::Info("  -out \"File.ext\"");
    ezLog::Info("     Absolute path to main output file");
    ezLog::Info("     ext = tga, dds, ezTexture2D, ezTexture3D, ezTextureCube or ezTextureAtlas.");
    ezLog::Info("");
    ezLog::Info("  -inX \"File\"");
    ezLog::Info("    Specifies input image X.");
    ezLog::Info("    X = 0 .. 63, e.g. -in0, -in1, etc.");
    ezLog::Info("    If X is not given, X equals 0.");
    ezLog::Info("");
    ezLog::Info("  -r / -rg / -rgb / -rgba inX.rgba");
    ezLog::Info("    Specifies how many output channels are used (1 - 4) and from which input image to take the data.");
    ezLog::Info("    Examples:");
    ezLog::Info("    -rgba in0 -> Output has 4 channels, all taken from input image 0.");
    ezLog::Info("    -rgb in0 -> Output has 3 channels, all taken from input image 0.");
    ezLog::Info("    -rgb in0 -a in1.r -> Output has 4 channels, RGB taken from input image 0 (RGB) Alpha taken from input 1 (Red).");
    ezLog::Info("    -rgb in0.bgr -> Output has 3 channels, taken from image 0 and swapped blue and red.");
    ezLog::Info("    -r in0.r -g in1.r -b in2.r -a in3.r -> Output has 4 channels, each one taken from another input image (Red).");
    ezLog::Info("    -rgb0 in0 -rgb1 in1 -rgb2 in2 -rgb3 in3 -rgb4 in4 -rgb5 in5 -> Output has 3 channels and six faces (-type Cubemap), built from 6 images.");

    ezLog::Info("");
    ezLog::Info("  -thumbnailOut \"File.ext\"");
    ezLog::Info("     Absolute path to 2D thumbnail file.");
    ezLog::Info("     ext = tga, jpg, png");
    ezLog::Info("  -thumbnailRes Number");
    ezLog::Info("     Thumbnail resolution. Should be a power-of-two.");
    ezLog::Info("");
    ezLog::Info("  -lowOut \"File.ext\"");
    ezLog::Info("     Absolute path to low-resolution output file.");
    ezLog::Info("     ext = Same as main output");
    ezLog::Info("  -lowMips Number");
    ezLog::Info("     Number of mipmaps to use from main result as low-res data.");
    ezLog::Info("");
    PrintOptionValuesHelp("  -type", m_AllowedOutputTypes);
    ezLog::Info("     The type of output to generate.");
    ezLog::Info("");
    ezLog::Info("  -assetVersion Number");
    ezLog::Info("     Asset version number to embed in ez specific output formats.");
    ezLog::Info("  -assetHashLow 0xHEX_VALUE / -assetHashHigh 0xHEX_VALUE");
    ezLog::Info("     Low and high part of a 64 bit asset hash value.");
    ezLog::Info("     Required to be non-zero when using ez specific output formats.");
    ezLog::Info("");
    PrintOptionValuesHelp("  -compression", m_AllowedCompressionModes);
    ezLog::Info("     Compression strength for output format.");
    ezLog::Info("");
    PrintOptionValuesHelp("  -usage", m_AllowedUsages);
    ezLog::Info("     What type of data the image contains. Affects which final output format is used and how mipmaps are generated.");
    ezLog::Info("");
    PrintOptionValuesHelp("  -mipmaps", m_AllowedMimapModes);
    ezLog::Info("     Whether to generate mipmaps and with which algorithm.");
    ezLog::Info("  -mipsPerserveCoverage");
    ezLog::Info("     Whether to preserve alpha-coverage in mipmaps for alpha-tested geometry.");
    ezLog::Info("  -mipsAlphaThreshold float [0; 1]");
    ezLog::Info("     Alpha threshold used by renderer for alpha-testing, when alpha-coverage should be preserved.");
    ezLog::Info("");
    PrintOptionValuesHelp("  -addressU/V/W", m_AllowedWrapModes);
    ezLog::Info("     Which texture address mode to use along U/V/W. Only supported by ez-specific output formats.");
    PrintOptionValuesHelp("  -filter", m_AllowedFilterModes);
    ezLog::Info("     Which texture filter mode to use at runtime. Only supported by ez-specific output formats.");
    ezLog::Info("");
    ezLog::Info("  -minRes / -maxRes Number");
    ezLog::Info("    The minimum and maximum resolution allowed for the output.");
    ezLog::Info("  -downscale Number");
    ezLog::Info("    How often to half the input texture resolution.");
    ezLog::Info("");
    ezLog::Info("  -flip_horz");
    ezLog::Info("    Whether to flip the output horizontally.");
    ezLog::Info("  -premulalpha");
    ezLog::Info("    Whether to multiply the alpha channel into the RGB channels.");
    ezLog::Info("  -hdrExposure float [-20;+20]");
    ezLog::Info("    For scaling HDR image brightness up or down.");
    ezLog::Info("  -clamp float");
    ezLog::Info("    Input values will be clamped to [-value;+value] (default 64000).");
    PrintOptionValuesHelp("  -bumpMapFilter", m_AllowedBumpMapFilters);
    ezLog::Info("    Filter used to approximate the x/y bump map gradients.");

    return EZ_FAILURE;
  }

  EZ_SUCCEED_OR_RETURN(ParseOutputFiles());
  EZ_SUCCEED_OR_RETURN(DetectOutputFormat());

  EZ_SUCCEED_OR_RETURN(ParseOutputType());
  EZ_SUCCEED_OR_RETURN(ParseAssetHeader());
  EZ_SUCCEED_OR_RETURN(ParseTargetPlatform());
  EZ_SUCCEED_OR_RETURN(ParseCompressionMode());
  EZ_SUCCEED_OR_RETURN(ParseUsage());
  EZ_SUCCEED_OR_RETURN(ParseMipmapMode());
  EZ_SUCCEED_OR_RETURN(ParseWrapModes());
  EZ_SUCCEED_OR_RETURN(ParseFilterModes());
  EZ_SUCCEED_OR_RETURN(ParseResolutionModifiers());
  EZ_SUCCEED_OR_RETURN(ParseMiscOptions());
  EZ_SUCCEED_OR_RETURN(ParseInputFiles());
  EZ_SUCCEED_OR_RETURN(ParseChannelMappings());
  EZ_SUCCEED_OR_RETURN(ParseBumpMapFilter());

  return EZ_SUCCESS;
}

ezResult ezTexConv2::ParseOutputType()
{
  if (m_sOutputFile.IsEmpty())
  {
    m_Processor.m_Descriptor.m_OutputType = ezTexConvOutputType::None;
    return EZ_SUCCESS;
  }

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
  else if (m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::Cubemap)
  {
    if (!m_bOutputSupportsCube)
    {
      ezLog::Error("Cubemap textures are not supported by the chosen output file format.");
      return EZ_FAILURE;
    }
  }
  else if (m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::Atlas)
  {
    if (!m_bOutputSupportsAtlas)
    {
      ezLog::Error("Atlas textures are not supported by the chosen output file format.");
      return EZ_FAILURE;
    }

    if (!ParseFile("-atlasDesc", m_Processor.m_Descriptor.m_sTextureAtlasDescFile))
      return EZ_FAILURE;
  }
  else if (m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::Volume)
  {
    if (!m_bOutputSupports3D)
    {
      ezLog::Error("Volume textures are not supported by the chosen output file format.");
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

ezResult ezTexConv2::ParseInputFiles()
{
  if (m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::Atlas)
    return EZ_SUCCESS;

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

  if (m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::Cubemap)
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
  ParseFile("-out", m_sOutputFile);

  if (ParseFile("-thumbnailOut", m_sOutputThumbnailFile))
  {
    EZ_SUCCEED_OR_RETURN(ParseUIntOption("-thumbnailRes", 32, 1024, m_Processor.m_Descriptor.m_uiThumbnailOutputResolution));
  }

  if (ParseFile("-lowOut", m_sOutputLowResFile))
  {
    EZ_SUCCEED_OR_RETURN(ParseUIntOption("-lowMips", 0, 8, m_Processor.m_Descriptor.m_uiLowResMipmaps));
  }

  return EZ_SUCCESS;
}

ezResult ezTexConv2::ParseUsage()
{
  if (m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::Atlas)
    return EZ_SUCCESS;

  ezInt32 value = -1;
  EZ_SUCCEED_OR_RETURN(ParseStringOption("-usage", m_AllowedUsages, value));

  m_Processor.m_Descriptor.m_Usage = static_cast<ezTexConvUsage::Enum>(value);
  return EZ_SUCCESS;
}

ezResult ezTexConv2::ParseMipmapMode()
{
  if (!m_bOutputSupportsMipmaps)
  {
    m_Processor.m_Descriptor.m_MipmapMode = ezTexConvMipmapMode::None;
    return EZ_SUCCESS;
  }

  ezInt32 value = -1;
  EZ_SUCCEED_OR_RETURN(ParseStringOption("-mipmaps", m_AllowedMimapModes, value));
  m_Processor.m_Descriptor.m_MipmapMode = static_cast<ezTexConvMipmapMode::Enum>(value);


  EZ_SUCCEED_OR_RETURN(ParseBoolOption("-mipsPerserveCoverage", m_Processor.m_Descriptor.m_bPreserveMipmapCoverage));

  if (m_Processor.m_Descriptor.m_bPreserveMipmapCoverage)
  {
    EZ_SUCCEED_OR_RETURN(ParseFloatOption("-mipsAlphaThreshold", 0.01f, 0.99f, m_Processor.m_Descriptor.m_fMipmapAlphaThreshold));
  }

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
    m_Processor.m_Descriptor.m_CompressionMode = ezTexConvCompressionMode::None;
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
  if (m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::Cubemap ||
      m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::Atlas ||
      m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::None)
    return EZ_SUCCESS;

  {
    ezInt32 value = -1;
    EZ_SUCCEED_OR_RETURN(ParseStringOption("-addressU", m_AllowedWrapModes, value));
    m_Processor.m_Descriptor.m_AddressModeU = static_cast<ezImageAddressMode::Enum>(value);
  }
  {
    ezInt32 value = -1;
    EZ_SUCCEED_OR_RETURN(ParseStringOption("-addressV", m_AllowedWrapModes, value));
    m_Processor.m_Descriptor.m_AddressModeV = static_cast<ezImageAddressMode::Enum>(value);
  }

  if (m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::Volume)
  {
    ezInt32 value = -1;
    EZ_SUCCEED_OR_RETURN(ParseStringOption("-addressW", m_AllowedWrapModes, value));
    m_Processor.m_Descriptor.m_AddressModeW = static_cast<ezImageAddressMode::Enum>(value);
  }

  return EZ_SUCCESS;
}

ezResult ezTexConv2::ParseFilterModes()
{
  if (!m_bOutputSupportsFiltering)
    return EZ_SUCCESS;

  ezInt32 value = -1;
  EZ_SUCCEED_OR_RETURN(ParseStringOption("-filter", m_AllowedFilterModes, value));

  m_Processor.m_Descriptor.m_FilterMode = static_cast<ezTextureFilterSetting::Enum>(value);
  return EZ_SUCCESS;
}

ezResult ezTexConv2::ParseResolutionModifiers()
{
  if (m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::None)
    return EZ_SUCCESS;

  EZ_SUCCEED_OR_RETURN(ParseUIntOption("-minRes", 4, 8 * 1024, m_Processor.m_Descriptor.m_uiMinResolution));
  EZ_SUCCEED_OR_RETURN(ParseUIntOption("-maxRes", 4, 16 * 1024, m_Processor.m_Descriptor.m_uiMaxResolution));
  EZ_SUCCEED_OR_RETURN(ParseUIntOption("-downscale", 0, 10, m_Processor.m_Descriptor.m_uiDownscaleSteps));

  return EZ_SUCCESS;
}

ezResult ezTexConv2::ParseMiscOptions()
{
  if (m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::Texture2D ||
      m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::None)
  {
    EZ_SUCCEED_OR_RETURN(ParseBoolOption("-flip_horz", m_Processor.m_Descriptor.m_bFlipHorizontal));
    EZ_SUCCEED_OR_RETURN(ParseBoolOption("-premulalpha", m_Processor.m_Descriptor.m_bPremultiplyAlpha));
  }

  if (m_Processor.m_Descriptor.m_Usage == ezTexConvUsage::Hdr)
  {
    EZ_SUCCEED_OR_RETURN(ParseFloatOption("-hdrExposure", -20.0f, 20.0f, m_Processor.m_Descriptor.m_fHdrExposureBias));
  }

  EZ_SUCCEED_OR_RETURN(ParseFloatOption("-clamp", -64000.f, 64000.f, m_Processor.m_Descriptor.m_fMaxValue));

  return EZ_SUCCESS;
}

ezResult ezTexConv2::ParseAssetHeader()
{
  const ezStringView ext = ezPathUtils::GetFileExtension(m_sOutputFile);

  if (!ext.StartsWith_NoCase("ez"))
    return EZ_SUCCESS;

  const auto pCmd = ezCommandLineUtils::GetGlobalInstance();

  ezUInt32 tmp = m_Processor.m_Descriptor.m_uiAssetVersion;
  EZ_SUCCEED_OR_RETURN(ParseUIntOption("-assetVersion", 1, 0xFFFF, tmp));
  m_Processor.m_Descriptor.m_uiAssetVersion = tmp;

  const ezUInt64 uiHashLow = ezConversionUtils::ConvertHexStringToUInt32(pCmd->GetStringOption("-assetHashLow"));
  const ezUInt64 uiHashHigh = ezConversionUtils::ConvertHexStringToUInt32(pCmd->GetStringOption("-assetHashHigh"));

  m_Processor.m_Descriptor.m_uiAssetHash = (uiHashHigh << 32) | uiHashLow;

  if (m_Processor.m_Descriptor.m_uiAssetHash == 0)
  {
    ezLog::Error("'-assetHashLow 0xHEX32' and '-assetHashHigh 0xHEX32' have not been specified.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezTexConv2::ParseBumpMapFilter()
{
  ezInt32 value = -1;
  EZ_SUCCEED_OR_RETURN(ParseStringOption("-bumpMapFilter", m_AllowedBumpMapFilters, value));

  m_Processor.m_Descriptor.m_BumpMapFilter = static_cast<ezTexConvBumpMapFilter::Enum>(value);
  return EZ_SUCCESS;
}
