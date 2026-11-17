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
    Texture2DArray,

    Default = Texture2D
  };

  using StorageType = ezUInt8;
};

struct ezTexConvCompressionMode
{
  enum Enum
  {
    // note: order of enum values matters
    None = 0,   // uncompressed
    Medium = 1, // compressed with high quality, if possible
    High = 2,   // strongest compression, if possible

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

    NormalMap,          // DirectX convention
    NormalMap_Inverted, // OpenGL convention

    BumpMap,

    /// The input is a height map (red channel, white = top). The output stores the height in red and the
    /// relaxed cone ratio in green, for relaxed cone step parallax mapping. Only supported for 2D textures.
    /// The cone ratios are computed by ray marching the height field. That is expensive for large textures,
    /// from seconds for 512x512 up to minutes for 2048x2048, depending on the content.
    ConeMap,

    /// Same output as ConeMap, but the cone ratios are computed with a local slope test instead of ray marching, which is
    /// much faster. The test is more conservative, so the cones are narrower, especially for noisy height maps. Narrower
    /// cones need more steps in the shader for the same quality.
    ConeMapFast,

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

/// Defines which channel of another texture to read to get a value
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

/// Defines which filter kernel is used to approximate the x/y bump map gradients
struct ezTexConvBumpMapFilter
{
  enum Enum
  {
    Finite, ///< Simple finite differences in a 4-Neighborhood
    Sobel,  ///< Sobel kernel (8-Neighborhood)
    Scharr, ///< Scharr kernel (8-Neighborhood)

    Default = Finite
  };

  using StorageType = ezUInt8;
};
