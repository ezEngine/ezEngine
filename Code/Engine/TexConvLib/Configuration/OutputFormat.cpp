#include <PCH.h>

#include <TexConvLib/Configuration/TexConvState.h>

ezImageFormat::Enum DetermineOutputFormatDefault(ezTexConvTargetFormat::Enum targetFormat, ezTexConvCompressionMode::Enum compressionMode)
{
  if (targetFormat == ezTexConvTargetFormat::NormalMap || targetFormat == ezTexConvTargetFormat::NormalMap_Inverted)
  {
    if (compressionMode == ezTexConvCompressionMode::OptimizeForSize)
      return ezImageFormat::BC5_UNORM;

    if (compressionMode == ezTexConvCompressionMode::OptimizeForQuality)
      return ezImageFormat::R8G8_UNORM;

    return ezImageFormat::R16G16_UNORM;
  }

  if (targetFormat == ezTexConvTargetFormat::Color)
  {
    // if (compressionMode == ezTexConvCompressionMode::OptimizeForSize && alphaFormat != AlphaFormat::FULL)
    //  return ezImageFormat::BC1_UNORM_SRGB;

    if (compressionMode <= ezTexConvCompressionMode::OptimizeForQuality)
      return ezImageFormat::BC7_UNORM_SRGB;

    return ezImageFormat::R8G8B8A8_UNORM_SRGB;
  }

  if (targetFormat == ezTexConvTargetFormat::Color_Hdr)
  {
    // if (compressionMode == ezTexConvCompressionMode::OptimizeForSize && alphaFormat == AlphaFormat::OPAQUE)
    //  return ezImageFormat::BC6_F16_UNSIGNED;

    // if (compressionMode == ezTexConvCompressionMode::OptimizeForQuality && alphaFormat == AlphaFormat::OPAQUE)
    //  return ezImageFormat::R11_G11_B10_UNSIGNED_FLOAT;

    return ezImageFormat::R16G16B16A16_FLOAT;
  }

  if (targetFormat == ezTexConvTargetFormat::Grayscale)
  {
    if (compressionMode <= ezTexConvCompressionMode::OptimizeForQuality)
      return ezImageFormat::BC4_UNORM;

    return ezImageFormat::R8_UNORM;
  }

  if (targetFormat == ezTexConvTargetFormat::Grayscale_Hdr)
  {
    if (compressionMode == ezTexConvCompressionMode::OptimizeForSize)
      return ezImageFormat::BC6H_UF16;

    if (compressionMode == ezTexConvCompressionMode::OptimizeForQuality)
      return ezImageFormat::R16_FLOAT;

    return ezImageFormat::R32_FLOAT;
  }

  if (targetFormat == ezTexConvTargetFormat::Compressed_1_Channel)
  {
    return ezImageFormat::BC4_UNORM;
  }

  if (targetFormat == ezTexConvTargetFormat::Compressed_2_Channel)
  {
    return ezImageFormat::BC5_UNORM;
  }

  if (targetFormat == ezTexConvTargetFormat::Compressed_4_Channel)
  {
    // if (alphaFormat != AlphaFormat::FULL)
    //  return ezImageFormat::BC1_UNORM;

    return ezImageFormat::BC7_UNORM;
  }

  if (targetFormat == ezTexConvTargetFormat::Compressed_4_Channel_sRGB)
  {
    // if (alphaFormat != AlphaFormat::FULL)
    //  return ezImageFormat::BC1_UNORM_SRGB;

    return ezImageFormat::BC7_UNORM_SRGB;
  }

  if (targetFormat == ezTexConvTargetFormat::Compressed_Hdr_3_Channel)
  {
    return ezImageFormat::BC6H_UF16;
  }

  if (targetFormat == ezTexConvTargetFormat::Uncompressed_8_Bit_UNorm_1_Channel)
  {
    return ezImageFormat::R8_UNORM;
  }

  if (targetFormat == ezTexConvTargetFormat::Uncompressed_8_Bit_UNorm_2_Channel)
  {
    return ezImageFormat::R8G8_UNORM;
  }

  if (targetFormat == ezTexConvTargetFormat::Uncompressed_8_Bit_UNorm_4_Channel)
  {
    return ezImageFormat::R8G8B8A8_UNORM;
  }

  if (targetFormat == ezTexConvTargetFormat::Uncompressed_8_Bit_UNorm_4_Channel_SRGB)
  {
    return ezImageFormat::R8G8B8A8_UNORM_SRGB;
  }

  if (targetFormat == ezTexConvTargetFormat::Uncompressed_16_Bit_UNorm_1_Channel)
  {
    return ezImageFormat::R16_UNORM;
  }

  if (targetFormat == ezTexConvTargetFormat::Uncompressed_16_Bit_UNorm_2_Channel)
  {
    return ezImageFormat::R16G16_UNORM;
  }

  if (targetFormat == ezTexConvTargetFormat::Uncompressed_16_Bit_UNorm_4_Channel)
  {
    return ezImageFormat::R16G16B16A16_UNORM;
  }

  if (targetFormat == ezTexConvTargetFormat::Uncompressed_16_Bit_Float_1_Channel)
  {
    return ezImageFormat::R16_FLOAT;
  }

  if (targetFormat == ezTexConvTargetFormat::Uncompressed_16_Bit_Float_2_Channel)
  {
    return ezImageFormat::R16G16_FLOAT;
  }

  if (targetFormat == ezTexConvTargetFormat::Uncompressed_16_Bit_Float_4_Channel)
  {
    return ezImageFormat::R16G16B16A16_FLOAT;
  }

  if (targetFormat == ezTexConvTargetFormat::Uncompressed_32_Bit_Float_1_Channel)
  {
    return ezImageFormat::R32_FLOAT;
  }

  if (targetFormat == ezTexConvTargetFormat::Uncompressed_32_Bit_Float_2_Channel)
  {
    return ezImageFormat::R32G32_FLOAT;
  }

  if (targetFormat == ezTexConvTargetFormat::Uncompressed_32_Bit_Float_3_Channel)
  {
    return ezImageFormat::R32G32B32_FLOAT;
  }

  if (targetFormat == ezTexConvTargetFormat::Uncompressed_32_Bit_Float_4_Channel)
  {
    return ezImageFormat::R32G32B32A32_FLOAT;
  }

  return ezImageFormat::UNKNOWN;
}

void ezTexConvState::ChooseOutputFormat()
{
  m_OutputImageFormat = DetermineOutputFormatDefault(m_Descriptor.m_TargetFormat, m_Descriptor.m_CompressionMode);
}
