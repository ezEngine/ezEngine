#include <CoreUtils/PCH.h>
#include <CoreUtils/Image/Conversions/BC6Conversions.h>
#include <CoreUtils/Image/Conversions/ImageConversionMixin.h>

template<bool bSigned>
class ezImageConversion_BC6_RGBA : public ezImageConversionMixinBlockDecompression<ezImageConversion_BC6_RGBA<bSigned> >
{
public:
  static const ezUInt32 s_uiSourceBpp = 8;
  static const ezUInt32 s_uiTargetBpp = 128;

  typedef ezUInt8 SourceType;
  typedef ezColor TargetType;
  
  ezImageConversion_BC6_RGBA()
  {
    if (bSigned)
    {
      ezImageConversionBase::m_subConversions.PushBack(ezImageConversionBase::SubConversion(ezImageFormat::BC6H_SF16, ezImageFormat::R32G32B32A32_FLOAT, ezImageConversionFlags::None));
    }
    else
    {
      ezImageConversionBase::m_subConversions.PushBack(ezImageConversionBase::SubConversion(ezImageFormat::BC6H_UF16, ezImageFormat::R32G32B32A32_FLOAT, ezImageConversionFlags::None));
    }
  }

  static void DecompressBlock(const SourceType* pSource, TargetType* pTarget)
  {
    ezUInt8 uiMode = pSource[0];

    // If the lowest two bits are < 2, the mode is encoded in 2 bits, otherwise in 5.
    if ((uiMode & 0x03) < 2)
    {
      uiMode &= 0x03;
    }
    else
    {
      uiMode &= 0x1F;
    }

    switch (uiMode)
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

    case 15:
      DecompressMode15(pSource, pTarget);
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

private:

  union FP32
  {
    ezUInt32 u;
    float f;
  };

  struct Color
  {
    ezUInt32 r;
    ezUInt32 g;
    ezUInt32 b;

    void operator+=(const Color& other)
    {
      r += other.r;
      g += other.g;
      b += other.b;
    }
  };

  static FP32 HalfToFloat(ezUInt32 uiHalf)
  {
    FP32 out;
    if (bSigned)
    {
      out.u = (((uiHalf & 0x7FFF) << 13) + ((127 - 15) << 23)) | ((uiHalf & 0x8000) << 16);
    }
    else
    {
      out.u = (uiHalf << 12) + ((127 - 15) << 23);
    }
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

    for (ezUInt32 uiIndex = 1; uiIndex < 16; uiIndex++)
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

    for (ezUInt32 uiIndex = 0; uiIndex < 16; uiIndex++)
    {
      ezUInt32 i = 0;
      if (uiIndex == 0)
      {
        i = ReadBits<2, 82>(pSource);
      }
      else if (uiIndex < uiFixUpIndex[uiShape])
      {
        i = ReadBits(pSource, 3, 81 + 3 * uiIndex);
      }
      else if (uiIndex == uiFixUpIndex[uiShape])
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

    if (bSigned)
    {
      SignExtend(A[0], 10, 10, 10);
    }

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

    if (bSigned)
    {
      SignExtend(A[0], 7, 7, 7);
    }

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

    if (bSigned)
    {
      SignExtend(A[0], 11, 11, 11);
    }

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

    if (bSigned)
    {
      SignExtend(A, 10, 10, 10);
      SignExtend(B, 10, 10, 10);
    }

    // In this mode, the end points aren't delta-encoded

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

    if (bSigned)
    {
      SignExtend(A[0], 11, 11, 11);
    }

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

    if (bSigned)
    {
      SignExtend(A, 11, 11, 11);
    }

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

    if (bSigned)
    {
      SignExtend(A[0], 11, 11, 11);
    }

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

    if (bSigned)
    {
      SignExtend(A, 12, 12, 12);
    }

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

    if (bSigned)
    {
      SignExtend(A[0], 9, 9, 9);
    }

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

  static void DecompressMode15(const SourceType* pSource, TargetType* pTarget)
  {
    Color A, B;
    A.r = ReadBits<10, 5>(pSource) | (ReadBits<1, 49>(pSource) << 10) | (ReadBits<1, 48>(pSource) << 11) | (ReadBits<1, 47>(pSource) << 12) |
      (ReadBits<1, 46>(pSource) << 13) | (ReadBits<1, 45>(pSource) << 14) | (ReadBits<1, 44>(pSource) << 15) | (ReadBits<1, 43>(pSource) << 16);
    A.g = ReadBits<10, 15>(pSource) | (ReadBits<1, 59>(pSource) << 10) | (ReadBits<1, 58>(pSource) << 11) | (ReadBits<1, 57>(pSource) << 12) |
      (ReadBits<1, 56>(pSource) << 13) | (ReadBits<1, 55>(pSource) << 14) | (ReadBits<1, 54>(pSource) << 15) | (ReadBits<1, 53>(pSource) << 16);
    A.b = ReadBits<10, 25>(pSource) | (ReadBits<1, 69>(pSource) << 10) | (ReadBits<1, 68>(pSource) << 11) | (ReadBits<1, 67>(pSource) << 12) |
      (ReadBits<1, 66>(pSource) << 13) | (ReadBits<1, 65>(pSource) << 14) | (ReadBits<1, 64>(pSource) << 15) | (ReadBits<1, 63>(pSource) << 16);

    B.r = ReadBits<4, 35>(pSource);
    B.g = ReadBits<4, 45>(pSource);
    B.b = ReadBits<4, 55>(pSource);

    if (bSigned)
    {
      SignExtend(A, 16, 16, 16);
    }

    SignExtend(B, 4, 4, 4);

    B += A;

    // No need to Unquantize since we already have 16b values

    DecodeIndices(pSource, pTarget, A, B);
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

    if (bSigned)
    {
      SignExtend(A[0], 8, 8, 8);
    }

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

    if (bSigned)
    {
      SignExtend(A[0], 8, 8, 8);
    }

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

    if (bSigned)
    {
      SignExtend(A[0], 8, 8, 8);
    }

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

    if (bSigned)
    {
      SignExtend(A[0], 6, 6, 6);
      SignExtend(A[1], 6, 6, 6);
      SignExtend(B[0], 6, 6, 6);
      SignExtend(B[1], 6, 6, 6);
    }

    // In this mode, the end points aren't delta-encoded

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

  static ezUInt32 Unquantize(ezInt32 iComponent, ezUInt32 uiBitsPerComponent)
  {
    if (bSigned)
    {
      ezInt32 iSign = 1;
      if (iComponent < 0)
      {
        iSign = -1;
        iComponent = -iComponent;
      }

      if (iComponent == 0)
      {
        return 0;
      }
      else if (iComponent >= ((1 << (uiBitsPerComponent - 1)) - 1))
      {
        return 0x7FFF * iSign;
      }
      else
      {
        return (((iComponent << 15) + 0x4000) >> (uiBitsPerComponent - 1)) * iSign;
      }
    }
    else
    {
      if (iComponent == 0)
      {
        return 0;
      }
      else if (iComponent == ((1U << uiBitsPerComponent) - 1))
      {
        return 0xFFFF;
      }
      else
      {
        return ((iComponent << 16) + 0x8000) >> uiBitsPerComponent;
      }
    }
  }

  static ezUInt32 FinishUnquantize(ezInt32 iComponent)
  {
    if (bSigned)
    {
      iComponent = (iComponent < 0) ? -(((-iComponent) * 31) >> 5) : (iComponent * 31) >> 5;   // scale the magnitude by 31/32
      int s = 0;
      if (iComponent < 0)
      {
        s = 0x8000;
        iComponent = -iComponent;
      }
      return (unsigned short) (s | iComponent);
    }
    else
    {
      return (iComponent * 31) >> 5;
    }
  }

  template<ezUInt32 uiNumBits, ezUInt32 uiBitPosition>
  static ezUInt32 ReadBits(const ezUInt8* pSource)
  {
    return (*reinterpret_cast<const ezUInt32*>(pSource + (uiBitPosition >> 3)) >> (uiBitPosition & 7)) & ((1U << uiNumBits) - 1);
  }

  static ezUInt32 ReadBits(const ezUInt8* pSource, ezUInt32 uiNumBits, ezUInt32 uiBitPosition)
  {
    // Rely on endianess and reading 32bit at unaligned addresses... 
    /// \todo Implement for non-x86
    return (*reinterpret_cast<const ezUInt32*>(pSource + (uiBitPosition >> 3)) >> (uiBitPosition & 7)) & ((1U << uiNumBits) - 1);

    // Slow but platform independent version:
    /*ezUInt32 uiResult = 0;
    for (ezUInt32 uiBit = uiBitPosition; uiBit < uiBitPosition + uiNumBits; uiBit++)
    {
    uiResult |= ((pSource[uiBit >> 3] >> (uiBit & 7)) & 1) << (uiBit - uiBitPosition);
    }
    return uiResult;*/
  }
};


static ezImageConversion_BC6_RGBA<true> g_conversionBC6S;
static ezImageConversion_BC6_RGBA<false> g_conversionBC6U;



EZ_STATICLINK_FILE(CoreUtils, CoreUtils_Image_Conversions_BC6Conversions);

