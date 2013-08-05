#include <ezImageConversion.h>

#include <Foundation/Containers/Bitfield.h>
#include "ezImageConversionMixin.h"


struct ezImageConversion4444_8888 : public ezImageConversionMixin<ezImageConversion4444_8888>
{
  static const ezUInt32 s_uiSourceBpp = 16;
  static const ezUInt32 s_uiTargetBpp = 32;
  static const ezUInt32 s_uiMultiConversionSize = 1;

  typedef ezUInt8 SourceTypeSingle;
  typedef ezUInt8 TargetTypeSingle;

  static void ConvertSingle(const SourceTypeSingle* pSource, TargetTypeSingle* pTarget)
  {
    pTarget[0] = (pSource[0] & 0x0F) * 17;
    pTarget[1] = (pSource[0] >> 4) * 17;
    pTarget[2] = (pSource[1] & 0x0F) * 17;
    pTarget[3] = (pSource[1] >> 4) * 17;
  }

  typedef SourceTypeSingle SourceTypeMultiple;
  typedef TargetTypeSingle TargetTypeMultiple;

  static void ConvertMultiple(const SourceTypeMultiple* pSource, TargetTypeMultiple* pTarget)
  {
    return ConvertSingle(pSource, pTarget);
  }
};


struct ezImageSwizzleConversion32_2103_SSE2 : public ezImageConversionMixin<ezImageSwizzleConversion32_2103_SSE2>
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

struct ezImageSwizzleConversion32_2103_SSSE3 : public ezImageConversionMixin<ezImageSwizzleConversion32_2103_SSSE3>
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
            const char* pSource = source.GetDataPointer<char>(uiMipLevel, uiFace, uiArrayIndex, 0, 0, uiSlice);
            char* pTarget = target.GetDataPointer<char>(uiMipLevel, uiFace, uiArrayIndex, 0, 0, uiSlice);
            for(ezUInt32 uiRow = 0; uiRow < source.GetHeight(uiMipLevel); uiRow++)
            {
              memcpy(pTarget, pSource, ezMath::Min(uiSourceRowPitch, uiTargetRowPitch));
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

  RegisterConversionFunction(ezImageConversion4444_8888::ConvertImage, ezImageFormat::B4G4R4A4_UNORM, ezImageFormat::B8G8R8A8_UNORM);

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

