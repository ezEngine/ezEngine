#pragma once

#include <TexConvLib/Basics.h>

struct ezTexConvOutputType
{
  enum Enum
  {
    Texture2D,
    TextureCube,
    RenderTarget,
    Texture3D,
    DecalAtlas,

    Default = Texture2D
  };

  using StorageType = ezUInt8;
};

// TODO: Rename to something like 'quality mode' ?
// and then use
// Best (uncompressed), High (compressed), Low (compressed)
struct ezTexConvCompressionMode
{
  enum Enum
  {
    // note: order of enum values matters
    OptimizeForSize = 1,
    OptimizeForQuality = 2,
    Uncompressed = 3,

    Default = OptimizeForQuality,
  };

  using StorageType = ezUInt8;
};

struct ezTexConvUsage
{
  enum Enum
  {
    Auto, ///< Target format will be detected from heuristics (filename, content)

    // Abstract modes:
    // Exact format will be decided together with ezTexConvCompressionMode

    Color,
    Color_Hdr,

    Grayscale,
    Grayscale_Hdr,

    NormalMap,
    NormalMap_Inverted,

    // Concrete modes:
    // These formats will be mapped as closely to target platform features as possible
    // e.g. BC1-7 on D3D etc.

    Compressed_1_Channel,
    Compressed_2_Channel,
    Compressed_4_Channel,
    Compressed_4_Channel_sRGB,

    Compressed_Hdr_3_Channel,

    Uncompressed_8_Bit_UNorm_1_Channel,
    Uncompressed_8_Bit_UNorm_2_Channel,
    Uncompressed_8_Bit_UNorm_4_Channel,
    Uncompressed_8_Bit_UNorm_4_Channel_SRGB,

    Uncompressed_16_Bit_UNorm_1_Channel,
    Uncompressed_16_Bit_UNorm_2_Channel,
    Uncompressed_16_Bit_UNorm_4_Channel,

    Uncompressed_16_Bit_Float_1_Channel,
    Uncompressed_16_Bit_Float_2_Channel,
    Uncompressed_16_Bit_Float_4_Channel,

    Uncompressed_32_Bit_Float_1_Channel,
    Uncompressed_32_Bit_Float_2_Channel,
    Uncompressed_32_Bit_Float_3_Channel,
    Uncompressed_32_Bit_Float_4_Channel,

    Default = Auto
  };

  using StorageType = ezUInt8;
};

struct ezTexConvMipmapMode
{
  enum Enum
  {
    None, ///< Mipmap generation is disabled, output will have no mipmaps
    Linear,
    Kaiser,

    Default = Linear
  };

  using StorageType = ezUInt8;
};

struct ezTexConvWrapMode
{
  enum Enum
  {
    Repeat,
    Mirror,
    Clamp,

    Default = Repeat
  };

  using StorageType = ezUInt8;
};

struct ezTexConvFilterMode
{
  enum Enum
  {
    // fixed modes, use with care
    FixedNearest,
    FixedBilinear,
    FixedTrilinear,
    FixedAnisotropic2x,
    FixedAnisotropic4x,
    FixedAnisotropic8x,
    FixedAnisotropic16x,

    // modes relative to the runtime 'default' setting
    LowestQuality,
    LowQuality,
    DefaultQuality = 9,
    HighQuality,
    HighestQuality,

    Default = DefaultQuality
  };

  using StorageType = ezUInt8;
};

struct ezTexConvTargetPlatform
{
  enum Enum
  {
    PC,
    Android,

    Default = PC
  };

  using StorageType = ezUInt8;
};

/// \brief Defines which channel of another texture to read to get a value
struct ezTexConvChannelValue
{
  enum Enum
  {
    Red,   ///< read the RED channel
    Green, ///< read the GREEN channel
    Blue,  ///< read the BLUE channel
    Alpha, ///< read the ALPHA channel

    Black, ///< don't read any channel, just take the constant value 0
    White, ///< don't read any channel, just take the constant value 0xFF / 1.0f
  };
};
