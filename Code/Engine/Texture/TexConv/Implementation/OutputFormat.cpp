#include <TexturePCH.h>

#include <Texture/TexConv/TexConvProcessor.h>

static ezImageFormat::Enum DetermineOutputFormatPC(
  ezTexConvUsage::Enum targetFormat, ezTexConvCompressionMode::Enum compressionMode, ezUInt32 uiNumChannels)
{
  if (targetFormat == ezTexConvUsage::NormalMap || targetFormat == ezTexConvUsage::NormalMap_Inverted || targetFormat == ezTexConvUsage::BumpMap)
  {
    if (compressionMode >= ezTexConvCompressionMode::High)
      return ezImageFormat::BC5_UNORM;

    if (compressionMode >= ezTexConvCompressionMode::Medium)
      return ezImageFormat::R8G8_UNORM;

    return ezImageFormat::R16G16_UNORM;
  }

  if (targetFormat == ezTexConvUsage::Color)
  {
    if (compressionMode >= ezTexConvCompressionMode::High && uiNumChannels < 4)
      return ezImageFormat::BC1_UNORM_SRGB;

    if (compressionMode >= ezTexConvCompressionMode::Medium)
      return ezImageFormat::BC7_UNORM_SRGB;

    return ezImageFormat::R8G8B8A8_UNORM_SRGB;
  }

  if (targetFormat == ezTexConvUsage::Linear)
  {
    switch (uiNumChannels)
    {
      case 1:
        if (compressionMode >= ezTexConvCompressionMode::Medium)
          return ezImageFormat::BC4_UNORM;

        return ezImageFormat::R8_UNORM;

      case 2:
        if (compressionMode >= ezTexConvCompressionMode::Medium)
          return ezImageFormat::BC5_UNORM;

        return ezImageFormat::R8G8_UNORM;

      case 3:
        if (compressionMode >= ezTexConvCompressionMode::High)
          return ezImageFormat::BC1_UNORM;

        if (compressionMode >= ezTexConvCompressionMode::Medium)
          return ezImageFormat::BC7_UNORM;

        return ezImageFormat::R8G8B8A8_UNORM;

      case 4:
        if (compressionMode >= ezTexConvCompressionMode::Medium)
          return ezImageFormat::BC7_UNORM;

        return ezImageFormat::R8G8B8A8_UNORM;

      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
    }
  }

  if (targetFormat == ezTexConvUsage::Hdr)
  {
    switch (uiNumChannels)
    {
      case 1:
        if (compressionMode >= ezTexConvCompressionMode::High)
          return ezImageFormat::BC6H_UF16;

        return ezImageFormat::R16_FLOAT;

      case 2:
        return ezImageFormat::R16G16_FLOAT;

      case 3:
        if (compressionMode >= ezTexConvCompressionMode::High)
          return ezImageFormat::BC6H_UF16;

        if (compressionMode >= ezTexConvCompressionMode::Medium)
          return ezImageFormat::R11G11B10_FLOAT;

        return ezImageFormat::R16G16B16A16_FLOAT;

      case 4:
        return ezImageFormat::R16G16B16A16_FLOAT;
    }
  }

  return ezImageFormat::UNKNOWN;
}

ezResult ezTexConvProcessor::ChooseOutputFormat(ezEnum<ezImageFormat>& out_Format, ezEnum<ezTexConvUsage> usage, ezUInt32 uiNumChannels) const
{
  EZ_ASSERT_DEV(out_Format == ezImageFormat::UNKNOWN, "Output format already set");

  switch (m_Descriptor.m_TargetPlatform)
  {
      // case  ezTexConvTargetPlatform::Android:
      //  out_Format = DetermineOutputFormatAndroid(m_Descriptor.m_TargetFormat, m_Descriptor.m_CompressionMode);
      //  break;

    case ezTexConvTargetPlatform::PC:
      out_Format = DetermineOutputFormatPC(usage, m_Descriptor.m_CompressionMode, uiNumChannels);
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  if (out_Format == ezImageFormat::UNKNOWN)
  {
    ezLog::Error("Failed to decide for an output image format.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_OutputFormat);

