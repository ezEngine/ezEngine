#include <Conversions/SwizzleConversions.h>

#include <ImageDefinitions.h>

struct ezImageSwizzleConversion32_2103_SSE2 : public ezImageConversionMixinLinear<ezImageSwizzleConversion32_2103_SSE2>
{
  static const ezUInt32 s_uiSourceBpp = 32;
  static const ezUInt32 s_uiTargetBpp = 32;
  static const ezUInt32 s_uiMultiConversionSize = 16;

  typedef ezUInt8 SourceTypeSingle;
  typedef ezUInt8 TargetTypeSingle;

  static void ConvertSingle(const SourceTypeSingle* pSource, TargetTypeSingle* pTarget)
  {
    pTarget[0] = pSource[2];
    pTarget[1] = pSource[1];
    pTarget[2] = pSource[0];
    pTarget[3] = pSource[3];
  }

  typedef __m128i SourceTypeMultiple;
  typedef __m128i TargetTypeMultiple;

  static void ConvertMultiple(const SourceTypeMultiple* pSource, TargetTypeMultiple* pTarget)
  {
    __m128i mask1 = _mm_set1_epi32(0xff00ff00);
    __m128i mask2 = _mm_set1_epi32(0x00ff00ff);

    // Intel optimization manual, Color Pixel Format Conversion Using SSE2
    pTarget[0] = _mm_or_si128(_mm_and_si128(pSource[0], mask1), _mm_and_si128(_mm_or_si128(_mm_slli_epi32(pSource[0], 16), _mm_srli_epi32(pSource[0], 16)), mask2));
    pTarget[1] = _mm_or_si128(_mm_and_si128(pSource[1], mask1), _mm_and_si128(_mm_or_si128(_mm_slli_epi32(pSource[1], 16), _mm_srli_epi32(pSource[1], 16)), mask2));
    pTarget[2] = _mm_or_si128(_mm_and_si128(pSource[2], mask1), _mm_and_si128(_mm_or_si128(_mm_slli_epi32(pSource[2], 16), _mm_srli_epi32(pSource[2], 16)), mask2));
    pTarget[3] = _mm_or_si128(_mm_and_si128(pSource[3], mask1), _mm_and_si128(_mm_or_si128(_mm_slli_epi32(pSource[3], 16), _mm_srli_epi32(pSource[3], 16)), mask2));
  }
};

struct ezImageSwizzleConversion32_2103_SSSE3 : public ezImageConversionMixinLinear<ezImageSwizzleConversion32_2103_SSSE3>
{
  static const ezUInt32 s_uiSourceBpp = 32;
  static const ezUInt32 s_uiTargetBpp = 32;
  static const ezUInt32 s_uiMultiConversionSize = 16;

  typedef ezUInt8 SourceTypeSingle;
  typedef ezUInt8 TargetTypeSingle;

  static void ConvertSingle(const SourceTypeSingle* pSource, TargetTypeSingle* pTarget)
  {
    pTarget[0] = pSource[2];
    pTarget[1] = pSource[1];
    pTarget[2] = pSource[0];
    pTarget[3] = pSource[3];
  }

  typedef __m128i SourceTypeMultiple;
  typedef __m128i TargetTypeMultiple;

  static void ConvertMultiple(const SourceTypeMultiple* pSource, TargetTypeMultiple* pTarget)
  {
    // Intel optimization manual, Color Pixel Format Conversion Using SSSE3
    __m128i shuffleMask = _mm_set_epi8(15, 12, 13, 14, 11, 8, 9, 10, 7, 4, 5, 6, 3, 0, 1, 2);
    pTarget[0] = _mm_shuffle_epi8(pSource[0], shuffleMask);
    pTarget[1] = _mm_shuffle_epi8(pSource[1], shuffleMask);
    pTarget[2] = _mm_shuffle_epi8(pSource[2], shuffleMask);
    pTarget[3] = _mm_shuffle_epi8(pSource[3], shuffleMask);
  }
};

void ezSwizzleImage32_2103(const ezImage& source, ezImage& target)
{
  bool bSupportsSSSE3 = true;
  if(bSupportsSSSE3)
  {
    return ezImageSwizzleConversion32_2103_SSSE3::ConvertImage(source, target);
  }
  else
  {
    return ezImageSwizzleConversion32_2103_SSE2::ConvertImage(source, target);
  }
}