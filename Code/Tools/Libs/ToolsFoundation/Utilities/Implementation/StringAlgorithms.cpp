#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <ToolsFoundation/Utilities/StringAlgorithms.h>

static void ParseSegments(ezStringView sName, ezTempHybridArray<ezInt32, 8>& out_segs)
{
  out_segs.Clear();
  while (!sName.IsEmpty())
  {
    const char* pDot = sName.FindSubString(".");
    ezStringView sSeg;
    if (pDot == nullptr)
    {
      sSeg = sName;
      sName = ezStringView();
    }
    else
    {
      sSeg = ezStringView(sName.GetStartPointer(), pDot);
      sName = ezStringView(pDot + 1, sName.GetEndPointer());
    }
    ezInt32 val = 0;
    ezConversionUtils::StringToInt(sSeg, val).IgnoreResult();
    out_segs.PushBack(val);
  }
}

static void AppendSegments(ezArrayPtr<const ezInt32> segs, ezStringBuilder& ref_sText)
{
  for (ezUInt32 i = 0; i < segs.GetCount(); ++i)
  {
    if (i > 0)
      ref_sText.Append(".");
    ref_sText.AppendFormat("{}", segs[i]);
  }
}

static void FindMidpointWithVirtual(ezArrayPtr<const ezInt32> leftSegs, ezInt32 iVirtualRight, ezTempHybridArray<ezInt32, 8>& ref_result)
{
  while (true)
  {
    const ezInt32 lRoot = leftSegs.IsEmpty() ? 0 : leftSegs[0];
    const ezInt32 diff = iVirtualRight - lRoot;

    if (diff > 1)
    {
      ref_result.PushBack((lRoot + iVirtualRight) / 2);
      return;
    }

    // diff == 1: adjacent values, go one level deeper
    ref_result.PushBack(lRoot);
    leftSegs = leftSegs.IsEmpty() ? leftSegs : leftSegs.GetSubArray(1);
    iVirtualRight = 10;
  }
}

static void FindMidpoint(ezArrayPtr<const ezInt32> leftSegs, ezArrayPtr<const ezInt32> rightSegs, ezTempHybridArray<ezInt32, 8>& ref_result)
{
  // Strip matching prefix
  while (!leftSegs.IsEmpty() && !rightSegs.IsEmpty() && leftSegs[0] == rightSegs[0])
  {
    ref_result.PushBack(leftSegs[0]);
    leftSegs = leftSegs.GetSubArray(1);
    rightSegs = rightSegs.GetSubArray(1);
  }

  const ezInt32 lRoot = leftSegs.IsEmpty() ? 0 : leftSegs[0];
  const ezInt32 rRoot = rightSegs.IsEmpty() ? lRoot + 1 : rightSegs[0];
  const ezInt32 diff = rRoot - lRoot;

  EZ_ASSERT_DEBUG(diff > 0, "Left segment value must be less than right segment value");

  if (diff > 1)
  {
    ref_result.PushBack((lRoot + rRoot) / 2);
    return;
  }

  // diff == 1: fall back to sub-level notation
  ref_result.PushBack(lRoot);
  const ezArrayPtr<const ezInt32> lTail = leftSegs.IsEmpty() ? leftSegs : leftSegs.GetSubArray(1);
  FindMidpointWithVirtual(lTail, 10, ref_result);
}

/// Splits sName into a leading text prefix and a trailing numeric part.
/// The numeric part begins at the first digit, or at a '-' immediately followed by a digit.
/// ref_sName is updated to point at the numeric part; the prefix is returned.
static ezStringView ExtractTextPrefix(ezStringView& ref_sName)
{
  const char* pStart = ref_sName.GetStartPointer();
  const char* pEnd = ref_sName.GetEndPointer();
  const char* p = pStart;

  while (p < pEnd)
  {
    const char c = *p;
    if (c >= '0' && c <= '9')
      break;
    if (c == '-' && p + 1 < pEnd && p[1] >= '0' && p[1] <= '9')
      break;
    ++p;
  }

  ezStringView prefix(pStart, p);
  ref_sName = ezStringView(p, pEnd);
  return prefix;
}

void ezStringAlgorithms::ComputeNameBetween(ezStringView sLeft, ezStringView sRight, ezStringBuilder& ref_sName)
{
  if (sLeft.IsEmpty() && sRight.IsEmpty())
  {
    ref_sName = "1";
    return;
  }

  // Strip any leading text prefix (non-numeric characters) from both names.
  // The result inherits the left prefix, or the right prefix when prepending.
  ezStringView sLeftNumeric = sLeft;
  ezStringView sRightNumeric = sRight;
  ezStringView sLeftPrefix, sRightPrefix;
  if (!sLeft.IsEmpty())
    sLeftPrefix = ExtractTextPrefix(sLeftNumeric);
  if (!sRight.IsEmpty())
    sRightPrefix = ExtractTextPrefix(sRightNumeric);

  const ezStringView sResultPrefix = sLeft.IsEmpty() ? sRightPrefix : sLeftPrefix;

  ezTempHybridArray<ezInt32, 8> lSegs, rSegs, midSegs;

  if (sLeft.IsEmpty())
  {
    // Prepend: go one below the right neighbor's root integer (may produce negative numbers)
    ParseSegments(sRightNumeric, rSegs);
    midSegs.PushBack((rSegs.IsEmpty() ? 0 : rSegs[0]) - 1);
  }
  else if (sRight.IsEmpty())
  {
    // Append: go one above the left neighbor's root integer
    ParseSegments(sLeftNumeric, lSegs);
    midSegs.PushBack((lSegs.IsEmpty() ? 0 : lSegs[0]) + 1);
  }
  else
  {
    ParseSegments(sLeftNumeric, lSegs);
    ParseSegments(sRightNumeric, rSegs);
    FindMidpoint(lSegs, rSegs, midSegs);
  }

  ref_sName.Clear();
  ref_sName.Append(sResultPrefix);
  if (!sResultPrefix.IsEmpty() && *(sResultPrefix.GetEndPointer() - 1) != ' ')
    ref_sName.Append(" ");
  AppendSegments(midSegs, ref_sName);
}
