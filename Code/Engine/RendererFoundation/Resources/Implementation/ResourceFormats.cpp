#include <RendererFoundation/PCH.h>
#include <RendererFoundation/Resources/ResourceFormats.h>

const ezUInt8 ezGALResourceFormat::Size[ezGALResourceFormat::ENUM_COUNT] =
{
  16, // RGBAFloat, XYZWFloat
  16, // RGBAUInt
  16, // RGBAInt

  12, // RGBFloat, XYZFloat, UVWFloat
  12, // RGBUInt
  12, // RGBInt

  2, // B5G6R5UNormalized

  4, // BGRAUByteNormalized
  4, // BGRAUByteNormalizedsRGB

  8, // RGBAHalf, XYZWHalf
  8, // RGBAUShort
  8, // RGBAUShortNormalized
  8, // RGBAShort
  8, // RGBAShortNormalized

  8, // RGFloat, XYFloat, UVFloat
  8, // RGUInt
  8, // RGInt

  4, // RGB10A2UInt
  4, // RGB10A2UIntNormalized
  4, // RG11B10Float

  4, // RGBAUByteNormalized
  4, // RGBAUByteNormalizedsRGB
  4, // RGBAUByte
  4, // RGBAByteNormalized
  4, // RGBAByte

  4, // RGHalf, XYHalf, UVHalf
  4, // RGUShort
  4, // RGUShortNormalized
  4, // RGShort
  4, // RGShortNormalized

  2, // RGUByte
  2, // RGUByteNormalized
  2, // RGByte
  2, // RGByteNormalized

  4, // DFloat
  4, // RFloat
  4, // RUInt
  4, // RInt

  2, // RHalf
  2, // RUShort
  2, // RUShortNormalized
  2, // RShort
  2, // RShortNormalized

  1, // RUByte
  1, // RUByteNormalized
  1, // RByte
  1, // RByteNormalized
  1, // AUByteNormalized

  4, // D24S8

  // For compressed formats see: http://msdn.microsoft.com/en-us/library/windows/desktop/hh308955%28v=vs.85%29.aspx

  8, // BC1
  8, // BC1sRGB

  16, // BC2
  16, // BC2sRGB
  16, // BC3
  16, // BC3sRGB

  8, // BC4UNormalized
  8, // BC4Normalized

  16, // BC5UNormalized
  16, // BC5Normalized
  16, // BC6UFloat
  16, // BC6Float
  16, // BC7UNormalized
  16  // BC7UNormalizedsRGB
};

const ezUInt8 ezGALResourceFormat::ChannelCount[ezGALResourceFormat::ENUM_COUNT] =
{
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



EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_ResourceFormats);

