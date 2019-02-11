#pragma once

#include <Texture/TextureDLL.h>

#include <Foundation/Reflection/Reflection.h>

struct ezTexConvOutputType
{
  enum Enum
  {
    None,
    Texture2D,
    Volume,
    Cubemap,
    Atlas,

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

EZ_DECLARE_REFLECTABLE_TYPE(EZ_TEXTURE_DLL, ezTexConvCompressionMode);

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

EZ_DECLARE_REFLECTABLE_TYPE(EZ_TEXTURE_DLL, ezTexConvUsage);

struct ezTexConvMipmapMode
{
  enum Enum
  {
    None, ///< Mipmap generation is disabled, output will have no mipmaps
    Linear,
    Kaiser,

    Default = Kaiser
  };

  using StorageType = ezUInt8;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_TEXTURE_DLL, ezTexConvMipmapMode);

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

