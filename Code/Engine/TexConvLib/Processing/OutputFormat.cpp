#include <PCH.h>

#include <TexConvLib/Processing/Processor.h>

ezImageFormat::Enum DetermineOutputFormatPC(
  ezTexConvUsage::Enum targetFormat, ezTexConvCompressionMode::Enum compressionMode, ezUInt32 uiNumChannels)
{
  if (targetFormat == ezTexConvUsage::NormalMap || targetFormat == ezTexConvUsage::NormalMap_Inverted)
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

    // TODO: encoder not yet supported
    // if (compressionMode >= ezTexConvCompressionMode::Medium)
    // return ezImageFormat::BC7_UNORM_SRGB;

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

        // TODO
        //if (compressionMode >= ezTexConvCompressionMode::Medium)
          //return ezImageFormat::BC7_UNORM;

        return ezImageFormat::R8G8B8A8_UNORM;

      case 4:
        // TODO
        //if (compressionMode >= ezTexConvCompressionMode::Medium)
          //return ezImageFormat::BC7_UNORM;

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

ezResult ezTexConvProcessor::ChooseOutputFormat()
{
  EZ_ASSERT_DEV(m_OutputImageFormat == ezImageFormat::UNKNOWN, "Output format already set");

  switch (m_Descriptor.m_TargetPlatform)
  {
      // case  ezTexConvTargetPlatform::Android:
      //  m_OutputImageFormat = DetermineOutputFormatAndroid(m_Descriptor.m_TargetFormat, m_Descriptor.m_CompressionMode);
      //  break;

    case ezTexConvTargetPlatform::PC:
      m_OutputImageFormat = DetermineOutputFormatPC(m_Descriptor.m_Usage, m_Descriptor.m_CompressionMode, m_uiNumChannels);
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  if (m_OutputImageFormat == ezImageFormat::UNKNOWN)
  {
    ezLog::Error("Failed to decide for an output image format.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}
