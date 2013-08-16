#include <Conversions/PixelConversions.h>

#include <ImageConversionMixin.h>

struct ezImageConversion4444_8888 : public ezImageConversionMixinLinear<ezImageConversion4444_8888>
{
  static const ezUInt32 s_uiSourceBpp = 16;
  static const ezUInt32 s_uiTargetBpp = 32;
  static const ezUInt32 s_uiMultiConversionSize = 8;

  typedef ezUInt8 SourceTypeSingle;
  typedef ezUInt8 TargetTypeSingle;

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


struct ezImageConversionF32_U8 : public ezImageConversionMixinLinear<ezImageConversionF32_U8>
{
  static const ezUInt32 s_uiSourceBpp = 128;
  static const ezUInt32 s_uiTargetBpp = 32;
  static const ezUInt32 s_uiMultiConversionSize = 1;

  typedef ezRgbaF SourceTypeSingle;
  typedef ezRgba TargetTypeSingle;

  static void ConvertSingle(const SourceTypeSingle* pSource, TargetTypeSingle* pTarget)
  {
    pTarget[0].r = static_cast<ezUInt8>(ezMath::Clamp(floor(pSource[0].r * 0xFF + 0.5f), 0.0f, 255.0f));
    pTarget[0].b = static_cast<ezUInt8>(ezMath::Clamp(floor(pSource[0].b * 0xFF + 0.5f), 0.0f, 255.0f));
    pTarget[0].g = static_cast<ezUInt8>(ezMath::Clamp(floor(pSource[0].g * 0xFF + 0.5f), 0.0f, 255.0f));
    pTarget[0].a = static_cast<ezUInt8>(ezMath::Clamp(floor(pSource[0].a * 0xFF + 0.5f), 0.0f, 255.0f));
  }

  typedef ezRgbaF SourceTypeMultiple;
  typedef ezRgba TargetTypeMultiple;

  static void ConvertMultiple(const SourceTypeMultiple* pSource, TargetTypeMultiple* pTarget)
  {
    return ConvertSingle(pSource, pTarget);
  }
};


ezBgra ezDecompress565(ezUInt16 uiColor)
{
  ezBgra result;
  result.b = (uiColor & 0x001Fu) * 255 / 31;
  result.g = (uiColor & 0x07E0u) * 255 / 2016;
  result.r = (uiColor & 0xF800u) * 255 / 63488;
  result.a = 0xFF;
  return result;
}

void ezConvertImage4444_8888(const ezImage& source, ezImage& target)
{
  return ezImageConversion4444_8888::ConvertImage(source, target);
}

void ezConvertImageF32_U8(const ezImage& source, ezImage& target)
{
  return ezImageConversionF32_U8::ConvertImage(source, target);
}

