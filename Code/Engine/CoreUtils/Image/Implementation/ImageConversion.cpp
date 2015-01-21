#include <CoreUtils/PCH.h>
#include <CoreUtils/Image/ImageConversion.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezImageConversionBase);

namespace
{
  ezStaticArray<const ezImageConversionBase*, ezImageFormat::NUM_FORMATS * ezImageFormat::NUM_FORMATS> s_conversionTable;
  ezStaticArray<ezUInt32, ezImageFormat::NUM_FORMATS * ezImageFormat::NUM_FORMATS> s_subConversionTable;
  ezStaticArray<float, ezImageFormat::NUM_FORMATS * ezImageFormat::NUM_FORMATS> s_costTable;
  bool s_bConversionTableValid = false;

  ezUInt32 GetTableIndex(ezUInt32 sourceFormat, ezUInt32 targetFormat)
  {
    return sourceFormat * ezImageFormat::NUM_FORMATS + targetFormat;
  }

  float GetConversionCost(ezBitflags<ezImageConversionFlags> flags)
  {
    // Base conversion cost
    float fCost = 1.0f;

    // Penalty for conversions that can't be performed in-place
    if (!flags.IsSet(ezImageConversionFlags::InPlace))
    {
      fCost *= 2;
    }

    // Penalty for conversions that are lossy
    if (flags.IsSet(ezImageConversionFlags::Lossy))
    {
      fCost *= 4;
    }

    return fCost;
  }
}

void ezImageConversionBase::RebuildConversionTable()
{
  s_conversionTable.SetCount(ezImageFormat::NUM_FORMATS * ezImageFormat::NUM_FORMATS);
  s_subConversionTable.SetCount(ezImageFormat::NUM_FORMATS * ezImageFormat::NUM_FORMATS);
  s_costTable.SetCount(ezImageFormat::NUM_FORMATS * ezImageFormat::NUM_FORMATS);

  for (ezUInt32 tableIdx = 0; tableIdx < s_conversionTable.GetCount(); tableIdx++)
  {
    s_conversionTable[tableIdx] = nullptr;
    s_costTable[tableIdx] = ezMath::BasicType<float>::GetInfinity();
  }

  // Prime conversion table with known conversions
  for (ezImageConversionBase* pConversion = ezImageConversionBase::GetFirstInstance(); pConversion; pConversion = pConversion->GetNextInstance())
  {
    for (ezUInt32 uiSubConversion = 0; uiSubConversion < pConversion->m_subConversions.GetCount(); uiSubConversion++)
    {
      const SubConversion& subConversion = pConversion->m_subConversions[uiSubConversion];

      if (subConversion.m_flags.IsSet(ezImageConversionFlags::InPlace))
      {
        EZ_ASSERT_DEBUG(ezImageFormat::GetBitsPerPixel(subConversion.m_sourceFormat) == ezImageFormat::GetBitsPerPixel(subConversion.m_targetFormat),
          "In-place conversions are only allowed between formats of the same number of bits per pixel");
        EZ_ASSERT_DEBUG(ezImageFormat::GetType(subConversion.m_sourceFormat) == ezImageFormat::GetType(subConversion.m_targetFormat),
          "In-place conversions are only allowed between formats of the same type");
      }

      ezUInt32 uiTableIndex = GetTableIndex(subConversion.m_sourceFormat, subConversion.m_targetFormat);

      // Use the cheapest known conversion for each combination in case there are multiple ones
      if (s_costTable[uiTableIndex] > GetConversionCost(subConversion.m_flags))
      {
        s_conversionTable[uiTableIndex] = pConversion;
        s_subConversionTable[uiTableIndex] = uiSubConversion;
        s_costTable[uiTableIndex] = GetConversionCost(subConversion.m_flags);
      }
    }
  }

  for (ezUInt32 i = 0; i < ezImageFormat::NUM_FORMATS; i++)
  {
    // Add copy-conversion (from and to same format)
    s_costTable[GetTableIndex(i, i)] = GetConversionCost(ezImageConversionFlags::InPlace);
  }

  // Straight from http://en.wikipedia.org/wiki/Floyd-Warshall_algorithm
  for (ezUInt32 k = 0; k < ezImageFormat::NUM_FORMATS; k++)
  {
    for (ezUInt32 i = 0; i < ezImageFormat::NUM_FORMATS; i++)
    {
      ezUInt32 uiTableIndexIK = GetTableIndex(i, k);
      for (ezUInt32 j = 0; j < ezImageFormat::NUM_FORMATS; j++)
      {
        ezUInt32 uiTableIndexIJ = GetTableIndex(i, j);
        ezUInt32 uiTableIndexKJ = GetTableIndex(k, j);
        if (s_costTable[uiTableIndexIK] + s_costTable[uiTableIndexKJ] < s_costTable[uiTableIndexIJ])
        {
          s_costTable[uiTableIndexIJ] = s_costTable[uiTableIndexIK] + s_costTable[uiTableIndexKJ];

          const ezImageConversionBase* pConversion = s_conversionTable[uiTableIndexIK];

          // To convert from format I to format J, first convert from I to K
          s_conversionTable[uiTableIndexIJ] = pConversion;
          s_subConversionTable[uiTableIndexIJ] = s_subConversionTable[uiTableIndexIK];
        }
      }
    }
  }

  s_bConversionTableValid = true;
}


ezResult ezImageConversionBase::Convert(const ezImage& source, ezImage& target, ezImageFormat::Enum targetFormat)
{
  ezImageFormat::Enum sourceFormat = source.GetImageFormat();

  // Trivial copy
  if (sourceFormat == targetFormat)
  {
    target = source;
    return EZ_SUCCESS;
  }

  if (!s_bConversionTableValid)
  {
    RebuildConversionTable();
  }

  ezUInt32 uiCurrentTableIndex = GetTableIndex(sourceFormat, targetFormat);

  // No conversion known
  if (s_conversionTable[uiCurrentTableIndex] == nullptr)
  {
    return EZ_FAILURE;
  }

  const ezImageConversionBase* pConversion = s_conversionTable[uiCurrentTableIndex];
  const SubConversion& subConversion = pConversion->m_subConversions[s_subConversionTable[uiCurrentTableIndex]];

  if (subConversion.m_targetFormat == targetFormat)
  {
    if (&source == &target && !subConversion.m_flags.IsSet(ezImageConversionFlags::InPlace))
    {
      ezImage copy = source;
      return pConversion->DoConvert(copy, target, subConversion.m_targetFormat);
    }
    else
    {
      return pConversion->DoConvert(source, target, subConversion.m_targetFormat);
    }
  }
  else
  {
    ezImage intermediate;
    if (pConversion->DoConvert(source, intermediate, subConversion.m_targetFormat) == EZ_FAILURE)
    {
      return EZ_FAILURE;
    }

    return Convert(intermediate, target, targetFormat);
  }
}

ezImageConversionBase::ezImageConversionBase()
{
  s_bConversionTableValid = false;
}

ezImageConversionBase::~ezImageConversionBase()
{
  s_bConversionTableValid = false;
}

ezImageFormat::Enum ezImageConversionBase::FindClosestCompatibleFormat(ezImageFormat::Enum format, const ezImageFormat::Enum* pCompatibleFormats, ezUInt32 uiNumCompatible)
{
  if (!s_bConversionTableValid)
  {
    RebuildConversionTable();
  }

  float fBestCost = ezMath::BasicType<float>::GetInfinity();
  ezImageFormat::Enum bestFormat = ezImageFormat::UNKNOWN;

  for (ezUInt32 uiTargetIndex = 0; uiTargetIndex < uiNumCompatible; uiTargetIndex++)
  {
    float fCost = s_costTable[GetTableIndex(format, pCompatibleFormats[uiTargetIndex])];
    if (fCost < fBestCost)
    {
      fBestCost = fCost;
      bestFormat = pCompatibleFormats[uiTargetIndex];
    }
  }

  return bestFormat;
}



EZ_STATICLINK_FILE(CoreUtils, CoreUtils_Image_Implementation_ImageConversion);

