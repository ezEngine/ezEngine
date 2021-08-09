#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/ResourceFormats.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezGALResourceFormat, 1)
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAFloat),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAUInt),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAInt),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBFloat),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBUInt),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBInt),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::B5G6R5UNormalized),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::BGRAUByteNormalized),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::BGRAUByteNormalizedsRGB),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAHalf),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAUShort),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAUShortNormalized),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAShort),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAShortNormalized),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGFloat),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGUInt),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGInt),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGB10A2UInt),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGB10A2UIntNormalized),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RG11B10Float),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAUByteNormalized),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAUByteNormalizedsRGB),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAUByte),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAByteNormalized),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGBAByte),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGHalf),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGUShort),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGUShortNormalized),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGShort),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGShortNormalized),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGUByte),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGUByteNormalized),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGByte),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RGByteNormalized),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::DFloat),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RFloat),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RUInt),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RInt),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RHalf),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RUShort),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RUShortNormalized),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RShort),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RShortNormalized),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RUByte),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RUByteNormalized),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RByte),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::RByteNormalized),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::AUByteNormalized),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::D16),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::D24S8),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::BC1),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::BC1sRGB),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::BC2),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::BC2sRGB),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::BC3),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::BC3sRGB),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::BC4UNormalized),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::BC4Normalized),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::BC5UNormalized),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::BC5Normalized),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::BC6UFloat),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::BC6Float),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::BC7UNormalized),
  EZ_ENUM_CONSTANT(ezGALResourceFormat::BC7UNormalizedsRGB)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
const ezUInt8 ezGALResourceFormat::s_BitsPerElement[ezGALResourceFormat::ENUM_COUNT] =
{
  0, // Invalid

  128, // RGBAFloat, XYZWFloat
  128, // RGBAUInt
  128, // RGBAInt

  96, // RGBFloat, XYZFloat, UVWFloat
  96, // RGBUInt
  96, // RGBInt

  16, // B5G6R5UNormalized

  32, // BGRAUByteNormalized
  32, // BGRAUByteNormalizedsRGB

  64, // RGBAHalf, XYZWHalf
  64, // RGBAUShort
  64, // RGBAUShortNormalized
  64, // RGBAShort
  64, // RGBAShortNormalized

  64, // RGFloat, XYFloat, UVFloat
  64, // RGUInt
  64, // RGInt

  32, // RGB10A2UInt
  32, // RGB10A2UIntNormalized
  32, // RG11B10Float

  32, // RGBAUByteNormalized
  32, // RGBAUByteNormalizedsRGB
  32, // RGBAUByte
  32, // RGBAByteNormalized
  32, // RGBAByte

  32, // RGHalf, XYHalf, UVHalf
  32, // RGUShort
  32, // RGUShortNormalized
  32, // RGShort
  32, // RGShortNormalized

  16, // RGUByte
  16, // RGUByteNormalized
  16, // RGByte
  16, // RGByteNormalized

  32, // DFloat
  32, // RFloat
  32, // RUInt
  32, // RInt

  16, // RHalf
  16, // RUShort
  16, // RUShortNormalized
  16, // RShort
  16, // RShortNormalized

  8, // RUByte
  8, // RUByteNormalized
  8, // RByte
  8, // RByteNormalized
  8, // AUByteNormalized

  16, // D16
  32, // D24S8

  // For compressed formats see: http://msdn.microsoft.com/en-us/library/windows/desktop/hh308955%28v=vs.85%29.aspx

  4, // BC1
  4, // BC1sRGB

  8, // BC2
  8, // BC2sRGB
  8, // BC3
  8, // BC3sRGB

  4, // BC4UNormalized
  4, // BC4Normalized

  8, // BC5UNormalized
  8, // BC5Normalized
  8, // BC6UFloat
  8, // BC6Float
  8, // BC7UNormalized
  8  // BC7UNormalizedsRGB
};

const ezUInt8 ezGALResourceFormat::s_ChannelCount[ezGALResourceFormat::ENUM_COUNT] =
{
  0, // Invalid

  4, // RGBAFloat, XYZWFloat
  4, // RGBAUInt
  4, // RGBAInt

  3, // RGBFloat, XYZFloat, UVWFloat
  3, // RGBUInt
  3, // RGBInt

  3, // B5G6R5UNormalized

  4, // BGRAUByteNormalized
  4, // BGRAUByteNormalizedsRGB

  4, // RGBAHalf, XYZWHalf
  4, // RGBAUShort
  4, // RGBAUShortNormalized
  4, // RGBAShort
  4, // RGBAShortNormalized

  2, // RGFloat, XYFloat, UVFloat
  2, // RGUInt
  2, // RGInt

  4, // RGB10A2UInt
  4, // RGB10A2UIntNormalized
  3, // RG11B10Float

  4, // RGBAUByteNormalized
  4, // RGBAUByteNormalizedsRGB
  4, // RGBAUByte
  4, // RGBAByteNormalized
  4, // RGBAByte

  2, // RGHalf, XYHalf, UVHalf
  2, // RGUShort
  2, // RGUShortNormalized
  2, // RGShort
  2, // RGShortNormalized

  2, // RGUByte
  2, // RGUByteNormalized
  2, // RGByte
  2, // RGByteNormalized

  1, // DFloat
  1, // RFloat
  1, // RUInt
  1, // RInt

  1, // RHalf
  1, // RUShort
  1, // RUShortNormalized
  1, // RShort
  1, // RShortNormalized

  1, // RUByte
  1, // RUByteNormalized
  1, // RByte
  1, // RByteNormalized
  1, // AUByteNormalized

  1, // D16
  2, // D24S8

  // For compressed formats see: http://msdn.microsoft.com/en-us/library/windows/desktop/hh308955%28v=vs.85%29.aspx

  4, // BC1
  4, // BC1sRGB
  4, // BC2
  4, // BC2sRGB
  4, // BC3
  4, // BC3sRGB
  1, // BC4UNormalized
  1, // BC4Normalized
  2, // BC5UNormalized
  2, // BC5Normalized
  3, // BC6UFloat
  3, // BC6Float
  4, // BC7UNormalized
  4  // BC7UNormalizedsRGB
};
// clang-format off


EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_ResourceFormats);

