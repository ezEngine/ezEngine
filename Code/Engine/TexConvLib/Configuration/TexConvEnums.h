#pragma once

#include <TexConvLib/Basics.h>

#include <Foundation/Reflection/Reflection.h>

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

struct ezTexConvCompressionMode
{
  enum Enum
  {
    // note: order of enum values matters
    None = 0, // uncompressed
    Medium = 1, // compressed with high quality, if possible
    High = 2, // strongest compression, if possible

    Default = Medium,
  };

  using StorageType = ezUInt8;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_TEXCONV_DLL, ezTexConvCompressionMode);

struct ezTexConvUsage
{
  enum Enum
  {
    Auto, ///< Target format will be detected from heuristics (filename, content)

    // Exact format will be decided together with ezTexConvCompressionMode

    Color,
    Linear,
    Hdr,

    NormalMap,
    NormalMap_Inverted,

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

EZ_DECLARE_REFLECTABLE_TYPE(EZ_TEXCONV_DLL, ezTexConvMipmapMode);

struct ezTexConvWrapMode
{
  enum Enum
  {
    Repeat = 0,
    Mirror = 1,
    Clamp = 2,

    Default = Repeat
  };

  using StorageType = ezUInt8;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_TEXCONV_DLL, ezTexConvWrapMode);

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

EZ_DECLARE_REFLECTABLE_TYPE(EZ_TEXCONV_DLL, ezTexConvFilterMode);

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
