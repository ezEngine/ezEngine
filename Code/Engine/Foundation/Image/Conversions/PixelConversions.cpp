#include <PCH.h>

#include <Foundation/Image/Conversions/ImageConversionMixin.h>
#include <Foundation/Image/Conversions/PixelConversions.h>
#include <emmintrin.h>

class ezImageConversion_4444_8888 : public ezImageConversionMixinLinear<ezImageConversion_4444_8888>
{
public:
  static const ezUInt32 s_uiSourceBpp = 16;
  static const ezUInt32 s_uiTargetBpp = 32;
  static const ezUInt32 s_uiMultiConversionSize = 8;

  typedef ezUInt8 SourceTypeSingle;
  typedef ezUInt8 TargetTypeSingle;

  ezImageConversion_4444_8888()
  {
    m_subConversions.PushBack(SubConversion(ezImageFormat::B4G4R4A4_UNORM, ezImageFormat::B8G8R8A8_UNORM, ezImageConversionFlags::None));
  }

  static void ConvertSingle(const SourceTypeSingle* pSource, TargetTypeSingle* pTarget)
  {
    pTarget[0] = (pSource[0] & 0x0F) * 17;
    pTarget[1] = (pSource[0] >> 4) * 17;
    pTarget[2] = (pSource[1] & 0x0F) * 17;
    pTarget[3] = (pSource[1] >> 4) * 17;
  }

  typedef __m128i SourceTypeMultiple;
  typedef __m128i TargetTypeMultiple;

  static void ConvertMultiple(const SourceTypeMultiple* pSource, TargetTypeMultiple* pTarget)
  {
    __m128i mask = _mm_set1_epi32(0x0F0F0F0F);

    // Mask out 4bits per byte
    __m128i A = _mm_and_si128(mask, pSource[0]);
    __m128i B = _mm_andnot_si128(mask, pSource[0]);

    // Duplicate each 4bit group - equivalent to multiplying by 17 = 255/15
    A = _mm_or_si128(A, _mm_slli_epi32(A, 4));
    B = _mm_or_si128(B, _mm_srli_epi32(B, 4));

    // Interleave bytes again
    pTarget[0] = _mm_unpacklo_epi8(A, B);
    pTarget[1] = _mm_unpackhi_epi8(A, B);
  }
};

class ezImageConversion_BGRX_BGRA : public ezImageConversionMixinLinear<ezImageConversion_BGRX_BGRA>
{
public:
  static const ezUInt32 s_uiSourceBpp = 32;
  static const ezUInt32 s_uiTargetBpp = 32;
  static const ezUInt32 s_uiMultiConversionSize = 4;

  typedef ezUInt32 SourceTypeSingle;
  typedef ezUInt32 TargetTypeSingle;

  ezImageConversion_BGRX_BGRA()
  {
    m_subConversions.PushBack(SubConversion(ezImageFormat::B8G8R8X8_UNORM, ezImageFormat::B8G8R8A8_UNORM, ezImageConversionFlags::InPlace));
  }

  static void ConvertSingle(const SourceTypeSingle* pSource, TargetTypeSingle* pTarget)
  {
    pTarget[0] = pSource[0] | 0xFF000000;
  }

  typedef __m128i SourceTypeMultiple;
  typedef __m128i TargetTypeMultiple;

  static void ConvertMultiple(const SourceTypeMultiple* pSource, TargetTypeMultiple* pTarget)
  {
    __m128i mask = _mm_set1_epi32(0xFF000000);

    pTarget[0] = _mm_or_si128(pSource[0], mask);
  }
};

struct ezImageConversion_F32_U8 : public ezImageConversionMixinLinear<ezImageConversion_F32_U8>
{
  static const ezUInt32 s_uiSourceBpp = 128;
  static const ezUInt32 s_uiTargetBpp = 32;
  static const ezUInt32 s_uiMultiConversionSize = 1;

  typedef ezColor SourceTypeSingle;
  typedef ezColorLinearUB TargetTypeSingle;

  ezImageConversion_F32_U8()
  {
    m_subConversions.PushBack(SubConversion(ezImageFormat::R32G32B32A32_FLOAT, ezImageFormat::R8G8B8A8_UNORM, ezImageConversionFlags::Lossy));
  }

  static void ConvertSingle(const SourceTypeSingle* pSource, TargetTypeSingle* pTarget)
  {
    pTarget[0].r = static_cast<ezUInt8>(ezMath::Clamp(ezMath::Floor(pSource[0].r * 255.0f + 0.5f), 0.0f, 255.0f));
    pTarget[0].b = static_cast<ezUInt8>(ezMath::Clamp(ezMath::Floor(pSource[0].b * 255.0f + 0.5f), 0.0f, 255.0f));
    pTarget[0].g = static_cast<ezUInt8>(ezMath::Clamp(ezMath::Floor(pSource[0].g * 255.0f + 0.5f), 0.0f, 255.0f));
    pTarget[0].a = static_cast<ezUInt8>(ezMath::Clamp(ezMath::Floor(pSource[0].a * 255.0f + 0.5f), 0.0f, 255.0f));
  }

  typedef ezColor SourceTypeMultiple;
  typedef ezColorLinearUB TargetTypeMultiple;

  static void ConvertMultiple(const SourceTypeMultiple* pSource, TargetTypeMultiple* pTarget)
  {
    return ConvertSingle(pSource, pTarget);
  }
};

struct ezImageConversion_U8_F32 : public ezImageConversionMixinLinear<ezImageConversion_U8_F32>
{
  static const ezUInt32 s_uiSourceBpp = 32;
  static const ezUInt32 s_uiTargetBpp = 128;
  static const ezUInt32 s_uiMultiConversionSize = 1;

  typedef ezColorLinearUB SourceTypeSingle;
  typedef ezColor TargetTypeSingle;

  ezImageConversion_U8_F32()
  {
    m_subConversions.PushBack(SubConversion(ezImageFormat::R8G8B8A8_UNORM, ezImageFormat::R32G32B32A32_FLOAT, ezImageConversionFlags::None));
  }

  static void ConvertSingle(const SourceTypeSingle* pSource, TargetTypeSingle* pTarget)
  {
    *pTarget = *pSource;
  }

  typedef ezColorLinearUB SourceTypeMultiple;
  typedef ezColor TargetTypeMultiple;

  static void ConvertMultiple(const SourceTypeMultiple* pSource, TargetTypeMultiple* pTarget)
  {
    return ConvertSingle(pSource, pTarget);
  }
};

struct ezImageConversion_R_RGBA : public ezImageConversionMixinLinear<ezImageConversion_R_RGBA>
{
  static const ezUInt32 s_uiSourceBpp = 8;
  static const ezUInt32 s_uiTargetBpp = 32;
  static const ezUInt32 s_uiMultiConversionSize = 1;

  typedef ezUInt8 SourceTypeSingle;
  typedef ezUInt8 TargetTypeSingle;

  ezImageConversion_R_RGBA()
  {
    m_subConversions.PushBack(SubConversion(ezImageFormat::R8_UNORM, ezImageFormat::R8G8B8A8_UNORM, ezImageConversionFlags::None));
  }

  static void ConvertSingle(const SourceTypeSingle* pSource, TargetTypeSingle* pTarget)
  {
    pTarget[0] = pSource[0];
    pTarget[1] = pSource[0];
    pTarget[2] = pSource[0];
    pTarget[3] = 0xFF;
  }

  typedef ezUInt8 SourceTypeMultiple;
  typedef ezUInt8 TargetTypeMultiple;

  static void ConvertMultiple(const SourceTypeMultiple* pSource, TargetTypeMultiple* pTarget)
  {
    return ConvertSingle(pSource, pTarget);
  }
};

class ezImageConversion_BGR_BGRA : public ezImageConversionMixinLinear<ezImageConversion_BGR_BGRA>
{
public:
  static const ezUInt32 s_uiSourceBpp = 24;
  static const ezUInt32 s_uiTargetBpp = 32;
  static const ezUInt32 s_uiMultiConversionSize = 1;

  typedef ezUInt8 SourceTypeSingle;
  typedef ezUInt8 TargetTypeSingle;

  ezImageConversion_BGR_BGRA()
  {
    m_subConversions.PushBack(SubConversion(ezImageFormat::B8G8R8_UNORM, ezImageFormat::B8G8R8A8_UNORM, ezImageConversionFlags::None));
  }

  static void ConvertSingle(const SourceTypeSingle* pSource, TargetTypeSingle* pTarget)
  {
    pTarget[0] = pSource[0];
    pTarget[1] = pSource[1];
    pTarget[2] = pSource[2];
    pTarget[3] = 0xFF;
  }

  typedef ezUInt8 SourceTypeMultiple;
  typedef ezUInt8 TargetTypeMultiple;

  static void ConvertMultiple(const SourceTypeMultiple* pSource, TargetTypeMultiple* pTarget)
  {
    return ConvertSingle(pSource, pTarget);
  }
};

class ezImageConversion_RGB_RGBA_F : public ezImageConversionMixinLinear<ezImageConversion_RGB_RGBA_F>
{
public:
  static const ezUInt32 s_uiSourceBpp = 96;
  static const ezUInt32 s_uiTargetBpp = 128;
  static const ezUInt32 s_uiMultiConversionSize = 1;

  typedef float SourceTypeSingle;
  typedef float TargetTypeSingle;

  ezImageConversion_RGB_RGBA_F()
  {
    m_subConversions.PushBack(SubConversion(ezImageFormat::R32G32B32_FLOAT, ezImageFormat::R32G32B32A32_FLOAT, ezImageConversionFlags::None));
  }

  static void ConvertSingle(const SourceTypeSingle* pSource, TargetTypeSingle* pTarget)
  {
    pTarget[0] = pSource[0];
    pTarget[1] = pSource[1];
    pTarget[2] = pSource[2];
    pTarget[3] = 1.0f;
  }

  typedef float SourceTypeMultiple;
  typedef float TargetTypeMultiple;

  static void ConvertMultiple(const SourceTypeMultiple* pSource, TargetTypeMultiple* pTarget)
  {
    return ConvertSingle(pSource, pTarget);
  }
};

class ezImageConversion_R_BGRA : public ezImageConversionMixinLinear<ezImageConversion_R_BGRA>
{
public:
  static const ezUInt32 s_uiSourceBpp = 8;
  static const ezUInt32 s_uiTargetBpp = 32;
  static const ezUInt32 s_uiMultiConversionSize = 1;

  typedef ezUInt8 SourceTypeSingle;
  typedef ezUInt8 TargetTypeSingle;

  ezImageConversion_R_BGRA()
  {
    m_subConversions.PushBack(SubConversion(ezImageFormat::R8_UNORM, ezImageFormat::B8G8R8A8_UNORM, ezImageConversionFlags::None));
  }

  static void ConvertSingle(const SourceTypeSingle* pSource, TargetTypeSingle* pTarget)
  {
    pTarget[0] = pSource[0];
    pTarget[1] = pSource[0];
    pTarget[2] = pSource[0];
    pTarget[3] = 0xFF;
  }

  typedef ezUInt8 SourceTypeMultiple;
  typedef ezUInt8 TargetTypeMultiple;

  static void ConvertMultiple(const SourceTypeMultiple* pSource, TargetTypeMultiple* pTarget)
  {
    return ConvertSingle(pSource, pTarget);
  }
};

class ezImageConversion_R_RGBA_F : public ezImageConversionMixinLinear<ezImageConversion_R_RGBA_F>
{
public:
  static const ezUInt32 s_uiSourceBpp = 32;
  static const ezUInt32 s_uiTargetBpp = 128;
  static const ezUInt32 s_uiMultiConversionSize = 1;

  typedef float SourceTypeSingle;
  typedef float TargetTypeSingle;

  ezImageConversion_R_RGBA_F()
  {
    m_subConversions.PushBack(SubConversion(ezImageFormat::R32_FLOAT, ezImageFormat::R32G32B32A32_FLOAT, ezImageConversionFlags::None));
  }

  static void ConvertSingle(const SourceTypeSingle* pSource, TargetTypeSingle* pTarget)
  {
    pTarget[0] = pSource[0];
    pTarget[1] = pSource[0];
    pTarget[2] = pSource[0];
    pTarget[3] = 1.0f;
  }

  typedef float SourceTypeMultiple;
  typedef float TargetTypeMultiple;

  static void ConvertMultiple(const SourceTypeMultiple* pSource, TargetTypeMultiple* pTarget)
  {
    return ConvertSingle(pSource, pTarget);
  }
};

class ezImageConversion_BGRA_BGR : public ezImageConversionMixinLinear<ezImageConversion_BGRA_BGR>
{
public:
  static const ezUInt32 s_uiSourceBpp = 32;
  static const ezUInt32 s_uiTargetBpp = 24;
  static const ezUInt32 s_uiMultiConversionSize = 1;

  typedef ezUInt8 SourceTypeSingle;
  typedef ezUInt8 TargetTypeSingle;

  ezImageConversion_BGRA_BGR()
  {
    /// \todo Not sure about the SRGB stuff and the Lossy flag. Also maybe this could be generalized, like the swizzle conversion ?

    m_subConversions.PushBack(SubConversion(ezImageFormat::B8G8R8A8_UNORM, ezImageFormat::B8G8R8_UNORM, ezImageConversionFlags::Lossy));
    m_subConversions.PushBack(SubConversion(ezImageFormat::B8G8R8X8_UNORM, ezImageFormat::B8G8R8_UNORM, ezImageConversionFlags::Lossy));
    m_subConversions.PushBack(SubConversion(ezImageFormat::B8G8R8A8_TYPELESS, ezImageFormat::B8G8R8_UNORM, ezImageConversionFlags::Lossy));
    m_subConversions.PushBack(SubConversion(ezImageFormat::B8G8R8A8_UNORM_SRGB, ezImageFormat::B8G8R8_UNORM, ezImageConversionFlags::Lossy));
    m_subConversions.PushBack(SubConversion(ezImageFormat::B8G8R8X8_TYPELESS, ezImageFormat::B8G8R8_UNORM, ezImageConversionFlags::Lossy));
    m_subConversions.PushBack(SubConversion(ezImageFormat::B8G8R8X8_UNORM_SRGB, ezImageFormat::B8G8R8_UNORM, ezImageConversionFlags::Lossy));
  }

  static void ConvertSingle(const SourceTypeSingle* pSource, TargetTypeSingle* pTarget)
  {
    pTarget[0] = pSource[0];
    pTarget[1] = pSource[1];
    pTarget[2] = pSource[2];
  }

  typedef ezUInt8 SourceTypeMultiple;
  typedef ezUInt8 TargetTypeMultiple;

  static void ConvertMultiple(const SourceTypeMultiple* pSource, TargetTypeMultiple* pTarget)
  {
    return ConvertSingle(pSource, pTarget);
  }
};

class ezImageConversion_RGBA_RGB_F : public ezImageConversionMixinLinear<ezImageConversion_RGBA_RGB_F>
{
public:
  static const ezUInt32 s_uiSourceBpp = 128;
  static const ezUInt32 s_uiTargetBpp = 96;
  static const ezUInt32 s_uiMultiConversionSize = 1;

  typedef float SourceTypeSingle;
  typedef float TargetTypeSingle;

  ezImageConversion_RGBA_RGB_F()
  {
    m_subConversions.PushBack(SubConversion(ezImageFormat::R32G32B32A32_FLOAT, ezImageFormat::R32G32B32_FLOAT, ezImageConversionFlags::Lossy));
  }

  static void ConvertSingle(const SourceTypeSingle* pSource, TargetTypeSingle* pTarget)
  {
    pTarget[0] = pSource[0];
    pTarget[1] = pSource[1];
    pTarget[2] = pSource[2];
  }

  typedef float SourceTypeMultiple;
  typedef float TargetTypeMultiple;

  static void ConvertMultiple(const SourceTypeMultiple* pSource, TargetTypeMultiple* pTarget)
  {
    return ConvertSingle(pSource, pTarget);
  }
};

ezColorLinearUB ezDecompress565(ezUInt16 uiColor)
{
  ezColorLinearUB result;
  result.r = (uiColor & 0xF800u) * 255 / 63488;
  result.g = (uiColor & 0x07E0u) * 255 / 2016;
  result.b = (uiColor & 0x001Fu) * 255 / 31;
  result.a = 0xFF;
  return result;
}

static ezImageConversion_4444_8888 g_conversion4444_8888;
static ezImageConversion_BGRX_BGRA g_conversionBGRX_BGRA;
static ezImageConversion_BGR_BGRA g_conversionBGR_BGRA;
static ezImageConversion_R_BGRA g_conversionR_BGRA;
static ezImageConversion_BGRA_BGR g_conversionBGRA_BGR;
static ezImageConversion_R_RGBA g_conversionR_RGBA;
static ezImageConversion_F32_U8 g_conversionF32_U8;
static ezImageConversion_U8_F32 g_conversionU8_F32;
static ezImageConversion_RGB_RGBA_F g_conversionRGB_RGBA_F;
static ezImageConversion_R_RGBA_F g_conversionR_RGBA_F;
static ezImageConversion_RGBA_RGB_F g_conversion_RGBA_RGB_F;



EZ_STATICLINK_FILE(Foundation, Foundation_Image_Conversions_PixelConversions);
