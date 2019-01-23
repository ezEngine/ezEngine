#pragma once

#include <TexConvLib/Basics.h>


struct TargetFormat
{
  enum Enum
  {
    AUTO,

    COLOR,
    COLOR_HDR,
    GRAYSCALE,
    GRAYSCALE_HDR,
    NORMAL_MAP,
    NORMAL_MAP_INVERT_GREEN,

    COMPRESSED_1_CHANNEL,
    COMPRESSED_2_CHANNEL,
    COMPRESSED_4_CHANNEL,
    COMPRESSED_4_CHANNEL_SRGB,
    COMPRESSED_HDR_3_CHANNEL,
    UNCOMPRESSED_8_BIT_UNORM_1_CHANNEL,
    UNCOMPRESSED_8_BIT_UNORM_2_CHANNEL,
    UNCOMPRESSED_8_BIT_UNORM_4_CHANNEL,
    UNCOMPRESSED_8_BIT_UNORM_4_CHANNEL_SRGB,
    UNCOMPRESSED_16_BIT_UNORM_1_CHANNEL,
    UNCOMPRESSED_16_BIT_UNORM_2_CHANNEL,
    UNCOMPRESSED_16_BIT_UNORM_4_CHANNEL,
    UNCOMPRESSED_16_BIT_FLOAT_1_CHANNEL,
    UNCOMPRESSED_16_BIT_FLOAT_2_CHANNEL,
    UNCOMPRESSED_16_BIT_FLOAT_4_CHANNEL,
    UNCOMPRESSED_32_BIT_FLOAT_1_CHANNEL,
    UNCOMPRESSED_32_BIT_FLOAT_2_CHANNEL,
    UNCOMPRESSED_32_BIT_FLOAT_3_CHANNEL,
    UNCOMPRESSED_32_BIT_FLOAT_4_CHANNEL
  };
};

struct TextureWrapMode
{
  enum Enum
  {
    REPEAT = 0,
    MIRROR = 1,
    CLAMP = 2
  };
};

struct MipmapMode
{
  enum Enum
  {
    NONE,
    LINEAR = 1,
    KAISER = 2,
  };
};

struct CompressionMode
{
  enum Enum
  {
    UNCOMPRESSED,
    OPTIMIZE_FOR_SIZE,
    OPTIMIZE_FOR_QUALITY,
  };
};


struct ezTexConvDesc
{
  // input:

  // n inputs (files / black / white / color)
  // per face RGBA channel mapping: input n, RGBA selector



  // output:

  // target: 2D, 3D, Cube, decal atlas
  // TextureUsage
  // output file name
  // low res output file + resolution
  // output thumbnail file + resolution
  // min + max resolution
  // downscale factor
  // generate mipmaps
  // premultiply alpha
  // flip horizontal
  // TargetFormat
  // TextureWrapMode U, V, W
  // MipmapMode
  //  preserve coverage
  //  alpha threshold
  // CompressionMode
  // target platform: PC / Android / iOS
  // exposure bias
  // texture filter mode (aniso / linear) (fixed or relative)

  // ez specific:
  //  asset hash + version
};
