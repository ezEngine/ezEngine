#include <ezImageConversion.h>

#include <Foundation/Containers/Bitfield.h>
#include "ezImageConversionMixin.h"
#include "Foundation/Memory/EndianHelper.h"


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

  static EZ_FORCE_INLINE void ConvertMultiple(const SourceTypeMultiple* pSource, TargetTypeMultiple* pTarget)
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

void ezDecompressBC1Block(const ezUInt8* pSource, ezBgra* pTarget, bool bForceFourColorMode)
{
  ezUInt16 uiColor0 = pSource[0] | (pSource[1] << 8);
  ezUInt16 uiColor1 = pSource[2] | (pSource[3] << 8);

  ezBgra colors[4];

  colors[0] = ezDecompress565(pSource[0] | (pSource[1] << 8));
  colors[1] = ezDecompress565(pSource[2] | (pSource[3] << 8));

  if(uiColor0 > uiColor1 || bForceFourColorMode)
  {
    colors[2] = ezBgra(
      (2 * colors[0].b + colors[1].b + 1) / 3,
      (2 * colors[0].g + colors[1].g + 1) / 3,
      (2 * colors[0].r + colors[1].r + 1) / 3,
      0xFF);
    colors[3] = ezBgra(
      (colors[0].b + 2 * colors[1].b + 1) / 3,
      (colors[0].g + 2 * colors[1].g + 1) / 3,
      (colors[0].r + 2 * colors[1].r + 1) / 3,
      0xFF);
  }
  else
  {
    colors[2] = ezBgra(
      (colors[0].b + colors[1].b) / 2,
      (colors[0].g + colors[1].g) / 2,
      (colors[0].r + colors[1].r) / 2,
      0xFF);
    colors[3] = ezBgra(0, 0, 0, 0);
  }

  for(ezUInt32 uiByteIdx = 0; uiByteIdx < 4; uiByteIdx++)
  {
    ezUInt8 uiIndices = pSource[4 + uiByteIdx];

    pTarget[4 * uiByteIdx + 0] = colors[(uiIndices >> 0) & 0x03];
    pTarget[4 * uiByteIdx + 1] = colors[(uiIndices >> 2) & 0x03];
    pTarget[4 * uiByteIdx + 2] = colors[(uiIndices >> 4) & 0x03];
    pTarget[4 * uiByteIdx + 3] = colors[(uiIndices >> 6) & 0x03];
  }
}

void ezDecompressBC4Block(const ezUInt8* pSource, ezUInt8* pTarget, ezUInt32 uiStride)
{
  ezUInt8 uiAlphas[8];

  uiAlphas[0] = pSource[0];
  uiAlphas[1] = pSource[1];

  if(uiAlphas[0] > uiAlphas[1])
  {
    uiAlphas[2] = (6 * uiAlphas[0] + 1 * uiAlphas[1] + 3) / 7;
    uiAlphas[3] = (5 * uiAlphas[0] + 2 * uiAlphas[1] + 3) / 7;
    uiAlphas[4] = (4 * uiAlphas[0] + 3 * uiAlphas[1] + 3) / 7;
    uiAlphas[5] = (3 * uiAlphas[0] + 4 * uiAlphas[1] + 3) / 7;
    uiAlphas[6] = (2 * uiAlphas[0] + 5 * uiAlphas[1] + 3) / 7;
    uiAlphas[7] = (1 * uiAlphas[0] + 6 * uiAlphas[1] + 3) / 7;
  }
  else
  {
    uiAlphas[2] = (4 * uiAlphas[0] + 1 * uiAlphas[1] + 2) / 5;
    uiAlphas[3] = (3 * uiAlphas[0] + 2 * uiAlphas[1] + 2) / 5;
    uiAlphas[4] = (2 * uiAlphas[0] + 3 * uiAlphas[1] + 2) / 5;
    uiAlphas[5] = (1 * uiAlphas[0] + 4 * uiAlphas[1] + 2) / 5;
    uiAlphas[6] = 0x00;
    uiAlphas[7] = 0xFF;
  }

  for(ezUInt32 uiTripleIdx = 0; uiTripleIdx < 2; uiTripleIdx++)
  {
    ezUInt32 uiIndices =
      pSource[2 + uiTripleIdx * 3 + 0] << 0 |
      pSource[2 + uiTripleIdx * 3 + 1] << 8 |
      pSource[2 + uiTripleIdx * 3 + 2] << 16;

    pTarget[(8 * uiTripleIdx + 0) * uiStride] = uiAlphas[(uiIndices >> 0) & 0x07];
    pTarget[(8 * uiTripleIdx + 1) * uiStride] = uiAlphas[(uiIndices >> 3) & 0x07];
    pTarget[(8 * uiTripleIdx + 2) * uiStride] = uiAlphas[(uiIndices >> 6) & 0x07];
    pTarget[(8 * uiTripleIdx + 3) * uiStride] = uiAlphas[(uiIndices >> 9) & 0x07];
    pTarget[(8 * uiTripleIdx + 4) * uiStride] = uiAlphas[(uiIndices >> 12) & 0x07];
    pTarget[(8 * uiTripleIdx + 5) * uiStride] = uiAlphas[(uiIndices >> 15) & 0x07];
    pTarget[(8 * uiTripleIdx + 6) * uiStride] = uiAlphas[(uiIndices >> 18) & 0x07];
    pTarget[(8 * uiTripleIdx + 7) * uiStride] = uiAlphas[(uiIndices >> 21) & 0x07];
  }
}

class ezImageConversion_BC1_BGRA : public ezImageConversionMixinBlockDecompression<ezImageConversion_BC1_BGRA>
{
public:
  static const ezUInt32 s_uiSourceBpp = 4;
  static const ezUInt32 s_uiTargetBpp = 32;

  typedef ezUInt8 SourceType;
  typedef ezBgra TargetType;

  static void DecompressBlock(const SourceType* pSource, TargetType* pTarget)
  {
    ezDecompressBC1Block(pSource, pTarget, false);
  }
};

class ezImageConversion_BC2_BGRA : public ezImageConversionMixinBlockDecompression<ezImageConversion_BC2_BGRA>
{
public:
  static const ezUInt32 s_uiSourceBpp = 8;
  static const ezUInt32 s_uiTargetBpp = 32;

  typedef ezUInt8 SourceType;
  typedef ezBgra TargetType;

  static void DecompressBlock(const SourceType* pSource, TargetType* pTarget)
  {
    ezDecompressBC1Block(pSource + 8, pTarget, true);

    for(ezUInt32 uiByteIdx = 0; uiByteIdx < 8; uiByteIdx++)
    {
      ezUInt8 uiIndices = pSource[uiByteIdx];

      pTarget[2 * uiByteIdx + 0].a = (uiIndices & 0x0F) | (uiIndices << 4);
      pTarget[2 * uiByteIdx + 1].a = (uiIndices & 0xF0) | (uiIndices >> 4);
    }
  }
};

class ezImageConversion_BC3_BGRA : public ezImageConversionMixinBlockDecompression<ezImageConversion_BC3_BGRA>
{
public:
  static const ezUInt32 s_uiSourceBpp = 8;
  static const ezUInt32 s_uiTargetBpp = 32;

  typedef ezUInt8 SourceType;
  typedef ezBgra TargetType;

  static void DecompressBlock(const SourceType* pSource, TargetType* pTarget)
  {
    ezDecompressBC1Block(pSource + 8, pTarget, true);
    ezDecompressBC4Block(pSource, reinterpret_cast<ezUInt8*>(pTarget), 4);
  }
};

class ezImageConversion_BC4_R : public ezImageConversionMixinBlockDecompression<ezImageConversion_BC4_R>
{
public:
  static const ezUInt32 s_uiSourceBpp = 4;
  static const ezUInt32 s_uiTargetBpp = 8;

  typedef ezUInt8 SourceType;
  typedef ezUInt8 TargetType;

  static void DecompressBlock(const SourceType* pSource, TargetType* pTarget)
  {
    ezDecompressBC4Block(pSource, pTarget, 1);
  }
};

class ezImageConversion_BC5_RG : public ezImageConversionMixinBlockDecompression<ezImageConversion_BC5_RG>
{
public:
  static const ezUInt32 s_uiSourceBpp = 8;
  static const ezUInt32 s_uiTargetBpp = 16;

  typedef ezUInt8 SourceType;
  typedef ezUInt8 TargetType;

  static void DecompressBlock(const SourceType* pSource, TargetType* pTarget)
  {
    ezDecompressBC4Block(pSource + 0, pTarget + 0, 2);
    ezDecompressBC4Block(pSource + 8, pTarget + 1, 2);
  }
};

class ezImageConversion_BC6U_RGBA : public ezImageConversionMixinBlockDecompression<ezImageConversion_BC6U_RGBA>
{
public:
  static const ezUInt32 s_uiSourceBpp = 8;
  static const ezUInt32 s_uiTargetBpp = 128;

  typedef ezUInt8 SourceType;
  typedef ezRgbaF TargetType;

  static void DecompressBlock(const SourceType* pSource, TargetType* pTarget)
  {
    ezUInt8 uiMode = pSource[0];

    // If the lowest two bits are < 2, the mode is encoded in 2 bits, otherwise in 5.
    if((uiMode & 0x03) < 2)
    {
      uiMode &= 0x03;
    }
    else
    {
      uiMode &= 0x1F;
    }

    // Decompression for 16.4 is still missing - need a sample case. All others should be bit-exact according to the spec.
    switch(uiMode)
    {
    case 0:
      DecompressMode0(pSource, pTarget);
      break;

    case 1:
      DecompressMode1(pSource, pTarget);
      break;

    case 2:
      DecompressMode2(pSource, pTarget);
      break;

    case 3:
      DecompressMode3(pSource, pTarget);
      break;

    case 6:
      DecompressMode6(pSource, pTarget);
      break;

    case 7:
      DecompressMode7(pSource, pTarget);
      break;

    case 10:
      DecompressMode10(pSource, pTarget);
      break;
      
    case 11:
      DecompressMode11(pSource, pTarget);
      break;

    case 14:
      DecompressMode14(pSource, pTarget);
      break;

    case 18:
      DecompressMode18(pSource, pTarget);
      break;

    case 22:
      DecompressMode22(pSource, pTarget);
      break;

   case 26:
      DecompressMode26(pSource, pTarget);
      break;

    case 30:
      DecompressMode30(pSource, pTarget);
      break;

    default:
      // Spec requires all invalid combinations to return 0.0f
      memset(pTarget, 0, 256);
      break;
    }
  }

  union FP32
  {
    ezUInt32 u;
    float f;
  };

  struct Color
  {
    ezUInt16 r;
    ezUInt16 g;
    ezUInt16 b;

    void operator+=(const Color& other)
    {
      r += other.r;
      g += other.g;
      b += other.b;
    }
  };

  static FP32 HalfToFloat(ezUInt16 uiHalf)
  {
    FP32 out;
    out.u = (uiHalf << 12) + ((127 - 15) << 23);
    return out;
  }

  static ezUInt32	SignExtend(ezUInt32 uiComponent, ezUInt32 uiBits)
  {
    return static_cast<ezInt32>(uiComponent << (32 - uiBits)) >> (32 - uiBits);
  }

  static void	SignExtend(Color& color, ezUInt32 uiBitsR, ezUInt32 uiBitsG, ezUInt32 uiBitsB)
  {
    color.r = SignExtend(color.r, uiBitsR);
    color.g = SignExtend(color.g, uiBitsG);
    color.b = SignExtend(color.b, uiBitsB);
  }

  static void DecodeIndices(const SourceType* pSource, TargetType* pTarget, const Color& A, const Color& B)
  {
    ezUInt32 i = ReadBits<3, 65>(pSource);
    pTarget[0].r = HalfToFloat(FinishUnquantize(Interpolate16(A.r, B.r, i))).f;
    pTarget[0].g = HalfToFloat(FinishUnquantize(Interpolate16(A.g, B.g, i))).f;
    pTarget[0].b = HalfToFloat(FinishUnquantize(Interpolate16(A.b, B.b, i))).f;
    pTarget[0].a = 1.0f;

    for(ezUInt32 uiIndex = 1; uiIndex < 16; uiIndex++)
    {
      ezUInt32 i = ReadBits(pSource, 4, 64 + 4 * uiIndex);
      pTarget[uiIndex].r = HalfToFloat(FinishUnquantize(Interpolate16(A.r, B.r, i))).f;
      pTarget[uiIndex].g = HalfToFloat(FinishUnquantize(Interpolate16(A.g, B.g, i))).f;
      pTarget[uiIndex].b = HalfToFloat(FinishUnquantize(Interpolate16(A.b, B.b, i))).f;
      pTarget[uiIndex].a = 1.0f;
    }
  }

  static void DecodeIndices2(const SourceType* pSource, TargetType* pTarget, const Color* A, const Color* B)
  {
    ezUInt8 uiShape = ReadBits<5, 77>(pSource);

    static const ezUInt32 uiFixUpIndex[32] =
    {
      15, 15, 15, 15,
      15, 15, 15, 15,
      15, 15, 15, 15,
      15, 15, 15, 15,
      15, 2, 8, 2,
      2, 8, 8, 15,
      2, 8, 2, 2,
      8, 8, 2, 2
    };

    static const ezUInt32 uiPartitions[32] =
    {
      0xCCCC, 0x8888, 0xEEEE, 0xECC8,
      0xC880, 0xFEEC, 0xFEC8, 0xEC80,
      0xC800, 0xFFEC, 0xFE80, 0xE800,
      0xFFE8, 0xFF00, 0xFFF0, 0xF000,
      0xF710, 0x008E, 0x7100, 0x08CE,
      0x008C, 0x7310, 0x3100, 0x8CCE,
      0x088C, 0x3110, 0x6666, 0x366C,
      0x17E8, 0x0FF0, 0x718E, 0x399C
    };

    for(ezUInt32 uiIndex = 0; uiIndex < 16; uiIndex++)
    {
      ezUInt32 i = 0;
      if(uiIndex == 0)
      {
        i = ReadBits<2, 82>(pSource);
      }
      else if(uiIndex < uiFixUpIndex[uiShape])
      {
        i = ReadBits(pSource, 3, 81 + 3 * uiIndex);
      }
      else if(uiIndex == uiFixUpIndex[uiShape])
      {
        i = ReadBits(pSource, 2, 81 + 3 * uiIndex);
      }
      else
      {
        i = ReadBits(pSource, 3, 80 + 3 * uiIndex);
      }

      ezUInt32 uiRegion = (uiPartitions[uiShape] >> uiIndex) & 1;

      pTarget[uiIndex].r = HalfToFloat(FinishUnquantize(Interpolate8(A[uiRegion].r, B[uiRegion].r, i))).f;
      pTarget[uiIndex].g = HalfToFloat(FinishUnquantize(Interpolate8(A[uiRegion].g, B[uiRegion].g, i))).f;
      pTarget[uiIndex].b = HalfToFloat(FinishUnquantize(Interpolate8(A[uiRegion].b, B[uiRegion].b, i))).f;
      pTarget[uiIndex].a = 1.0f;
    }
    
  }

  static void DecompressMode0(const SourceType* pSource, TargetType* pTarget)
  {
    Color A[2], B[2];

    A[0].r = ReadBits<10, 5>(pSource);
    A[0].g = ReadBits<10, 15>(pSource);
    A[0].b = ReadBits<10, 25>(pSource);

    B[0].r = ReadBits<5, 35>(pSource);
    B[0].g = ReadBits<5, 45>(pSource);
    B[0].b = ReadBits<5, 55>(pSource);

    A[1].r = ReadBits<5, 65>(pSource);
    A[1].g = ReadBits<4, 41>(pSource) | (ReadBits<1, 2>(pSource) << 4);
    A[1].b = ReadBits<4, 61>(pSource) | (ReadBits<1, 3>(pSource) << 4);

    B[1].r = ReadBits<5, 71>(pSource);
    B[1].g = ReadBits<4, 51>(pSource) | (ReadBits<1, 40>(pSource) << 4);
    B[1].b = ReadBits<1, 50>(pSource) | (ReadBits<1, 60>(pSource) << 1) | (ReadBits<1, 70>(pSource) << 2) | (ReadBits<1, 76>(pSource) << 3) | (ReadBits<1, 4>(pSource) << 4);

    SignExtend(A[1], 5, 5, 5);
    SignExtend(B[0], 5, 5, 5);
    SignExtend(B[1], 5, 5, 5);

    A[1] += A[0];
    B[0] += A[0];
    B[1] += A[0];

    Unquantize(A[0], 10);
    Unquantize(A[1], 10);
    Unquantize(B[0], 10);
    Unquantize(B[1], 10);

    DecodeIndices2(pSource, pTarget, A, B);
  }

  static void DecompressMode1(const SourceType* pSource, TargetType* pTarget)
  {
    Color A[2], B[2];

    A[0].r = ReadBits<7, 5>(pSource);
    A[0].g = ReadBits<7, 15>(pSource);
    A[0].b = ReadBits<7, 25>(pSource);

    B[0].r = ReadBits<6, 35>(pSource);
    B[0].g = ReadBits<6, 45>(pSource);
    B[0].b = ReadBits<6, 55>(pSource);

    A[1].r = ReadBits<6, 65>(pSource);
    A[1].g = ReadBits<4, 41>(pSource) | (ReadBits<1, 24>(pSource) << 4) | (ReadBits<1, 2>(pSource) << 5);
    A[1].b = ReadBits<4, 61>(pSource) | (ReadBits<1, 14>(pSource) << 4) | (ReadBits<1, 22>(pSource) << 5);

    B[1].r = ReadBits<6, 71>(pSource);
    B[1].g = ReadBits<4, 51>(pSource) | (ReadBits<1, 3>(pSource) << 4) | (ReadBits<1, 4>(pSource) << 5);
    B[1].b = ReadBits<1, 12>(pSource) | (ReadBits<1, 13>(pSource) << 1) | (ReadBits<1, 23>(pSource) << 2) | (ReadBits<1, 32>(pSource) << 3) | (ReadBits<1, 34>(pSource) << 4) | (ReadBits<1, 33>(pSource) << 5);

    SignExtend(A[1], 6, 6, 6);
    SignExtend(B[0], 6, 6, 6);
    SignExtend(B[1], 6, 6, 6);

    A[1] += A[0];
    B[0] += A[0];
    B[1] += A[0];

    Unquantize(A[0], 7);
    Unquantize(A[1], 7);
    Unquantize(B[0], 7);
    Unquantize(B[1], 7);

    DecodeIndices2(pSource, pTarget, A, B);
  }

  static void DecompressMode2(const SourceType* pSource, TargetType* pTarget)
  {
    Color A[2], B[2];

    A[0].r = ReadBits<10, 5>(pSource) | (ReadBits<1, 40>(pSource) << 10);
    A[0].g = ReadBits<10, 15>(pSource) | (ReadBits<1, 49>(pSource) << 10);
    A[0].b = ReadBits<10, 25>(pSource) | (ReadBits<1, 59>(pSource) << 10);

    B[0].r = ReadBits<5, 35>(pSource);
    B[0].g = ReadBits<4, 45>(pSource);
    B[0].b = ReadBits<4, 55>(pSource);
    
    A[1].r = ReadBits<5, 65>(pSource);
    A[1].g = ReadBits<4, 41>(pSource);
    A[1].b = ReadBits<4, 61>(pSource);

    B[1].r = ReadBits<5, 71>(pSource);
    B[1].g = ReadBits<4, 51>(pSource);
    B[1].b = ReadBits<1, 50>(pSource) | (ReadBits<1, 60>(pSource) << 1) | (ReadBits<1, 70>(pSource) << 2) | (ReadBits<1, 76>(pSource) << 3);

    SignExtend(A[1], 5, 4, 4);
    SignExtend(B[0], 5, 4, 4);
    SignExtend(B[1], 5, 4, 4);

    A[1] += A[0];
    B[0] += A[0];
    B[1] += A[0];

    Unquantize(A[0], 11);
    Unquantize(A[1], 11);
    Unquantize(B[0], 11);
    Unquantize(B[1], 11);

    DecodeIndices2(pSource, pTarget, A, B);
  }

  static void DecompressMode3(const SourceType* pSource, TargetType* pTarget)
  {
    Color A, B;
    A.r = ReadBits<10, 5>(pSource);
    A.g = ReadBits<10, 15>(pSource);
    A.b = ReadBits<10, 25>(pSource);

    B.r = ReadBits<10, 35>(pSource);
    B.g = ReadBits<10, 45>(pSource);
    B.b = ReadBits<10, 55>(pSource);

    Unquantize(A, 10);
    Unquantize(B, 10);

    DecodeIndices(pSource, pTarget, A, B);
  }

  static void DecompressMode6(const SourceType* pSource, TargetType* pTarget)
  {
    Color A[2], B[2];

    A[0].r = ReadBits<10, 5>(pSource) | (ReadBits<1, 39>(pSource) << 10);
    A[0].g = ReadBits<10, 15>(pSource) | (ReadBits<1, 50>(pSource) << 10);
    A[0].b = ReadBits<10, 25>(pSource) | (ReadBits<1, 59>(pSource) << 10);

    B[0].r = ReadBits<4, 35>(pSource);
    B[0].g = ReadBits<5, 45>(pSource);
    B[0].b = ReadBits<4, 55>(pSource);

    A[1].r = ReadBits<4, 65>(pSource);
    A[1].g = ReadBits<4, 41>(pSource) | (ReadBits<1, 75>(pSource) << 4);
    A[1].b = ReadBits<4, 61>(pSource);

    B[1].r = ReadBits<4, 71>(pSource);
    B[1].g = ReadBits<4, 51>(pSource) | (ReadBits<1, 40>(pSource) << 4);
    B[1].b = ReadBits<1, 69>(pSource) | (ReadBits<1, 60>(pSource) << 1) | (ReadBits<1, 70>(pSource) << 2) | (ReadBits<1, 76>(pSource) << 3);

    SignExtend(A[1], 4, 5, 4);
    SignExtend(B[0], 4, 5, 4);
    SignExtend(B[1], 4, 5, 4);

    A[1] += A[0];
    B[0] += A[0];
    B[1] += A[0];

    Unquantize(A[0], 11);
    Unquantize(A[1], 11);
    Unquantize(B[0], 11);
    Unquantize(B[1], 11);

    DecodeIndices2(pSource, pTarget, A, B);
  }

  static void DecompressMode7(const SourceType* pSource, TargetType* pTarget)
  {
    Color A, B;
    A.r = ReadBits<10, 5>(pSource) | (ReadBits<1, 44>(pSource) << 10);
    A.g = ReadBits<10, 15>(pSource) | (ReadBits<1, 54>(pSource) << 10);
    A.b = ReadBits<10, 25>(pSource) | (ReadBits<1, 64>(pSource) << 10);

    B.r = ReadBits<9, 35>(pSource);
    B.g = ReadBits<9, 45>(pSource);
    B.b = ReadBits<9, 55>(pSource);

    SignExtend(B, 9, 9, 9);

    B += A;

    Unquantize(A, 11);
    Unquantize(B, 11);

    DecodeIndices(pSource, pTarget, A, B);
  }

  static void DecompressMode10(const SourceType* pSource, TargetType* pTarget)
  {
    Color A[2], B[2];

    A[0].r = ReadBits<10, 5>(pSource) | (ReadBits<1, 39>(pSource) << 10);
    A[0].g = ReadBits<10, 15>(pSource) | (ReadBits<1, 49>(pSource) << 10);
    A[0].b = ReadBits<10, 25>(pSource) | (ReadBits<1, 60>(pSource) << 10);

    B[0].r = ReadBits<4, 35>(pSource);
    B[0].g = ReadBits<4, 45>(pSource);
    B[0].b = ReadBits<5, 55>(pSource);

    A[1].r = ReadBits<4, 65>(pSource);
    A[1].g = ReadBits<4, 41>(pSource);
    A[1].b = ReadBits<4, 61>(pSource) | (ReadBits<1, 40>(pSource) << 4);

    B[1].r = ReadBits<4, 71>(pSource);
    B[1].g = ReadBits<4, 51>(pSource);
    B[1].b = ReadBits<1, 50>(pSource) | (ReadBits<1, 69>(pSource) << 1) | (ReadBits<1, 70>(pSource) << 2) | (ReadBits<1, 76>(pSource) << 3) | (ReadBits<1, 75>(pSource) << 4);

    SignExtend(A[1], 4, 4, 5);
    SignExtend(B[0], 4, 4, 5);
    SignExtend(B[1], 4, 4, 5);

    A[1] += A[0];
    B[0] += A[0];
    B[1] += A[0];

    Unquantize(A[0], 11);
    Unquantize(A[1], 11);
    Unquantize(B[0], 11);
    Unquantize(B[1], 11);

    DecodeIndices2(pSource, pTarget, A, B);
  }

  static void DecompressMode11(const SourceType* pSource, TargetType* pTarget)
  {
    Color A, B;
    A.r = ReadBits<10, 5>(pSource) | (ReadBits<1, 44>(pSource) << 10) | (ReadBits<1, 43>(pSource) << 11);
    A.g = ReadBits<10, 15>(pSource) | (ReadBits<1, 54>(pSource) << 10) | (ReadBits<1, 53>(pSource) << 11);
    A.b = ReadBits<10, 25>(pSource) | (ReadBits<1, 64>(pSource) << 10) | (ReadBits<1, 63>(pSource) << 11);

    B.r = ReadBits<8, 35>(pSource);
    B.g = ReadBits<8, 45>(pSource);
    B.b = ReadBits<8, 55>(pSource);

    SignExtend(B, 8, 8, 8);

    B += A;

    Unquantize(A, 12);
    Unquantize(B, 12);
   
    DecodeIndices(pSource, pTarget, A, B);
  }

  static void DecompressMode14(const SourceType* pSource, TargetType* pTarget)
  {
    Color A[2], B[2];

    A[0].r = ReadBits<9, 5>(pSource);
    A[0].g = ReadBits<9, 15>(pSource);
    A[0].b = ReadBits<9, 25>(pSource);

    B[0].r = ReadBits<5, 35>(pSource);
    B[0].g = ReadBits<5, 45>(pSource);
    B[0].b = ReadBits<5, 55>(pSource);

    A[1].r = ReadBits<5, 65>(pSource);
    A[1].g = ReadBits<4, 41>(pSource) | (ReadBits<1, 24>(pSource) << 4);
    A[1].b = ReadBits<4, 61>(pSource) | (ReadBits<1, 14>(pSource) << 4);

    B[1].r = ReadBits<5, 71>(pSource);
    B[1].g = ReadBits<4, 51>(pSource) | (ReadBits<1, 40>(pSource) << 4);
    B[1].b = ReadBits<1, 50>(pSource) | (ReadBits<1, 60>(pSource) << 1) | (ReadBits<1, 70>(pSource) << 2) | (ReadBits<1, 76>(pSource) << 3) | (ReadBits<1, 34>(pSource) << 4);

    SignExtend(A[1], 5, 5, 5);
    SignExtend(B[0], 5, 5, 5);
    SignExtend(B[1], 5, 5, 5);

    A[1] += A[0];
    B[0] += A[0];
    B[1] += A[0];

    Unquantize(A[0], 9);
    Unquantize(A[1], 9);
    Unquantize(B[0], 9);
    Unquantize(B[1], 9);

    DecodeIndices2(pSource, pTarget, A, B);
  }

  static void DecompressMode18(const SourceType* pSource, TargetType* pTarget)
  {
    Color A[2], B[2];

    A[0].r = ReadBits<8, 5>(pSource);
    A[0].g = ReadBits<8, 15>(pSource);
    A[0].b = ReadBits<8, 25>(pSource);

    B[0].r = ReadBits<6, 35>(pSource);
    B[0].g = ReadBits<5, 45>(pSource);
    B[0].b = ReadBits<5, 55>(pSource);

    A[1].r = ReadBits<6, 65>(pSource);
    A[1].g = ReadBits<4, 41>(pSource) | (ReadBits<1, 24>(pSource) << 4);
    A[1].b = ReadBits<4, 61>(pSource) | (ReadBits<1, 14>(pSource) << 4);

    B[1].r = ReadBits<6, 71>(pSource);
    B[1].g = ReadBits<4, 51>(pSource) | (ReadBits<1, 13>(pSource) << 4);
    B[1].b = ReadBits<1, 50>(pSource) | (ReadBits<1, 60>(pSource) << 1) | (ReadBits<1, 23>(pSource) << 2) | (ReadBits<1, 33>(pSource) << 3) | (ReadBits<1, 34>(pSource) << 4);

    SignExtend(A[1], 6, 5, 5);
    SignExtend(B[0], 6, 5, 5);
    SignExtend(B[1], 6, 5, 5);

    A[1] += A[0];
    B[0] += A[0];
    B[1] += A[0];

    Unquantize(A[0], 8);
    Unquantize(A[1], 8);
    Unquantize(B[0], 8);
    Unquantize(B[1], 8);

    DecodeIndices2(pSource, pTarget, A, B);
  }

  static void DecompressMode22(const SourceType* pSource, TargetType* pTarget)
  {
    Color A[2], B[2];

    A[0].r = ReadBits<8, 5>(pSource);
    A[0].g = ReadBits<8, 15>(pSource);
    A[0].b = ReadBits<8, 25>(pSource);

    B[0].r = ReadBits<5, 35>(pSource);
    B[0].g = ReadBits<6, 45>(pSource);
    B[0].b = ReadBits<5, 55>(pSource);

    A[1].r = ReadBits<5, 65>(pSource);
    A[1].g = ReadBits<4, 41>(pSource) | (ReadBits<1, 24>(pSource) << 4) | (ReadBits<1, 23>(pSource) << 5);
    A[1].b = ReadBits<4, 61>(pSource) | (ReadBits<1, 14>(pSource) << 4);

    B[1].r = ReadBits<5, 71>(pSource);
    B[1].g = ReadBits<4, 51>(pSource) | (ReadBits<1, 40>(pSource) << 4) | (ReadBits<1, 33>(pSource) << 5);
    B[1].b = ReadBits<1, 13>(pSource) | (ReadBits<1, 60>(pSource) << 1) | (ReadBits<1, 70>(pSource) << 2) | (ReadBits<1, 76>(pSource) << 3) | (ReadBits<1, 34>(pSource) << 4);

    SignExtend(A[1], 5, 6, 5);
    SignExtend(B[0], 5, 6, 5);
    SignExtend(B[1], 5, 6, 5);

    A[1] += A[0];
    B[0] += A[0];
    B[1] += A[0];

    Unquantize(A[0], 8);
    Unquantize(A[1], 8);
    Unquantize(B[0], 8);
    Unquantize(B[1], 8);

    DecodeIndices2(pSource, pTarget, A, B);
  }

  static void DecompressMode26(const SourceType* pSource, TargetType* pTarget)
  {
    Color A[2], B[2];

    A[0].r = ReadBits<8, 5>(pSource);
    A[0].g = ReadBits<8, 15>(pSource);
    A[0].b = ReadBits<8, 25>(pSource);

    B[0].r = ReadBits<5, 35>(pSource);
    B[0].g = ReadBits<5, 45>(pSource);
    B[0].b = ReadBits<6, 55>(pSource);

    A[1].r = ReadBits<5, 65>(pSource);
    A[1].g = ReadBits<4, 41>(pSource) | (ReadBits<1, 24>(pSource) << 4);
    A[1].b = ReadBits<4, 61>(pSource) | (ReadBits<1, 14>(pSource) << 4) | (ReadBits<1, 23>(pSource) << 5);

    B[1].r = ReadBits<5, 71>(pSource);
    B[1].g = ReadBits<4, 51>(pSource) | (ReadBits<1, 40>(pSource) << 4);
    B[1].b = ReadBits<1, 50>(pSource) | (ReadBits<1, 13>(pSource) << 1) | (ReadBits<1, 70>(pSource) << 2) | (ReadBits<1, 76>(pSource) << 3) | (ReadBits<1, 34>(pSource) << 4) | (ReadBits<1, 33>(pSource) << 5);

    SignExtend(A[1], 5, 5, 6);
    SignExtend(B[0], 5, 5, 6);
    SignExtend(B[1], 5, 5, 6);

    A[1] += A[0];
    B[0] += A[0];
    B[1] += A[0];

    Unquantize(A[0], 8);
    Unquantize(A[1], 8);
    Unquantize(B[0], 8);
    Unquantize(B[1], 8);

    DecodeIndices2(pSource, pTarget, A, B);
  }

  static void DecompressMode30(const SourceType* pSource, TargetType* pTarget)
  {
    Color A[2], B[2];

    A[0].r = ReadBits<6, 5>(pSource);
    A[0].g = ReadBits<6, 15>(pSource);
    A[0].b = ReadBits<6, 25>(pSource);

    B[0].r = ReadBits<6, 35>(pSource);
    B[0].g = ReadBits<6, 45>(pSource);
    B[0].b = ReadBits<6, 55>(pSource);

    A[1].r = ReadBits<6, 65>(pSource);
    A[1].g = ReadBits<4, 41>(pSource) | (ReadBits<1, 24>(pSource) << 4) | (ReadBits<1, 21>(pSource) << 5);
    A[1].b = ReadBits<4, 61>(pSource) | (ReadBits<1, 14>(pSource) << 4) | (ReadBits<1, 22>(pSource) << 5);

    B[1].r = ReadBits<6, 71>(pSource);
    B[1].g = ReadBits<4, 51>(pSource) | (ReadBits<1, 11>(pSource) << 4) | (ReadBits<1, 31>(pSource) << 5);
    B[1].b = ReadBits<1, 12>(pSource) | (ReadBits<1, 13>(pSource) << 1) | (ReadBits<1, 23>(pSource) << 2) | (ReadBits<1, 32>(pSource) << 3) | (ReadBits<1, 34>(pSource) << 4) | (ReadBits<1, 33>(pSource) << 5);

    Unquantize(A[0], 6);
    Unquantize(A[1], 6);
    Unquantize(B[0], 6);
    Unquantize(B[1], 6);

    DecodeIndices2(pSource, pTarget, A, B);
  }

  static ezInt32 Interpolate8(ezInt32 a, ezInt32 b, ezUInt32 i)
  {
    static const ezUInt32 uiWeights[8] = {0, 9, 18, 27, 37, 46, 55, 64};

    return (a * (64 - uiWeights[i]) + b * uiWeights[i] + 32) >> 6;
  }

  static ezInt32 Interpolate16(ezInt32 a, ezInt32 b, ezUInt32 i)
  {
    static const ezUInt32 uiWeights[16] = {0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64};

    return (a * (64 - uiWeights[i]) + b * uiWeights[i] + 32) >> 6;
  }

  static void Unquantize(Color& color, ezUInt32 uiBitPerComponent)
  {          
    color.r = Unquantize(color.r, uiBitPerComponent);
    color.g = Unquantize(color.g, uiBitPerComponent);
    color.b = Unquantize(color.b, uiBitPerComponent);
  }

  static ezUInt32 Unquantize(ezUInt32 uiComponent, ezUInt32 uiBitsPerComponent)
  {
    if(uiComponent == 0)
    {
      return 0;
    }
    else if(uiComponent == ((1U << uiBitsPerComponent) - 1))
    {
      return 0xFFFF;
    }
    else
    {
      return ((uiComponent << 16) + 0x8000) >> uiBitsPerComponent;
    }
  }

  static ezUInt32 FinishUnquantize(ezUInt32 uiComponent)
  {
    return (uiComponent * 31) >> 5;
  }

  template<ezUInt32 uiNumBits, ezUInt32 uiBitPosition>
  static ezUInt32 ReadBits(const ezUInt8* pSource)
  {
    return (*reinterpret_cast<const ezUInt32*>(pSource + (uiBitPosition >> 3)) >> (uiBitPosition & 7)) & ((1U << uiNumBits) - 1);
  }

  static ezUInt32 ReadBits(const ezUInt8* pSource, ezUInt32 uiNumBits, ezUInt32 uiBitPosition)
  {
    // Rely on endianness and reading 32bit at unaligned addresses... TODO: Implement for non-x86
    return (*reinterpret_cast<const ezUInt32*>(pSource + (uiBitPosition >> 3)) >> (uiBitPosition & 7)) & ((1U << uiNumBits) - 1);

    // Slow but platform independent version:
    /*ezUInt32 uiResult = 0;
    for(ezUInt32 uiBit = uiBitPosition; uiBit < uiBitPosition + uiNumBits; uiBit++)
    {
      uiResult |= ((pSource[uiBit >> 3] >> (uiBit & 7)) & 1) << (uiBit - uiBitPosition);
    }
    return uiResult;*/
  }
};

namespace
{
  struct ezImageConversionFunctionEntry
  {
    ezImageConversionFunctionEntry(ezImageConversion::ConversionFunction pFunction, ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) :
      m_pFunction(pFunction), m_sourceFormat(sourceFormat), m_targetFormat(targetFormat), m_fCost(1.0f)
    {}

    ezImageConversion::ConversionFunction m_pFunction;
    ezImageFormat::Enum m_sourceFormat;
    ezImageFormat::Enum m_targetFormat;
    float m_fCost;
  };

  ezDynamicArray<ezImageConversionFunctionEntry>* s_conversionFunctions = NULL;

  ezStaticArray<ezInt32, ezImageFormat::NUM_FORMATS * ezImageFormat::NUM_FORMATS> s_conversionTable;
  ezStaticArray<float, ezImageFormat::NUM_FORMATS * ezImageFormat::NUM_FORMATS> s_costTable;
  bool s_bConversionTableValid = false;

  ezUInt32 GetTableIndex(ezUInt32 sourceFormat, ezUInt32 targetFormat)
  {
    return sourceFormat * ezImageFormat::NUM_FORMATS + targetFormat;
  }

  void CopyImage(const ezImage& source, ezImage& target)
  {
    EZ_ASSERT(
      ezImageFormat::GetBitsPerPixel(source.GetImageFormat()) == ezImageFormat::GetBitsPerPixel(target.GetImageFormat()),
      "Can only copy images with matching pixel sizes");

    target.SetWidth(source.GetWidth());
    target.SetHeight(source.GetHeight());
    target.SetDepth(source.GetDepth());
    target.SetNumMipLevels(source.GetNumMipLevels());
    target.SetNumFaces(source.GetNumFaces());
    target.SetNumArrayIndices(source.GetNumArrayIndices());

    target.AllocateImageData();

    for(ezUInt32 uiArrayIndex = 0; uiArrayIndex < source.GetNumArrayIndices(); uiArrayIndex++)
    {
      for(ezUInt32 uiFace = 0; uiFace < source.GetNumFaces(); uiFace++)
      {
        for(ezUInt32 uiMipLevel = 0; uiMipLevel < source.GetNumMipLevels(); uiMipLevel++)
        {
          const ezUInt32 uiSourceRowPitch = source.GetRowPitch(uiMipLevel, uiFace, uiArrayIndex);
          const ezUInt32 uiTargetRowPitch = target.GetRowPitch(uiMipLevel, uiFace, uiArrayIndex);

          for(ezUInt32 uiSlice = 0; uiSlice < source.GetDepth(uiMipLevel); uiSlice++)
          {
            const char* pSource = source.GetPixelPointer<char>(uiMipLevel, uiFace, uiArrayIndex, 0, 0, uiSlice);
            char* pTarget = target.GetPixelPointer<char>(uiMipLevel, uiFace, uiArrayIndex, 0, 0, uiSlice);
            for(ezUInt32 uiRow = 0; uiRow < source.GetHeight(uiMipLevel); uiRow++)
            {
              ezMemoryUtils::Copy(pTarget, pSource, ezMath::Min(uiSourceRowPitch, uiTargetRowPitch));
              pSource += uiSourceRowPitch;
              pTarget += uiTargetRowPitch;
            }
          }
        }
      }
    }
  }
}

ezResult ezImageConversion::Convert(const ezImage& source, ezImage& target)
{
  ezImageFormat::Enum sourceFormat = source.GetImageFormat();
  const ezImageFormat::Enum targetFormat = target.GetImageFormat();

  if(!s_bConversionTableValid)
  {
    RebuildConversionTable();
  }

  ezUInt32 uiCurrentTableIndex = GetTableIndex(sourceFormat, targetFormat);

  // No conversion known
  if(s_conversionTable[uiCurrentTableIndex] == -1)
  {
    return EZ_FAILURE;
  }

  // Count number of conversion steps
  ezUInt32 uiConversionSteps = 1;
  while((*s_conversionFunctions)[s_conversionTable[uiCurrentTableIndex]].m_targetFormat != targetFormat)
  {
    uiCurrentTableIndex = GetTableIndex((*s_conversionFunctions)[s_conversionTable[uiCurrentTableIndex]].m_targetFormat, targetFormat);
    uiConversionSteps++;
  }

  ezImage intermediate;

  // To convert, we ping-pong between the target image and an intermediate. Choose the first target depending on the parity of the conversion
  // chain length so that the final conversion ends up in the target image.
  const ezImage* pSource = &source;
  ezImage* pTarget = NULL;
  ezImage* pSwapTarget = NULL;

  if(uiConversionSteps % 2 == 0)
  {
    pTarget = &intermediate;
    pSwapTarget = &target;
  }
  else
  {
    pTarget = &target;
    pSwapTarget = &intermediate;
  }

  // Apply conversion
  uiCurrentTableIndex = GetTableIndex(sourceFormat, targetFormat);
  for(ezUInt32 uiStep = 0; uiStep < uiConversionSteps; uiStep++)
  {
    const ezImageConversionFunctionEntry& entry = (*s_conversionFunctions)[s_conversionTable[GetTableIndex(sourceFormat, targetFormat)]];
    pTarget->SetImageFormat(entry.m_targetFormat);
    entry.m_pFunction(*pSource, *pTarget);

    pSource = pTarget;

    ezMath::Swap(pTarget, pSwapTarget);

    sourceFormat = entry.m_targetFormat;
  }

  return EZ_SUCCESS;
}

void ezImageConversion::RegisterConversionFunction(ConversionFunction pFunction, ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat)
{
  s_conversionFunctions->PushBack(ezImageConversionFunctionEntry(pFunction, sourceFormat, targetFormat));
  s_bConversionTableValid = false;
}

void ezImageConversion::RebuildConversionTable()
{
  // Straight from http://en.wikipedia.org/wiki/Floyd-Warshall_algorithm
  s_conversionTable.SetCount(ezImageFormat::NUM_FORMATS * ezImageFormat::NUM_FORMATS);
  s_costTable.SetCount(ezImageFormat::NUM_FORMATS * ezImageFormat::NUM_FORMATS);

  for(ezUInt32 tableIdx = 0; tableIdx < s_conversionTable.GetCount(); tableIdx++)
  {
    s_conversionTable[tableIdx] = -1;
    s_costTable[tableIdx] = ezMath::BasicType<float>::GetInfinity();
  }

  // Prime conversion table with known functions
  for(ezUInt32 uiConversionIdx = 0; uiConversionIdx < s_conversionFunctions->GetCount(); uiConversionIdx++)
  {
    const ezImageConversionFunctionEntry& entry = (*s_conversionFunctions)[uiConversionIdx];
    ezUInt32 uiTableIndex = GetTableIndex(entry.m_sourceFormat, entry.m_targetFormat);
    s_conversionTable[uiTableIndex] = uiConversionIdx;
    s_costTable[uiTableIndex] = entry.m_fCost;
  }

  for(ezUInt32 k = 0; k < ezImageFormat::NUM_FORMATS; k++)
  {
    for(ezUInt32 i = 0; i < ezImageFormat::NUM_FORMATS; i++)
    {
      ezUInt32 uiTableIndexIK = GetTableIndex(i, k);
      for(ezUInt32 j = 0; j < ezImageFormat::NUM_FORMATS; j++)
      {
        ezUInt32 uiTableIndexIJ = GetTableIndex(i, j);
        ezUInt32 uiTableIndexKJ = GetTableIndex(k, j);
        if(s_costTable[uiTableIndexIK] + s_costTable[uiTableIndexKJ] < s_costTable[uiTableIndexIJ])
        {
          s_costTable[uiTableIndexIJ] = s_costTable[uiTableIndexIK] + s_costTable[uiTableIndexKJ];

          // To convert from format I to format J, first convert from I to K
          s_conversionTable[uiTableIndexIJ] = s_conversionTable[uiTableIndexIK];
        }
      }
    }
  }

  s_bConversionTableValid = true;
}

void ezImageConversion::Startup()
{
  s_conversionFunctions = new ezDynamicArray<ezImageConversionFunctionEntry>();

  bool bSupportsSSSE3 = true;
  if(bSupportsSSSE3)
  {
    // BGRA => RGBA swizzling
    RegisterConversionFunction(ezImageSwizzleConversion32_2103_SSSE3::ConvertImage, ezImageFormat::B8G8R8A8_TYPELESS, ezImageFormat::R8G8B8A8_TYPELESS);
    RegisterConversionFunction(ezImageSwizzleConversion32_2103_SSSE3::ConvertImage, ezImageFormat::B8G8R8A8_UNORM, ezImageFormat::R8G8B8A8_UNORM);
    RegisterConversionFunction(ezImageSwizzleConversion32_2103_SSSE3::ConvertImage, ezImageFormat::B8G8R8A8_UNORM_SRGB, ezImageFormat::R8G8B8A8_UNORM_SRGB);

    // RGBA => BGRA swizzling
    RegisterConversionFunction(ezImageSwizzleConversion32_2103_SSSE3::ConvertImage, ezImageFormat::R8G8B8A8_TYPELESS, ezImageFormat::B8G8R8A8_TYPELESS);
    RegisterConversionFunction(ezImageSwizzleConversion32_2103_SSSE3::ConvertImage, ezImageFormat::R8G8B8A8_UNORM, ezImageFormat::B8G8R8A8_UNORM);
    RegisterConversionFunction(ezImageSwizzleConversion32_2103_SSSE3::ConvertImage, ezImageFormat::R8G8B8A8_UNORM_SRGB, ezImageFormat::B8G8R8A8_UNORM_SRGB);
  }
  else
  {
    // BGRA => RGBA swizzling
    RegisterConversionFunction(ezImageSwizzleConversion32_2103_SSE2::ConvertImage, ezImageFormat::B8G8R8A8_TYPELESS, ezImageFormat::R8G8B8A8_TYPELESS);
    RegisterConversionFunction(ezImageSwizzleConversion32_2103_SSE2::ConvertImage, ezImageFormat::B8G8R8A8_UNORM, ezImageFormat::R8G8B8A8_UNORM);
    RegisterConversionFunction(ezImageSwizzleConversion32_2103_SSE2::ConvertImage, ezImageFormat::B8G8R8A8_UNORM_SRGB, ezImageFormat::R8G8B8A8_UNORM_SRGB);

    // RGBA => BGRA swizzling
    RegisterConversionFunction(ezImageSwizzleConversion32_2103_SSE2::ConvertImage, ezImageFormat::R8G8B8A8_TYPELESS, ezImageFormat::B8G8R8A8_TYPELESS);
    RegisterConversionFunction(ezImageSwizzleConversion32_2103_SSE2::ConvertImage, ezImageFormat::R8G8B8A8_UNORM, ezImageFormat::B8G8R8A8_UNORM);
    RegisterConversionFunction(ezImageSwizzleConversion32_2103_SSE2::ConvertImage, ezImageFormat::R8G8B8A8_UNORM_SRGB, ezImageFormat::B8G8R8A8_UNORM_SRGB);
  }

  // Alpha can be converted to unused component by simply copying
  RegisterConversionFunction(CopyImage, ezImageFormat::B5G5R5A1_UNORM, ezImageFormat::B5G5R5X1_UNORM);

  RegisterConversionFunction(ezImageConversionF32_U8::ConvertImage, ezImageFormat::R32G32B32A32_FLOAT, ezImageFormat::R8G8B8A8_UNORM);

  RegisterConversionFunction(ezImageConversion4444_8888::ConvertImage, ezImageFormat::B4G4R4A4_UNORM, ezImageFormat::B8G8R8A8_UNORM);

  RegisterConversionFunction(ezImageConversion_BC1_BGRA::ConvertImage, ezImageFormat::BC1_UNORM, ezImageFormat::B8G8R8A8_UNORM);
  RegisterConversionFunction(ezImageConversion_BC2_BGRA::ConvertImage, ezImageFormat::BC2_UNORM, ezImageFormat::B8G8R8A8_UNORM);
  RegisterConversionFunction(ezImageConversion_BC3_BGRA::ConvertImage, ezImageFormat::BC3_UNORM, ezImageFormat::B8G8R8A8_UNORM);
  RegisterConversionFunction(ezImageConversion_BC4_R::ConvertImage, ezImageFormat::BC4_UNORM, ezImageFormat::R8_UNORM);
  RegisterConversionFunction(ezImageConversion_BC5_RG::ConvertImage, ezImageFormat::BC5_UNORM, ezImageFormat::R8G8_UNORM);
  RegisterConversionFunction(ezImageConversion_BC6U_RGBA::ConvertImage, ezImageFormat::BC6H_UF16, ezImageFormat::R32G32B32A32_FLOAT);

  for(ezUInt32 uiFormat = 0; uiFormat < ezImageFormat::NUM_FORMATS; uiFormat++)
  {
    ezImageFormat::Enum format = static_cast<ezImageFormat::Enum>(uiFormat);
    RegisterConversionFunction(CopyImage, format, format);
  }
}

void ezImageConversion::Shutdown()
{
  delete s_conversionFunctions;
}

ezImageFormat::Enum
  ezImageConversion::FindClosestCompatibleFormat(ezImageFormat::Enum format, const ezImageFormat::Enum* pCompatibleFormats, ezUInt32 uiNumCompatible)
{
  if(!s_bConversionTableValid)
  {
    RebuildConversionTable();
  }

  ezImageFormat::Enum bestMatch = ezImageFormat::UNKNOWN;
  float fBestCost = ezMath::BasicType<float>::GetInfinity();

  for(ezUInt32 uiIndex = 0; uiIndex < uiNumCompatible; uiIndex++)
  {
    if(s_costTable[GetTableIndex(format, pCompatibleFormats[uiIndex])] < fBestCost)
    {
      fBestCost = s_costTable[GetTableIndex(format, pCompatibleFormats[uiIndex])];
      bestMatch = pCompatibleFormats[uiIndex];
    }
  }

  return bestMatch;
}

