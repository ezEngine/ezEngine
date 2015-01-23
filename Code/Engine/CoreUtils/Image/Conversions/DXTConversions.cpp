#include <CoreUtils/PCH.h>
#include <CoreUtils/Image/Conversions/DXTConversions.h>
#include <CoreUtils/Image/Conversions/ImageConversionMixin.h>
#include <CoreUtils/Image/Conversions/PixelConversions.h>

void ezDecompressBlockBC1(const ezUInt8* pSource, ezColorLinearUB* pTarget, bool bForceFourColorMode)
{
  ezUInt16 uiColor0 = pSource[0] | (pSource[1] << 8);
  ezUInt16 uiColor1 = pSource[2] | (pSource[3] << 8);

  ezColorLinearUB colors[4];

  colors[0] = ezDecompress565(pSource[0] | (pSource[1] << 8));
  colors[1] = ezDecompress565(pSource[2] | (pSource[3] << 8));

  if (uiColor0 > uiColor1 || bForceFourColorMode)
  {
    colors[2] = ezColorLinearUB(
      (2 * colors[0].r + colors[1].r + 1) / 3,
      (2 * colors[0].g + colors[1].g + 1) / 3,
      (2 * colors[0].b + colors[1].b + 1) / 3,
      0xFF);
    colors[3] = ezColorLinearUB(
      (colors[0].r + 2 * colors[1].r + 1) / 3,
      (colors[0].g + 2 * colors[1].g + 1) / 3,
      (colors[0].b + 2 * colors[1].b + 1) / 3,
      0xFF);
  }
  else
  {
    colors[2] = ezColorLinearUB(
      (colors[0].r + colors[1].r) / 2,
      (colors[0].g + colors[1].g) / 2,
      (colors[0].b + colors[1].b) / 2,
      0xFF);
    colors[3] = ezColorLinearUB(0, 0, 0, 0);
  }

  for (ezUInt32 uiByteIdx = 0; uiByteIdx < 4; uiByteIdx++)
  {
    ezUInt8 uiIndices = pSource[4 + uiByteIdx];

    pTarget[4 * uiByteIdx + 0] = colors[(uiIndices >> 4) & 0x03]; // R
    pTarget[4 * uiByteIdx + 1] = colors[(uiIndices >> 2) & 0x03]; // G
    pTarget[4 * uiByteIdx + 2] = colors[(uiIndices >> 0) & 0x03]; // B
    pTarget[4 * uiByteIdx + 3] = colors[(uiIndices >> 6) & 0x03]; // A
  }
}

void ezDecompressBlockBC4(const ezUInt8* pSource, ezUInt8* pTarget, ezUInt32 uiStride)
{
  ezUInt8 uiAlphas[8];

  uiAlphas[0] = pSource[0];
  uiAlphas[1] = pSource[1];

  if (uiAlphas[0] > uiAlphas[1])
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

  for (ezUInt32 uiTripleIdx = 0; uiTripleIdx < 2; uiTripleIdx++)
  {
    ezUInt32 uiIndices =
      pSource[2 + uiTripleIdx * 3 + 0] << 0 |
      pSource[2 + uiTripleIdx * 3 + 1] << 8 |
      pSource[2 + uiTripleIdx * 3 + 2] << 16;

    pTarget[(8 * uiTripleIdx + 0) * uiStride] = uiAlphas[(uiIndices >> 6) & 0x07];  // R
    pTarget[(8 * uiTripleIdx + 1) * uiStride] = uiAlphas[(uiIndices >> 3) & 0x07];  // G
    pTarget[(8 * uiTripleIdx + 2) * uiStride] = uiAlphas[(uiIndices >> 0) & 0x07];  // B
    pTarget[(8 * uiTripleIdx + 3) * uiStride] = uiAlphas[(uiIndices >> 9) & 0x07];  // A
    pTarget[(8 * uiTripleIdx + 4) * uiStride] = uiAlphas[(uiIndices >> 18) & 0x07]; // R
    pTarget[(8 * uiTripleIdx + 5) * uiStride] = uiAlphas[(uiIndices >> 15) & 0x07]; // G
    pTarget[(8 * uiTripleIdx + 6) * uiStride] = uiAlphas[(uiIndices >> 12) & 0x07]; // B
    pTarget[(8 * uiTripleIdx + 7) * uiStride] = uiAlphas[(uiIndices >> 21) & 0x07]; // A
  }
}

class ezImageConversion_BC1_BGRA : public ezImageConversionMixinBlockDecompression<ezImageConversion_BC1_BGRA>
{
public:
  static const ezUInt32 s_uiSourceBpp = 4;
  static const ezUInt32 s_uiTargetBpp = 32;

  typedef ezUInt8 SourceType;
  typedef ezColorLinearUB TargetType;

  ezImageConversion_BC1_BGRA()
  {
    m_subConversions.PushBack(SubConversion(ezImageFormat::BC1_UNORM, ezImageFormat::B8G8R8A8_UNORM, ezImageConversionFlags::None));
  }

  static void DecompressBlock(const SourceType* pSource, TargetType* pTarget)
  {
    ezDecompressBlockBC1(pSource, pTarget, false);
  }
};

class ezImageConversion_BC2_BGRA : public ezImageConversionMixinBlockDecompression<ezImageConversion_BC2_BGRA>
{
public:
  static const ezUInt32 s_uiSourceBpp = 8;
  static const ezUInt32 s_uiTargetBpp = 32;

  typedef ezUInt8 SourceType;
  typedef ezColorLinearUB TargetType;

  ezImageConversion_BC2_BGRA()
  {
    m_subConversions.PushBack(SubConversion(ezImageFormat::BC2_UNORM, ezImageFormat::B8G8R8A8_UNORM, ezImageConversionFlags::None));
  }

  static void DecompressBlock(const SourceType* pSource, TargetType* pTarget)
  {
    ezDecompressBlockBC1(pSource + 8, pTarget, true);

    for (ezUInt32 uiByteIdx = 0; uiByteIdx < 8; uiByteIdx++)
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
  typedef ezColorLinearUB TargetType;

  ezImageConversion_BC3_BGRA()
  {
    m_subConversions.PushBack(SubConversion(ezImageFormat::BC3_UNORM, ezImageFormat::B8G8R8A8_UNORM, ezImageConversionFlags::None));
  }

  static void DecompressBlock(const SourceType* pSource, TargetType* pTarget)
  {
    ezDecompressBlockBC1(pSource + 8, pTarget, true);
    ezDecompressBlockBC4(pSource, reinterpret_cast<ezUInt8*>(pTarget), 4);
  }
};

class ezImageConversion_BC4_R : public ezImageConversionMixinBlockDecompression<ezImageConversion_BC4_R>
{
public:
  static const ezUInt32 s_uiSourceBpp = 4;
  static const ezUInt32 s_uiTargetBpp = 8;

  typedef ezUInt8 SourceType;
  typedef ezUInt8 TargetType;

  ezImageConversion_BC4_R()
  {
    m_subConversions.PushBack(SubConversion(ezImageFormat::BC4_UNORM, ezImageFormat::R8_UNORM, ezImageConversionFlags::None));
  }

  static void DecompressBlock(const SourceType* pSource, TargetType* pTarget)
  {
    ezDecompressBlockBC4(pSource, pTarget, 1);
  }
};

class ezImageConversion_BC5_RG : public ezImageConversionMixinBlockDecompression<ezImageConversion_BC5_RG>
{
public:
  static const ezUInt32 s_uiSourceBpp = 8;
  static const ezUInt32 s_uiTargetBpp = 16;

  typedef ezUInt8 SourceType;
  typedef ezUInt8 TargetType;

  ezImageConversion_BC5_RG()
  {
    m_subConversions.PushBack(SubConversion(ezImageFormat::BC5_UNORM, ezImageFormat::R8G8_UNORM, ezImageConversionFlags::None));
  }

  static void DecompressBlock(const SourceType* pSource, TargetType* pTarget)
  {
    ezDecompressBlockBC4(pSource + 0, pTarget + 0, 2);
    ezDecompressBlockBC4(pSource + 8, pTarget + 1, 2);
  }
};

static ezImageConversion_BC1_BGRA g_conversionBC1;
static ezImageConversion_BC2_BGRA g_conversionBC2;
static ezImageConversion_BC3_BGRA g_conversionBC3;
static ezImageConversion_BC4_R g_conversionBC4;
static ezImageConversion_BC5_RG g_conversionBC5;



EZ_STATICLINK_FILE(CoreUtils, CoreUtils_Image_Conversions_DXTConversions);

