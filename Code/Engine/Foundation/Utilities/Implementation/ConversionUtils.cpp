#include <Foundation/PCH.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <Foundation/Strings/StringUtils.h>

namespace ezConversionUtils
{

static bool IsWhitespace(char c)
{
  return (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f' || c == '\a');
}

static void SkipWhitespace(const char*& szString)
{
  if (szString == NULL)
    return;

  while (*szString != '\0' && IsWhitespace(*szString))
  {
    ++szString;
  }
}

static ezResult FindFirstDigit(const char*& inout_szString, bool& out_bSignIsPositive)
{
  out_bSignIsPositive = true;

  // we are only looking at ASCII characters here, so no need to decode Utf8 sequences

  while (*inout_szString != '\0')
  {
    const char c = *inout_szString;

    // found a digit
    if (c >= '0' && c <= '9')
      break;

    // skip all whitespaces
    if (IsWhitespace(c))
    {
      ++inout_szString;
      continue;
    }

    // NO change sign, just ignore + signs
    if (c == '+')
    {
      ++inout_szString;
      continue;
    }

    // change sign
    if (c == '-')
    {
      out_bSignIsPositive = !out_bSignIsPositive;
      ++inout_szString;
      continue;
    }

    return EZ_FAILURE;
  }

  // not a single digit found
  if (ezStringUtils::IsNullOrEmpty(inout_szString))
    return EZ_FAILURE;

  // remove all leading zeroes
  while (inout_szString[0] == '0' && inout_szString[1] == '0')
    ++inout_szString;

  // if it is a leading zero before a non-zero digit, remove it (otherwise keep the zero)
  if (inout_szString[0] == '0' && inout_szString[1] >= '1' && inout_szString[1] <= '9')
    ++inout_szString;

  return EZ_SUCCESS;
}

ezResult StringToInt(const char* szString, ezInt32& out_Res, const char** out_LastParsePosition)
{
  ezInt64 tmp = out_Res;
  if (StringToInt64(szString, tmp, out_LastParsePosition) == EZ_SUCCESS && 
    tmp <= (ezInt32)0x7FFFFFFF && tmp >= (ezInt32)0x80000000)
  {
    out_Res = (ezInt32)tmp;
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezResult StringToInt64(const char* szString, ezInt64& out_Res, const char** out_LastParsePosition)
{
  if (ezStringUtils::IsNullOrEmpty(szString))
    return EZ_FAILURE;

  bool bSignIsPos = true;

  if (FindFirstDigit(szString, bSignIsPos) == EZ_FAILURE)
    return EZ_FAILURE;

  ezInt64 iCurRes = 0;
  ezInt64 iSign = bSignIsPos ? 1 : -1;
  const ezInt64 iMax = 0x7FFFFFFFFFFFFFFF;
  const ezInt64 iMin = 0x8000000000000000;

  while (*szString != '\0')
  {
    const char c = *szString;

    // end of digits reached -> return success (allows to write something like "239*4" -> parses first part as 239)
    if (c < '0' || c > '9')
      break;

    const ezInt64 iLastDigit = c - '0';

    if ((iCurRes > iMax / 10) || (iCurRes == iMax / 10 && iLastDigit > 7)) // going to overflow
      return EZ_FAILURE;

    if ((iCurRes < iMin / 10) || (iCurRes == iMin / 10 && iLastDigit > 8)) // going to underflow
      return EZ_FAILURE;

    iCurRes = iCurRes * 10 + iLastDigit * iSign; // shift all previously read digits to the left and add the last digit

    ++szString;
  }

  out_Res = iCurRes;

  if (out_LastParsePosition != NULL)
    *out_LastParsePosition = szString;

  return EZ_SUCCESS;
}

ezResult StringToFloat(const char* szString, double& out_Res, const char** out_LastParsePosition)
{
  if (ezStringUtils::IsNullOrEmpty(szString))
    return EZ_FAILURE;

  bool bSignIsPos = true;

  if (FindFirstDigit(szString, bSignIsPos) == EZ_FAILURE)
  {
    // if it is a '.' continue (this is valid)
    if (*szString != '.')
      return EZ_FAILURE;
  }

  enum NumberPart
  {
    Integer,
    Fraction,
    Exponent,
  };

  NumberPart Part = Integer;

  ezUInt64 uiIntegerPart = 0;     // with 64 Bit to represent the values a 32 Bit float value can be stored, but a 64 Bit double cannot
  ezUInt64 uiFractionalPart = 0;  // lets just assume we won't have such large or precise values stored in text form
  ezUInt64 uiFractionDivisor = 1;
  ezUInt64 uiExponentPart = 0;
  bool bExponentIsPositive = true;

  while (*szString != '\0')
  {
    const char c = *szString;

    if (Part == Integer)
    {
      if (c == '.')
      {
        Part = Fraction;
        ++szString;
        continue;
      }

      if (c >= '0' && c <= '9')
      {
        uiIntegerPart *= 10;
        uiIntegerPart += c - '0';
        ++szString;
        continue;
      }

      if (c == 'e')
      {
        Part = Exponent;
        ++szString;

        if (*szString == '-')
        {
          bExponentIsPositive = false;
          ++szString;
        }

        continue;
      }
    }
    else if (Part == Fraction)
    {
      if (c >= '0' && c <= '9')
      {
        uiFractionalPart *= 10;
        uiFractionalPart += c - '0';
        uiFractionDivisor *= 10;
        ++szString;
        continue;
      }

      if (c == 'e')
      {
        Part = Exponent;
        ++szString;

        if (*szString == '-')
        {
          bExponentIsPositive = false;
          ++szString;
        }

        continue;
      }
    }
    else if (Part == Exponent)
    {
      if (c >= '0' && c <= '9')
      {
        uiExponentPart *= 10;
        uiExponentPart += c - '0';
        ++szString;
        continue;
      }
    }

    if ((c == 'f') || (c == 'd'))
    {
      // if we find an 'f', skip it as well, then stop parsing
      ++szString;
    }

    // found something that is not part of a float value -> stop parsing here
    break;
  }

  // we might lose some precision here, but at least up to this point no precision loss was accumulated yet
  out_Res = (double) uiIntegerPart + (double) uiFractionalPart / (double) uiFractionDivisor;

  if (!bSignIsPos)
    out_Res = -out_Res;

  if (out_LastParsePosition)
    *out_LastParsePosition = szString;

  if (Part == Exponent)
  {
    if (bExponentIsPositive)
      out_Res *= ezMath::Pow(10.0, (double) uiExponentPart);
    else
      out_Res /= ezMath::Pow(10.0, (double) uiExponentPart);
  }

  return EZ_SUCCESS;
}

ezResult StringToBool(const char* szString, bool& out_Res, const char** out_LastParsePosition)
{
  SkipWhitespace(szString);

  if (ezStringUtils::IsNullOrEmpty(szString))
    return EZ_FAILURE;

  // we are only looking at ASCII characters here, so no need to decode Utf8 sequences

  if (ezStringUtils::StartsWith_NoCase(szString, "true"))
  {
    out_Res = true;

    if (out_LastParsePosition)
      *out_LastParsePosition = szString + 4;

    return EZ_SUCCESS;
  }

  if (ezStringUtils::StartsWith_NoCase(szString, "false"))
  {
    out_Res = false;

    if (out_LastParsePosition)
      *out_LastParsePosition = szString + 5;

    return EZ_SUCCESS;
  }

  if (ezStringUtils::StartsWith_NoCase(szString, "on"))
  {
    out_Res = true;

    if (out_LastParsePosition)
      *out_LastParsePosition = szString + 2;

    return EZ_SUCCESS;
  }

  if (ezStringUtils::StartsWith_NoCase(szString, "off"))
  {
    out_Res = false;

    if (out_LastParsePosition)
      *out_LastParsePosition = szString + 3;

    return EZ_SUCCESS;
  }

  if (ezStringUtils::StartsWith_NoCase(szString, "yes"))
  {
    out_Res = true;

    if (out_LastParsePosition)
      *out_LastParsePosition = szString + 3;

    return EZ_SUCCESS;
  }

  if (ezStringUtils::StartsWith_NoCase(szString, "no"))
  {
    out_Res = false;

    if (out_LastParsePosition)
      *out_LastParsePosition = szString + 2;

    return EZ_SUCCESS;
  }

  if (ezStringUtils::StartsWith_NoCase(szString, "1"))
  {
    out_Res = true;

    if (out_LastParsePosition)
      *out_LastParsePosition = szString + 1;

    return EZ_SUCCESS;
  }

  if (ezStringUtils::StartsWith_NoCase(szString, "0"))
  {
    out_Res = false;

    if (out_LastParsePosition)
      *out_LastParsePosition = szString + 1;

    return EZ_SUCCESS;
  }

  if (ezStringUtils::StartsWith_NoCase(szString, "enable"))
  {
    out_Res = true;

    if (out_LastParsePosition)
      *out_LastParsePosition = szString + 6;

    return EZ_SUCCESS;
  }

  if (ezStringUtils::StartsWith_NoCase(szString, "disable"))
  {
    out_Res = false;

    if (out_LastParsePosition)
      *out_LastParsePosition = szString + 7;

    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}


ezString ToString(ezInt32 value)
{
  ezStringBuilder sb;
  sb.Format("%i", value);
  return sb;
}

ezString ToString(ezUInt32 value)
{
  ezStringBuilder sb;
  sb.Format("%u", value);
  return sb;
}

ezString ToString(ezInt64 value)
{
  ezStringBuilder sb;
  sb.Format("%lli", value);
  return sb;
}

ezString ToString(ezUInt64 value)
{
  ezStringBuilder sb;
  sb.Format("%llu", value);
  return sb;
}

ezString ToString(float value)
{
  ezStringBuilder sb;
  sb.Format("%f", value);
  return sb;
}

ezString ToString(double value)
{
  ezStringBuilder sb;
  sb.Format("%f", value);
  return sb;
}

ezString ToString(const ezColor& value)
{
  ezStringBuilder sb;
  sb.Format("{ r=%f, g=%f, b=%f, a=%f }", value.r, value.g, value.b, value.a);
  return sb;
}

ezString ToString(const ezVec2& value)
{
  ezStringBuilder sb;
  sb.Format("{ x=%f, y=%f }", value.x, value.y);
  return sb;
}

ezString ToString(const ezVec3& value)
{
  ezStringBuilder sb;
  sb.Format("{ x=%f, y=%f, z=%f }", value.x, value.y, value.z);
  return sb;
}

ezString ToString(const ezVec4& value)
{
  ezStringBuilder sb;
  sb.Format("{ x=%f, y=%f, z=%f, w=%f }", value.x, value.y, value.z, value.w);
  return sb;
}

ezString ToString(const ezQuat& value)
{
  ezStringBuilder sb;
  sb.Format("{ x=%f, y=%f, z=%f, w=%f }", value.v.x, value.v.y, value.v.z, value.w);
  return sb;
}

ezString ToString(const ezMat3& value)
{
  ezStringBuilder sb;
  sb.Format("{ c1r1=%f, c2r1=%f, c3r1=%f, "
              "c1r2=%f, c2r2=%f, c3r2=%f, "
              "c1r3=%f, c2r3=%f, c3r3=%f }",
              value.Element(0, 0), value.Element(1, 0), value.Element(2, 0),
              value.Element(0, 1), value.Element(1, 1), value.Element(2, 1),
              value.Element(0, 2), value.Element(1, 2), value.Element(2, 2));
  return sb;
}

ezString ToString(const ezMat4& value)
{
  ezStringBuilder sb;
  sb.Format("{ c1r1=%f, c2r1=%f, c3r1=%f, c4r1=%f, "
              "c1r2=%f, c2r2=%f, c3r2=%f, c4r2=%f, "
              "c1r3=%f, c2r3=%f, c3r3=%f, c4r3=%f, "
              "c1r4=%f, c2r4=%f, c3r4=%f, c4r4=%f }",
              value.Element(0, 0), value.Element(1, 0), value.Element(2, 0), value.Element(3, 0),
              value.Element(0, 1), value.Element(1, 1), value.Element(2, 1), value.Element(3, 1),
              value.Element(0, 2), value.Element(1, 2), value.Element(2, 2), value.Element(3, 2),
              value.Element(0, 3), value.Element(1, 3), value.Element(2, 3), value.Element(3, 3));
  return sb;
}

} // namespace ezConvertionUtils


EZ_STATICLINK_REFPOINT(Foundation_Utilities_Implementation_ConversionUtils);

