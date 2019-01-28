#include <PCH.h>

#include <TexConvLib/Processing/Processor.h>

ezImageFormat::Enum DetermineOutputFormatPC(ezTexConvUsage::Enum targetFormat, ezTexConvCompressionMode::Enum compressionMode)
{
  if (targetFormat == ezTexConvUsage::NormalMap || targetFormat == ezTexConvUsage::NormalMap_Inverted)
  {
    if (compressionMode == ezTexConvCompressionMode::OptimizeForSize)
      return ezImageFormat::BC5_UNORM;

    if (compressionMode == ezTexConvCompressionMode::OptimizeForQuality)
      return ezImageFormat::R8G8_UNORM;

    return ezImageFormat::R16G16_UNORM;
  }

  if (targetFormat == ezTexConvUsage::Color)
  {
    // if (compressionMode == ezTexConvCompressionMode::OptimizeForSize && alphaFormat != AlphaFormat::FULL)
    //  return ezImageFormat::BC1_UNORM_SRGB;

    // TODO: encoder not yet supported
    //if (compressionMode <= ezTexConvCompressionMode::OptimizeForQuality)
      //return ezImageFormat::BC7_UNORM_SRGB;

    return ezImageFormat::R8G8B8A8_UNORM_SRGB;
  }

  if (targetFormat == ezTexConvUsage::Color_Hdr)
  {
    // if (compressionMode == ezTexConvCompressionMode::OptimizeForSize && alphaFormat == AlphaFormat::OPAQUE)
    //  return ezImageFormat::BC6_F16_UNSIGNED;

    // if (compressionMode == ezTexConvCompressionMode::OptimizeForQuality && alphaFormat == AlphaFormat::OPAQUE)
    //  return ezImageFormat::R11_G11_B10_UNSIGNED_FLOAT;

    return ezImageFormat::R16G16B16A16_FLOAT;
  }

  if (targetFormat == ezTexConvUsage::Grayscale)
  {
    if (compressionMode <= ezTexConvCompressionMode::OptimizeForQuality)
      return ezImageFormat::BC4_UNORM;

    return ezImageFormat::R8_UNORM;
  }

  if (targetFormat == ezTexConvUsage::Grayscale_Hdr)
  {
    if (compressionMode == ezTexConvCompressionMode::OptimizeForSize)
      return ezImageFormat::BC6H_UF16;

    if (compressionMode == ezTexConvCompressionMode::OptimizeForQuality)
      return ezImageFormat::R16_FLOAT;

    return ezImageFormat::R32_FLOAT;
  }

  if (targetFormat == ezTexConvUsage::Compressed_1_Channel)
  {
    return ezImageFormat::BC4_UNORM;
  }

  if (targetFormat == ezTexConvUsage::Compressed_2_Channel)
  {
    return ezImageFormat::BC5_UNORM;
  }

  if (targetFormat == ezTexConvUsage::Compressed_4_Channel)
  {
    // if (alphaFormat != AlphaFormat::FULL)
    //  return ezImageFormat::BC1_UNORM;

    return ezImageFormat::BC7_UNORM;
  }

  if (targetFormat == ezTexConvUsage::Compressed_4_Channel_sRGB)
  {
    // if (alphaFormat != AlphaFormat::FULL)
    //  return ezImageFormat::BC1_UNORM_SRGB;

    return ezImageFormat::BC7_UNORM_SRGB;
  }

  if (targetFormat == ezTexConvUsage::Compressed_Hdr_3_Channel)
  {
    return ezImageFormat::BC6H_UF16;
  }

  if (targetFormat == ezTexConvUsage::Uncompressed_8_Bit_UNorm_1_Channel)
  {
    return ezImageFormat::R8_UNORM;
  }

  if (targetFormat == ezTexConvUsage::Uncompressed_8_Bit_UNorm_2_Channel)
  {
    return ezImageFormat::R8G8_UNORM;
  }

  if (targetFormat == ezTexConvUsage::Uncompressed_8_Bit_UNorm_4_Channel)
  {
    return ezImageFormat::R8G8B8A8_UNORM;
  }

  if (targetFormat == ezTexConvUsage::Uncompressed_8_Bit_UNorm_4_Channel_SRGB)
  {
    return ezImageFormat::R8G8B8A8_UNORM_SRGB;
  }

  if (targetFormat == ezTexConvUsage::Uncompressed_16_Bit_UNorm_1_Channel)
  {
    return ezImageFormat::R16_UNORM;
  }

  if (targetFormat == ezTexConvUsage::Uncompressed_16_Bit_UNorm_2_Channel)
  {
    return ezImageFormat::R16G16_UNORM;
  }

  if (targetFormat == ezTexConvUsage::Uncompressed_16_Bit_UNorm_4_Channel)
  {
    return ezImageFormat::R16G16B16A16_UNORM;
  }

  if (targetFormat == ezTexConvUsage::Uncompressed_16_Bit_Float_1_Channel)
  {
    return ezImageFormat::R16_FLOAT;
  }

  if (targetFormat == ezTexConvUsage::Uncompressed_16_Bit_Float_2_Channel)
  {
    return ezImageFormat::R16G16_FLOAT;
  }

  if (targetFormat == ezTexConvUsage::Uncompressed_16_Bit_Float_4_Channel)
  {
    return ezImageFormat::R16G16B16A16_FLOAT;
  }

  if (targetFormat == ezTexConvUsage::Uncompressed_32_Bit_Float_1_Channel)
  {
    return ezImageFormat::R32_FLOAT;
  }

  if (targetFormat == ezTexConvUsage::Uncompressed_32_Bit_Float_2_Channel)
  {
    return ezImageFormat::R32G32_FLOAT;
  }

  if (targetFormat == ezTexConvUsage::Uncompressed_32_Bit_Float_3_Channel)
  {
    return ezImageFormat::R32G32B32_FLOAT;
  }

  if (targetFormat == ezTexConvUsage::Uncompressed_32_Bit_Float_4_Channel)
  {
    return ezImageFormat::R32G32B32A32_FLOAT;
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
      m_OutputImageFormat = DetermineOutputFormatPC(m_Descriptor.m_Usage, m_Descriptor.m_CompressionMode);
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
