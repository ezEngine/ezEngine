#include <PCH.h>

#include <TexConv2/TexConv2.h>

ezResult ezTexConv2::ParseCommandLine()
{
  EZ_SUCCEED_OR_RETURN(ParseOutputFiles());
  EZ_SUCCEED_OR_RETURN(DetectOutputFormat());

  EZ_SUCCEED_OR_RETURN(ParseAssetHeader());
  EZ_SUCCEED_OR_RETURN(ParseTargetPlatform());
  EZ_SUCCEED_OR_RETURN(ParseOutputType());
  EZ_SUCCEED_OR_RETURN(ParseCompressionMode());
  EZ_SUCCEED_OR_RETURN(ParseUsage());
  EZ_SUCCEED_OR_RETURN(ParseMipmapMode());
  EZ_SUCCEED_OR_RETURN(ParseWrapModes());
  EZ_SUCCEED_OR_RETURN(ParseFilterModes());
  EZ_SUCCEED_OR_RETURN(ParseResolutionModifiers());
  EZ_SUCCEED_OR_RETURN(ParseMiscOptions());
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
  ezInt32 value = -1;
  EZ_SUCCEED_OR_RETURN(ParseStringOption("-usage", m_AllowedUsages, value));

  m_Processor.m_Descriptor.m_Usage = static_cast<ezTexConvUsage::Enum>(value);
  return EZ_SUCCESS;
}

ezResult ezTexConv2::ParseMipmapMode()
{
  if (m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::Texture3D)
    return EZ_SUCCESS;

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
    EZ_SUCCEED_OR_RETURN(ParseStringOption("-addressU", m_AllowedWrapModes, value));
    m_Processor.m_Descriptor.m_WrapModes[0] = static_cast<ezTexConvWrapMode::Enum>(value);
  }
  {
    ezInt32 value = -1;
    EZ_SUCCEED_OR_RETURN(ParseStringOption("-addressV", m_AllowedWrapModes, value));
    m_Processor.m_Descriptor.m_WrapModes[1] = static_cast<ezTexConvWrapMode::Enum>(value);
  }

  if (m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::Texture3D)
  {
    ezInt32 value = -1;
    EZ_SUCCEED_OR_RETURN(ParseStringOption("-addressW", m_AllowedWrapModes, value));
    m_Processor.m_Descriptor.m_WrapModes[2] = static_cast<ezTexConvWrapMode::Enum>(value);
  }

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

ezResult ezTexConv2::ParseResolutionModifiers()
{
  if (m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::RenderTarget)
    return EZ_SUCCESS;

  EZ_SUCCEED_OR_RETURN(ParseUIntOption("-minRes", 4, 8 * 1024, m_Processor.m_Descriptor.m_uiMinResolution));
  EZ_SUCCEED_OR_RETURN(ParseUIntOption("-maxRes", 4, 16 * 1024, m_Processor.m_Descriptor.m_uiMaxResolution));
  EZ_SUCCEED_OR_RETURN(ParseUIntOption("-downscale", 0, 10, m_Processor.m_Descriptor.m_uiDownscaleSteps));

  return EZ_SUCCESS;
}

ezResult ezTexConv2::ParseMiscOptions()
{
  if (m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::Texture2D)
  {
    EZ_SUCCEED_OR_RETURN(ParseBoolOption("-flip_horz", m_Processor.m_Descriptor.m_bFlipHorizontal));
    EZ_SUCCEED_OR_RETURN(ParseBoolOption("-premulalpha", m_Processor.m_Descriptor.m_bPremultiplyAlpha));
  }

  if (m_Processor.m_Descriptor.m_Usage == ezTexConvUsage::Hdr)
  {
    EZ_SUCCEED_OR_RETURN(ParseFloatOption("-hdrExposure", -20.0f, 20.0f, m_Processor.m_Descriptor.m_fHdrExposureBias));
  }

  return EZ_SUCCESS;
}

ezResult ezTexConv2::ParseAssetHeader()
{
  const ezStringView ext = ezPathUtils::GetFileExtension(m_sOutputFile);

  if (!ext.StartsWith_NoCase("ez"))
    return EZ_SUCCESS;

  const auto pCmd = ezCommandLineUtils::GetGlobalInstance();

  EZ_SUCCEED_OR_RETURN(ParseUIntOption("-assetVersion", 1, 255, m_uiEzFormatAssetVersion));

  const ezUInt64 uiHashLow = ezConversionUtils::ConvertHexStringToUInt32(pCmd->GetStringOption("-assetHashLow"));
  const ezUInt64 uiHashHigh = ezConversionUtils::ConvertHexStringToUInt32(pCmd->GetStringOption("-assetHashHigh"));

  m_uiEzFormatAssetHash = (uiHashHigh << 32) | uiHashLow;

  if (m_uiEzFormatAssetHash == 0)
  {
    ezLog::Error("'-assetHashLow 0xHEX32' and '-assetHashHigh 0xHEX32' have not been specified.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}
