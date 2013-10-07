#include <Foundation/PCH.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <Foundation/Strings/StringUtils.h>

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

ezResult ezConversionUtils::StringToInt(const char* szString, ezInt32& out_Res, const char** out_LastParsePosition)
{
  if (ezStringUtils::IsNullOrEmpty(szString))
    return EZ_FAILURE;

  bool bSignIsPos = true;

  if (FindFirstDigit(szString, bSignIsPos) == EZ_FAILURE)
    return EZ_FAILURE;

  ezInt32 iCurRes = 0;

  while (*szString != '\0')
  {
    const char c = *szString;

    // end of digits reached -> return success (allows to write something like "239*4" -> parses first part as 239)
    if (c < '0' || c > '9')
      break;

    if (iCurRes >= 214748364) // going to overflow
      return EZ_FAILURE;

    iCurRes *= 10; // shift all previously read digits to the left

    iCurRes += c - '0'; // add the last digit

    ++szString;
  }

  if (!bSignIsPos)
    iCurRes = -iCurRes;

  out_Res = iCurRes;

  if (out_LastParsePosition)
    *out_LastParsePosition = szString;

  return EZ_SUCCESS;
}

ezResult ezConversionUtils::StringToFloat(const char* szString, double& out_Res, const char** out_LastParsePosition)
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

ezResult ezConversionUtils::StringToBool(const char* szString, bool& out_Res, const char** out_LastParsePosition)
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

