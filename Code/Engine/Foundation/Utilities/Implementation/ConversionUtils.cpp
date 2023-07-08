#include <Foundation/FoundationPCH.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Utilities/ConversionUtils.h>

namespace ezConversionUtils
{

  static bool IsWhitespace(ezUInt32 c)
  {
    return (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f' || c == '\a');
  }

  static void SkipWhitespace(ezStringView& ref_sText)
  {
    // we are only looking at ASCII characters here, so no need to decode Utf8 sequences

    while (!ref_sText.IsEmpty() && IsWhitespace(*ref_sText.GetStartPointer()))
    {
      ref_sText.ChopAwayFirstCharacterAscii();
    }
  }

  static ezResult FindFirstDigit(ezStringView& inout_sText, bool& out_bSignIsPositive)
  {
    out_bSignIsPositive = true;

    while (!inout_sText.IsEmpty())
    {
      // we are only looking at ASCII characters here, so no need to decode Utf8 sequences
      const char c = *inout_sText.GetStartPointer();

      // found a digit
      if (c >= '0' && c <= '9')
        break;

      // skip all whitespace
      if (IsWhitespace(c))
      {
        inout_sText.ChopAwayFirstCharacterAscii();
        continue;
      }

      // NO change sign, just ignore + signs
      if (c == '+')
      {
        inout_sText.ChopAwayFirstCharacterAscii();
        continue;
      }

      // change sign
      if (c == '-')
      {
        out_bSignIsPositive = !out_bSignIsPositive;
        inout_sText.ChopAwayFirstCharacterAscii();
        continue;
      }

      return EZ_FAILURE;
    }

    // not a single digit found
    if (inout_sText.IsEmpty())
      return EZ_FAILURE;

    // remove all leading zeros
    while (inout_sText.StartsWith("00"))
    {
      inout_sText.ChopAwayFirstCharacterAscii();
    }

    // if it is a leading zero before a non-zero digit, remove it (otherwise keep the zero)
    if (inout_sText.GetElementCount() >= 2 && inout_sText.StartsWith("0"))
    {
      char c = *(inout_sText.GetStartPointer() + 1);

      if (c >= '1' && c <= '9')
      {
        inout_sText.ChopAwayFirstCharacterAscii();
      }
    }

    return EZ_SUCCESS;
  }

  ezResult StringToInt(ezStringView sText, ezInt32& out_iRes, const char** out_pLastParsePosition)
  {
    ezInt64 tmp = out_iRes;
    if (StringToInt64(sText, tmp, out_pLastParsePosition) == EZ_SUCCESS && tmp <= (ezInt32)0x7FFFFFFF && tmp >= (ezInt32)0x80000000)
    {
      out_iRes = (ezInt32)tmp;
      return EZ_SUCCESS;
    }

    return EZ_FAILURE;
  }

  ezResult StringToUInt(ezStringView sText, ezUInt32& out_uiRes, const char** out_pLastParsePosition)
  {
    ezInt64 tmp = out_uiRes;
    if (StringToInt64(sText, tmp, out_pLastParsePosition) == EZ_SUCCESS && tmp <= (ezUInt32)0xFFFFFFFF && tmp >= 0)
    {
      out_uiRes = (ezUInt32)tmp;
      return EZ_SUCCESS;
    }

    return EZ_FAILURE;
  }

  ezResult StringToInt64(ezStringView sText, ezInt64& out_iRes, const char** out_pLastParsePosition)
  {
    if (sText.IsEmpty())
      return EZ_FAILURE;

    bool bSignIsPos = true;

    if (FindFirstDigit(sText, bSignIsPos) == EZ_FAILURE)
      return EZ_FAILURE;

    ezInt64 iCurRes = 0;
    ezInt64 iSign = bSignIsPos ? 1 : -1;
    const ezInt64 iMax = 0x7FFFFFFFFFFFFFFF;
    const ezInt64 iMin = 0x8000000000000000;

    while (!sText.IsEmpty())
    {
      const char c = *sText.GetStartPointer();

      // end of digits reached -> return success (allows to write something like "239*4" -> parses first part as 239)
      if (c < '0' || c > '9')
        break;

      const ezInt64 iLastDigit = c - '0';

      if ((iCurRes > iMax / 10) || (iCurRes == iMax / 10 && iLastDigit > 7)) // going to overflow
        return EZ_FAILURE;

      if ((iCurRes < iMin / 10) || (iCurRes == iMin / 10 && iLastDigit > 8)) // going to underflow
        return EZ_FAILURE;

      iCurRes = iCurRes * 10 + iLastDigit * iSign; // shift all previously read digits to the left and add the last digit

      sText.ChopAwayFirstCharacterAscii();
    }

    out_iRes = iCurRes;

    if (out_pLastParsePosition != nullptr)
      *out_pLastParsePosition = sText.GetStartPointer();

    return EZ_SUCCESS;
  }

  ezResult StringToFloat(ezStringView sText, double& out_fRes, const char** out_pLastParsePosition)
  {
    if (sText.IsEmpty())
      return EZ_FAILURE;

    bool bSignIsPos = true;

    if (FindFirstDigit(sText, bSignIsPos) == EZ_FAILURE)
    {
      // if it is a '.' continue (this is valid)
      if (!sText.StartsWith("."))
        return EZ_FAILURE;
    }

    enum NumberPart
    {
      Integer,
      Fraction,
      Exponent,
    };

    NumberPart Part = Integer;

    ezUInt64 uiIntegerPart = 0;    // with 64 Bit to represent the values a 32 Bit float value can be stored, but a 64 Bit double cannot
    ezUInt64 uiFractionalPart = 0; // lets just assume we won't have such large or precise values stored in text form
    ezUInt64 uiFractionDivisor = 1;
    ezUInt64 uiExponentPart = 0;
    bool bExponentIsPositive = true;

    while (!sText.IsEmpty())
    {
      const char c = *sText.GetStartPointer();

      // allow underscores in floats for improved readability
      if (c == '_')
      {
        sText.ChopAwayFirstCharacterAscii();
        continue;
      }

      if (Part == Integer)
      {
        if (c == '.')
        {
          Part = Fraction;
          sText.ChopAwayFirstCharacterAscii();
          continue;
        }

        if (c >= '0' && c <= '9')
        {
          uiIntegerPart *= 10;
          uiIntegerPart += c - '0';
          sText.ChopAwayFirstCharacterAscii();
          continue;
        }

        if ((c == 'e') || (c == 'E'))
        {
          Part = Exponent;
          sText.ChopAwayFirstCharacterAscii();

          if (*sText.GetStartPointer() == '-')
          {
            bExponentIsPositive = false;
            sText.ChopAwayFirstCharacterAscii();
          }
          else if (*sText.GetStartPointer() == '+')
          {
            bExponentIsPositive = true;
            sText.ChopAwayFirstCharacterAscii();
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
          sText.ChopAwayFirstCharacterAscii();
          continue;
        }

        if ((c == 'e') || (c == 'E'))
        {
          Part = Exponent;
          sText.ChopAwayFirstCharacterAscii();

          if (*sText.GetStartPointer() == '-')
          {
            bExponentIsPositive = false;
            sText.ChopAwayFirstCharacterAscii();
          }
          else if (*sText.GetStartPointer() == '+')
          {
            bExponentIsPositive = true;
            sText.ChopAwayFirstCharacterAscii();
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
          sText.ChopAwayFirstCharacterAscii();
          continue;
        }
      }

      // found something that is not part of a float value -> stop parsing here
      break;
    }

    // we might lose some precision here, but at least up to this point no precision loss was accumulated yet
    out_fRes = (double)uiIntegerPart + (double)uiFractionalPart / (double)uiFractionDivisor;

    if (!bSignIsPos)
      out_fRes = -out_fRes;

    if (out_pLastParsePosition)
      *out_pLastParsePosition = sText.GetStartPointer();

    if (Part == Exponent)
    {
      if (bExponentIsPositive)
        out_fRes *= ezMath::Pow(10.0, (double)uiExponentPart);
      else
        out_fRes /= ezMath::Pow(10.0, (double)uiExponentPart);
    }

    return EZ_SUCCESS;
  }

  ezResult StringToBool(ezStringView sText, bool& out_bRes, const char** out_pLastParsePosition)
  {
    SkipWhitespace(sText);

    if (sText.IsEmpty())
      return EZ_FAILURE;

    // we are only looking at ASCII characters here, so no need to decode Utf8 sequences

    if (sText.StartsWith("1"))
    {
      out_bRes = true;

      if (out_pLastParsePosition)
        *out_pLastParsePosition = sText.GetStartPointer() + 1;

      return EZ_SUCCESS;
    }

    if (sText.StartsWith("0"))
    {
      out_bRes = false;

      if (out_pLastParsePosition)
        *out_pLastParsePosition = sText.GetStartPointer() + 1;

      return EZ_SUCCESS;
    }

    if (sText.StartsWith_NoCase("true"))
    {
      out_bRes = true;

      if (out_pLastParsePosition)
        *out_pLastParsePosition = sText.GetStartPointer() + 4;

      return EZ_SUCCESS;
    }

    if (sText.StartsWith_NoCase("false"))
    {
      out_bRes = false;

      if (out_pLastParsePosition)
        *out_pLastParsePosition = sText.GetStartPointer() + 5;

      return EZ_SUCCESS;
    }

    if (sText.StartsWith_NoCase("on"))
    {
      out_bRes = true;

      if (out_pLastParsePosition)
        *out_pLastParsePosition = sText.GetStartPointer() + 2;

      return EZ_SUCCESS;
    }

    if (sText.StartsWith_NoCase("off"))
    {
      out_bRes = false;

      if (out_pLastParsePosition)
        *out_pLastParsePosition = sText.GetStartPointer() + 3;

      return EZ_SUCCESS;
    }

    if (sText.StartsWith_NoCase("yes"))
    {
      out_bRes = true;

      if (out_pLastParsePosition)
        *out_pLastParsePosition = sText.GetStartPointer() + 3;

      return EZ_SUCCESS;
    }

    if (sText.StartsWith_NoCase("no"))
    {
      out_bRes = false;

      if (out_pLastParsePosition)
        *out_pLastParsePosition = sText.GetStartPointer() + 2;

      return EZ_SUCCESS;
    }

    if (sText.StartsWith_NoCase("enable"))
    {
      out_bRes = true;

      if (out_pLastParsePosition)
        *out_pLastParsePosition = sText.GetStartPointer() + 6;

      return EZ_SUCCESS;
    }

    if (sText.StartsWith_NoCase("disable"))
    {
      out_bRes = false;

      if (out_pLastParsePosition)
        *out_pLastParsePosition = sText.GetStartPointer() + 7;

      return EZ_SUCCESS;
    }

    return EZ_FAILURE;
  }

  ezUInt32 ExtractFloatsFromString(ezStringView sText, ezUInt32 uiNumFloats, float* out_pFloats, const char** out_pLastParsePosition)
  {
    ezUInt32 uiFloatsFound = 0;

    // just try to extract n floats from the given text
    // if n floats were extracted, or the text end is reached, stop

    while (!sText.IsEmpty() && uiFloatsFound < uiNumFloats)
    {
      double res;
      const char* szPos;

      // if successful, store the float, otherwise advance the string by one, to skip invalid characters
      if (StringToFloat(sText, res, &szPos) == EZ_SUCCESS)
      {
        out_pFloats[uiFloatsFound] = (float)res;
        ++uiFloatsFound;

        sText.SetStartPosition(szPos);
      }
      else
      {
        sText.ChopAwayFirstCharacterUtf8();
      }
    }

    if (out_pLastParsePosition != nullptr)
      *out_pLastParsePosition = sText.GetStartPointer();

    return uiFloatsFound;
  }

  ezInt8 HexCharacterToIntValue(ezUInt32 uiCharacter)
  {
    if (uiCharacter >= '0' && uiCharacter <= '9')
      return static_cast<ezInt8>(uiCharacter - '0');

    if (uiCharacter >= 'a' && uiCharacter <= 'f')
      return static_cast<ezInt8>(uiCharacter - 'a' + 10);

    if (uiCharacter >= 'A' && uiCharacter <= 'F')
      return static_cast<ezInt8>(uiCharacter - 'A' + 10);

    return -1;
  }

  ezResult ConvertHexStringToUInt32(ezStringView sHex, ezUInt32& out_uiResult)
  {
    ezUInt64 uiTemp = 0;
    const ezResult res = ConvertHexStringToUInt(sHex, uiTemp, 8, nullptr);

    out_uiResult = static_cast<ezUInt32>(uiTemp);
    return res;
  }

  ezResult ConvertHexStringToUInt64(ezStringView sHex, ezUInt64& out_uiResult)
  {
    return ConvertHexStringToUInt(sHex, out_uiResult, 16, nullptr);
  }

  ezResult ConvertHexStringToUInt(ezStringView sHex, ezUInt64& out_uiResult, ezUInt32 uiMaxHexCharacters, ezUInt32* pTotalCharactersParsed)
  {
    EZ_ASSERT_DEBUG(uiMaxHexCharacters <= 16, "Only HEX strings of up to 16 character can be parsed into a 64-bit integer");
    const ezUInt32 origStringElementsCount = sHex.GetElementCount();

    out_uiResult = 0;

    // skip 0x
    if (sHex.StartsWith_NoCase("0x"))
      sHex.Shrink(2, 0);

    // convert two characters to one byte, at a time
    for (ezUInt32 i = 0; i < uiMaxHexCharacters; ++i)
    {
      if (sHex.IsEmpty())
      {
        // a shorter/empty string is valid and is just interpreted as a smaller value (e.g. a 32 Bit HEX value)
        break;
      }

      const ezInt8 iValue = ezConversionUtils::HexCharacterToIntValue(sHex.GetCharacter());

      if (iValue < 0)
      {
        // invalid HEX character
        out_uiResult = 0;
        if (pTotalCharactersParsed)
        {
          *pTotalCharactersParsed = 0;
        }
        return EZ_FAILURE;
      }

      out_uiResult <<= 4; // 4 Bits, ie. half a byte
      out_uiResult += iValue;

      sHex.ChopAwayFirstCharacterAscii();
    }

    if (pTotalCharactersParsed)
    {
      EZ_ASSERT_DEBUG(sHex.GetElementCount() <= origStringElementsCount, "");
      *pTotalCharactersParsed = origStringElementsCount - sHex.GetElementCount();
    }

    return EZ_SUCCESS;
  }

  void ConvertHexToBinary(ezStringView sHex, ezUInt8* pBinary, ezUInt32 uiBinaryBuffer)
  {
    // skip 0x
    if (sHex.StartsWith_NoCase("0x"))
      sHex.Shrink(2, 0);

    // convert two characters to one byte, at a time
    // try not to run out of buffer space
    while (sHex.GetElementCount() >= 2 && uiBinaryBuffer >= 1)
    {
      const ezUInt32 c0 = *sHex.GetStartPointer();
      const ezUInt32 c1 = *(sHex.GetStartPointer() + 1);

      ezUInt8 uiValue1 = ezConversionUtils::HexCharacterToIntValue(c0);
      ezUInt8 uiValue2 = ezConversionUtils::HexCharacterToIntValue(c1);
      ezUInt8 uiValue = 16 * uiValue1 + uiValue2;
      *pBinary = uiValue;

      pBinary += 1;
      sHex.Shrink(2, 0);

      uiBinaryBuffer -= 1;
    }
  }

  const ezStringBuilder& ToString(ezInt8 value, ezStringBuilder& out_sResult)
  {
    out_sResult.Format("{0}", (ezInt32)value);
    return out_sResult;
  }

  const ezStringBuilder& ToString(ezUInt8 value, ezStringBuilder& out_sResult)
  {
    out_sResult.Format("{0}", (ezUInt32)value);
    return out_sResult;
  }

  const ezStringBuilder& ToString(ezInt16 value, ezStringBuilder& out_sResult)
  {
    out_sResult.Format("{0}", (ezInt32)value);
    return out_sResult;
  }

  const ezStringBuilder& ToString(ezUInt16 value, ezStringBuilder& out_sResult)
  {
    out_sResult.Format("{0}", (ezUInt32)value);
    return out_sResult;
  }

  const ezStringBuilder& ToString(ezInt32 value, ezStringBuilder& out_sResult)
  {
    out_sResult.Format("{0}", value);
    return out_sResult;
  }

  const ezStringBuilder& ToString(ezUInt32 value, ezStringBuilder& out_sResult)
  {
    out_sResult.Format("{0}", value);
    return out_sResult;
  }

  const ezStringBuilder& ToString(ezInt64 value, ezStringBuilder& out_sResult)
  {
    out_sResult.Format("{0}", value);
    return out_sResult;
  }

  const ezStringBuilder& ToString(ezUInt64 value, ezStringBuilder& out_sResult)
  {
    out_sResult.Format("{0}", value);
    return out_sResult;
  }

  const ezStringBuilder& ToString(float value, ezStringBuilder& out_sResult)
  {
    out_sResult.Format("{0}", value);
    return out_sResult;
  }

  const ezStringBuilder& ToString(double value, ezStringBuilder& out_sResult)
  {
    out_sResult.Format("{0}", value);
    return out_sResult;
  }

  const ezStringBuilder& ToString(const ezColor& value, ezStringBuilder& out_sResult)
  {
    out_sResult.Format("{ r={0}, g={1}, b={2}, a={3} }", value.r, value.g, value.b, value.a);
    return out_sResult;
  }

  const ezStringBuilder& ToString(const ezColorGammaUB& value, ezStringBuilder& out_sResult)
  {
    out_sResult.Format("{ r={0}, g={1}, b={2}, a={3} }", value.r, value.g, value.b, value.a);
    return out_sResult;
  }

  const ezStringBuilder& ToString(const ezVec2& value, ezStringBuilder& out_sResult)
  {
    out_sResult.Format("{ x={0}, y={1} }", value.x, value.y);
    return out_sResult;
  }

  const ezStringBuilder& ToString(const ezVec3& value, ezStringBuilder& out_sResult)
  {
    out_sResult.Format("{ x={0}, y={1}, z={2} }", value.x, value.y, value.z);
    return out_sResult;
  }

  const ezStringBuilder& ToString(const ezVec4& value, ezStringBuilder& out_sResult)
  {
    out_sResult.Format("{ x={0}, y={1}, z={2}, w={3} }", value.x, value.y, value.z, value.w);
    return out_sResult;
  }

  const ezStringBuilder& ToString(const ezVec2I32& value, ezStringBuilder& out_sResult)
  {
    out_sResult.Format("{ x={0}, y={1} }", value.x, value.y);
    return out_sResult;
  }

  const ezStringBuilder& ToString(const ezVec3I32& value, ezStringBuilder& out_sResult)
  {
    out_sResult.Format("{ x={0}, y={1}, z={2} }", value.x, value.y, value.z);
    return out_sResult;
  }

  const ezStringBuilder& ToString(const ezVec4I32& value, ezStringBuilder& out_sResult)
  {
    out_sResult.Format("{ x={0}, y={1}, z={2}, w={3} }", value.x, value.y, value.z, value.w);
    return out_sResult;
  }

  const ezStringBuilder& ToString(const ezQuat& value, ezStringBuilder& out_sResult)
  {
    out_sResult.Format("{ x={0}, y={1}, z={2}, w={3} }", value.v.x, value.v.y, value.v.z, value.w);
    return out_sResult;
  }

  const ezStringBuilder& ToString(const ezMat3& value, ezStringBuilder& out_sResult)
  {
    out_sResult.Printf("{ c1r1=%f, c2r1=%f, c3r1=%f, "
                       "c1r2=%f, c2r2=%f, c3r2=%f, "
                       "c1r3=%f, c2r3=%f, c3r3=%f }",
      value.Element(0, 0), value.Element(1, 0), value.Element(2, 0), value.Element(0, 1), value.Element(1, 1), value.Element(2, 1),
      value.Element(0, 2), value.Element(1, 2), value.Element(2, 2));
    return out_sResult;
  }

  const ezStringBuilder& ToString(const ezMat4& value, ezStringBuilder& out_sResult)
  {
    out_sResult.Printf("{ c1r1=%f, c2r1=%f, c3r1=%f, c4r1=%f, "
                       "c1r2=%f, c2r2=%f, c3r2=%f, c4r2=%f, "
                       "c1r3=%f, c2r3=%f, c3r3=%f, c4r3=%f, "
                       "c1r4=%f, c2r4=%f, c3r4=%f, c4r4=%f }",
      value.Element(0, 0), value.Element(1, 0), value.Element(2, 0), value.Element(3, 0), value.Element(0, 1), value.Element(1, 1),
      value.Element(2, 1), value.Element(3, 1), value.Element(0, 2), value.Element(1, 2), value.Element(2, 2), value.Element(3, 2),
      value.Element(0, 3), value.Element(1, 3), value.Element(2, 3), value.Element(3, 3));
    return out_sResult;
  }

  const ezStringBuilder& ToString(const ezTransform& value, ezStringBuilder& out_sResult)
  {
    ezStringBuilder tmp1, tmp2, tmp3;
    out_sResult.Format("{ position={0}, rotation={1}, scale={2} }", ToString(value.m_vPosition, tmp1), ToString(value.m_qRotation, tmp2),
      ToString(value.m_vScale, tmp3));
    return out_sResult;
  }

  const ezStringBuilder& ToString(const ezAngle& value, ezStringBuilder& out_sResult)
  {
    out_sResult.Format("{0}", value);
    return out_sResult;
  }

  const ezStringBuilder& ToString(const ezTime& value, ezStringBuilder& out_sResult)
  {
    out_sResult.Format("{0}", value);
    return out_sResult;
  }

  const ezStringBuilder& ToString(const ezHashedString& value, ezStringBuilder& out_sResult)
  {
    out_sResult = value.GetView();
    return out_sResult;
  }

  const ezStringBuilder& ToString(const ezTempHashedString& value, ezStringBuilder& out_sResult)
  {
    out_sResult.Format("0x{}", ezArgU(value.GetHash(), 16, true, 16));
    return out_sResult;
  }

  const ezStringBuilder& ToString(const ezDynamicArray<ezVariant>& value, ezStringBuilder& out_sResult)
  {
    out_sResult.Append("[");
    for (const ezVariant& var : value)
    {
      out_sResult.Append(var.ConvertTo<ezString>(), ", ");
    }
    if (!value.IsEmpty())
      out_sResult.Shrink(0, 2);
    out_sResult.Append("]");
    return out_sResult;
  }

  const ezStringBuilder& ToString(const ezHashTable<ezString, ezVariant>& value, ezStringBuilder& out_sResult)
  {
    out_sResult.Append("{");
    for (auto it : value)
    {
      out_sResult.Append(it.Key(), "=", it.Value().ConvertTo<ezString>(), ", ");
    }
    if (!value.IsEmpty())
      out_sResult.Shrink(0, 2);
    out_sResult.Append("}");
    return out_sResult;
  }

  ezUuid ConvertStringToUuid(ezStringView sText);

  const ezStringBuilder& ToString(const ezUuid& value, ezStringBuilder& out_sResult)
  {
    // Windows GUID formatting.
    struct GUID
    {
      ezUInt32 Data1;
      ezUInt16 Data2;
      ezUInt16 Data3;
      ezUInt8 Data4[8];
    };

    const GUID* pGuid = reinterpret_cast<const GUID*>(&value);

    out_sResult.Printf("{ %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x }", pGuid->Data1, pGuid->Data2, pGuid->Data3, pGuid->Data4[0],
      pGuid->Data4[1], pGuid->Data4[2], pGuid->Data4[3], pGuid->Data4[4], pGuid->Data4[5], pGuid->Data4[6], pGuid->Data4[7]);

    return out_sResult;
  }

  const ezStringBuilder& ToString(const ezStringView& value, ezStringBuilder& out_sResult)
  {
    out_sResult = value;
    return out_sResult;
  }

  bool IsStringUuid(ezStringView sText)
  {
    if (sText.GetElementCount() != 40)
      return false;

    if (!sText.StartsWith("{"))
      return false;

    const char* szText = sText.GetStartPointer();

    if ((szText[1] != ' ') || (szText[10] != '-') || (szText[15] != '-') || (szText[20] != '-') || (szText[25] != '-') || (szText[38] != ' ') || (szText[39] != '}'))
    {
      return false;
    }

    return true;
  }

  ezUuid ConvertStringToUuid(ezStringView sText)
  {
    EZ_ASSERT_DEBUG(IsStringUuid(sText), "The given string is not in the correct Uuid format: '{0}'", sText);

    const char* szText = sText.GetStartPointer();

    while (*szText == '{' || ezStringUtils::IsWhiteSpace(*szText))
      ++szText;

    struct GUID
    {
      ezUInt32 Data1;
      ezUInt16 Data2;
      ezUInt16 Data3;
      ezUInt8 Data4[8];
    };

    GUID guid;
    guid.Data1 = 0;
    guid.Data2 = 0;
    guid.Data3 = 0;

    for (int i = 0; i < 8; ++i)
    {
      guid.Data4[i] = 0;
      guid.Data1 = (guid.Data1 << 4) | HexCharacterToIntValue(*szText);
      ++szText;
    }

    // -
    ++szText;
    for (int i = 0; i < 4; ++i)
    {
      guid.Data2 = (guid.Data2 << 4) | HexCharacterToIntValue(*szText);
      ++szText;
    }

    // -
    ++szText;
    for (int i = 0; i < 4; ++i)
    {
      guid.Data3 = (guid.Data3 << 4) | HexCharacterToIntValue(*szText);
      ++szText;
    }

    // -
    ++szText;

    for (int i = 0; i < 2; ++i)
    {
      guid.Data4[i] = (guid.Data4[i] << 4) | HexCharacterToIntValue(*szText);
      ++szText;
      guid.Data4[i] = (guid.Data4[i] << 4) | HexCharacterToIntValue(*szText);
      ++szText;
    }

    // -
    ++szText;

    for (int i = 2; i < 8; ++i)
    {
      guid.Data4[i] = (guid.Data4[i] << 4) | HexCharacterToIntValue(*szText);
      ++szText;
      guid.Data4[i] = (guid.Data4[i] << 4) | HexCharacterToIntValue(*szText);
      ++szText;
    }

    ezUuid result;
    ezMemoryUtils::Copy<ezUuid>(&result, reinterpret_cast<ezUuid*>(&guid), 1);

    return result;
  }

#define Check(name)                                  \
  if (sColorName.IsEqual_NoCase(EZ_STRINGIZE(name))) \
  return ezColor::name

  ezColor GetColorByName(ezStringView sColorName, bool* out_pValidColorName)
  {
    if (out_pValidColorName)
      *out_pValidColorName = false;

    if (sColorName.IsEmpty())
      return ezColor::Black; // considered not to be a valid color name

    const ezUInt32 uiLen = sColorName.GetElementCount();

    auto twoCharsToByte = [](const char* szColorChars, ezUInt8& out_uiByte) -> ezResult
    {
      ezInt8 firstChar = HexCharacterToIntValue(szColorChars[0]);
      ezInt8 secondChar = HexCharacterToIntValue(szColorChars[1]);
      if (firstChar < 0 || secondChar < 0)
      {
        return EZ_FAILURE;
      }
      out_uiByte = (static_cast<ezUInt8>(firstChar) << 4) | static_cast<ezUInt8>(secondChar);
      return EZ_SUCCESS;
    };

    if (sColorName.StartsWith("#"))
    {
      if (uiLen == 7 || uiLen == 9) // #RRGGBB or #RRGGBBAA
      {
        ezUInt8 cv[4] = {0, 0, 0, 255};

        const char* szColorName = sColorName.GetStartPointer();

        if (twoCharsToByte(szColorName + 1, cv[0]).Failed())
          return ezColor::Black;
        if (twoCharsToByte(szColorName + 3, cv[1]).Failed())
          return ezColor::Black;
        if (twoCharsToByte(szColorName + 5, cv[2]).Failed())
          return ezColor::Black;

        if (uiLen == 9)
        {
          if (twoCharsToByte(szColorName + 7, cv[3]).Failed())
            return ezColor::Black;
        }

        if (out_pValidColorName)
          *out_pValidColorName = true;

        return ezColorGammaUB(cv[0], cv[1], cv[2], cv[3]);
      }

      // else RebeccaPurple !
    }
    else
    {
      if (out_pValidColorName)
        *out_pValidColorName = true;

      Check(AliceBlue);
      Check(AntiqueWhite);
      Check(Aqua);
      Check(Aquamarine);
      Check(Azure);
      Check(Beige);
      Check(Bisque);
      Check(Black);
      Check(BlanchedAlmond);
      Check(Blue);
      Check(BlueViolet);
      Check(Brown);
      Check(BurlyWood);
      Check(CadetBlue);
      Check(Chartreuse);
      Check(Chocolate);
      Check(Coral);
      Check(CornflowerBlue); // The Original!
      Check(Cornsilk);
      Check(Crimson);
      Check(Cyan);
      Check(DarkBlue);
      Check(DarkCyan);
      Check(DarkGoldenRod);
      Check(DarkGray);
      Check(DarkGrey);
      Check(DarkGreen);
      Check(DarkKhaki);
      Check(DarkMagenta);
      Check(DarkOliveGreen);
      Check(DarkOrange);
      Check(DarkOrchid);
      Check(DarkRed);
      Check(DarkSalmon);
      Check(DarkSeaGreen);
      Check(DarkSlateBlue);
      Check(DarkSlateGray);
      Check(DarkSlateGrey);
      Check(DarkTurquoise);
      Check(DarkViolet);
      Check(DeepPink);
      Check(DeepSkyBlue);
      Check(DimGray);
      Check(DimGrey);
      Check(DodgerBlue);
      Check(FireBrick);
      Check(FloralWhite);
      Check(ForestGreen);
      Check(Fuchsia);
      Check(Gainsboro);
      Check(GhostWhite);
      Check(Gold);
      Check(GoldenRod);
      Check(Gray);
      Check(Grey);
      Check(Green);
      Check(GreenYellow);
      Check(HoneyDew);
      Check(HotPink);
      Check(IndianRed);
      Check(Indigo);
      Check(Ivory);
      Check(Khaki);
      Check(Lavender);
      Check(LavenderBlush);
      Check(LawnGreen);
      Check(LemonChiffon);
      Check(LightBlue);
      Check(LightCoral);
      Check(LightCyan);
      Check(LightGoldenRodYellow);
      Check(LightGray);
      Check(LightGrey);
      Check(LightGreen);
      Check(LightPink);
      Check(LightSalmon);
      Check(LightSeaGreen);
      Check(LightSkyBlue);
      Check(LightSlateGray);
      Check(LightSlateGrey);
      Check(LightSteelBlue);
      Check(LightYellow);
      Check(Lime);
      Check(LimeGreen);
      Check(Linen);
      Check(Magenta);
      Check(Maroon);
      Check(MediumAquaMarine);
      Check(MediumBlue);
      Check(MediumOrchid);
      Check(MediumPurple);
      Check(MediumSeaGreen);
      Check(MediumSlateBlue);
      Check(MediumSpringGreen);
      Check(MediumTurquoise);
      Check(MediumVioletRed);
      Check(MidnightBlue);
      Check(MintCream);
      Check(MistyRose);
      Check(Moccasin);
      Check(NavajoWhite);
      Check(Navy);
      Check(OldLace);
      Check(Olive);
      Check(OliveDrab);
      Check(Orange);
      Check(OrangeRed);
      Check(Orchid);
      Check(PaleGoldenRod);
      Check(PaleGreen);
      Check(PaleTurquoise);
      Check(PaleVioletRed);
      Check(PapayaWhip);
      Check(PeachPuff);
      Check(Peru);
      Check(Pink);
      Check(Plum);
      Check(PowderBlue);
      Check(Purple);
      Check(RebeccaPurple);
      Check(Red);
      Check(RosyBrown);
      Check(RoyalBlue);
      Check(SaddleBrown);
      Check(Salmon);
      Check(SandyBrown);
      Check(SeaGreen);
      Check(SeaShell);
      Check(Sienna);
      Check(Silver);
      Check(SkyBlue);
      Check(SlateBlue);
      Check(SlateGray);
      Check(SlateGrey);
      Check(Snow);
      Check(SpringGreen);
      Check(SteelBlue);
      Check(Tan);
      Check(Teal);
      Check(Thistle);
      Check(Tomato);
      Check(Turquoise);
      Check(Violet);
      Check(Wheat);
      Check(White);
      Check(WhiteSmoke);
      Check(Yellow);
      Check(YellowGreen);
    }

    if (out_pValidColorName)
      *out_pValidColorName = false;

    return ezColor::RebeccaPurple;
  }

#undef Check

#define Check(name)         \
  if (ezColor::name == col) \
  return #name

  ezString GetColorName(const ezColor& col)
  {
    Check(AliceBlue);
    Check(AntiqueWhite);
    Check(Aqua);
    Check(Aquamarine);
    Check(Azure);
    Check(Beige);
    Check(Bisque);
    Check(Black);
    Check(BlanchedAlmond);
    Check(Blue);
    Check(BlueViolet);
    Check(Brown);
    Check(BurlyWood);
    Check(CadetBlue);
    Check(Chartreuse);
    Check(Chocolate);
    Check(Coral);
    Check(CornflowerBlue); // The Original!
    Check(Cornsilk);
    Check(Crimson);
    Check(Cyan);
    Check(DarkBlue);
    Check(DarkCyan);
    Check(DarkGoldenRod);
    Check(DarkGray);
    Check(DarkGreen);
    Check(DarkKhaki);
    Check(DarkMagenta);
    Check(DarkOliveGreen);
    Check(DarkOrange);
    Check(DarkOrchid);
    Check(DarkRed);
    Check(DarkSalmon);
    Check(DarkSeaGreen);
    Check(DarkSlateBlue);
    Check(DarkSlateGray);
    Check(DarkTurquoise);
    Check(DarkViolet);
    Check(DeepPink);
    Check(DeepSkyBlue);
    Check(DimGray);
    Check(DodgerBlue);
    Check(FireBrick);
    Check(FloralWhite);
    Check(ForestGreen);
    Check(Fuchsia);
    Check(Gainsboro);
    Check(GhostWhite);
    Check(Gold);
    Check(GoldenRod);
    Check(Gray);
    Check(Green);
    Check(GreenYellow);
    Check(HoneyDew);
    Check(HotPink);
    Check(IndianRed);
    Check(Indigo);
    Check(Ivory);
    Check(Khaki);
    Check(Lavender);
    Check(LavenderBlush);
    Check(LawnGreen);
    Check(LemonChiffon);
    Check(LightBlue);
    Check(LightCoral);
    Check(LightCyan);
    Check(LightGoldenRodYellow);
    Check(LightGray);
    Check(LightGreen);
    Check(LightPink);
    Check(LightSalmon);
    Check(LightSeaGreen);
    Check(LightSkyBlue);
    Check(LightSlateGray);
    Check(LightSteelBlue);
    Check(LightYellow);
    Check(Lime);
    Check(LimeGreen);
    Check(Linen);
    Check(Magenta);
    Check(Maroon);
    Check(MediumAquaMarine);
    Check(MediumBlue);
    Check(MediumOrchid);
    Check(MediumPurple);
    Check(MediumSeaGreen);
    Check(MediumSlateBlue);
    Check(MediumSpringGreen);
    Check(MediumTurquoise);
    Check(MediumVioletRed);
    Check(MidnightBlue);
    Check(MintCream);
    Check(MistyRose);
    Check(Moccasin);
    Check(NavajoWhite);
    Check(Navy);
    Check(OldLace);
    Check(Olive);
    Check(OliveDrab);
    Check(Orange);
    Check(OrangeRed);
    Check(Orchid);
    Check(PaleGoldenRod);
    Check(PaleGreen);
    Check(PaleTurquoise);
    Check(PaleVioletRed);
    Check(PapayaWhip);
    Check(PeachPuff);
    Check(Peru);
    Check(Pink);
    Check(Plum);
    Check(PowderBlue);
    Check(Purple);
    Check(RebeccaPurple);
    Check(Red);
    Check(RosyBrown);
    Check(RoyalBlue);
    Check(SaddleBrown);
    Check(Salmon);
    Check(SandyBrown);
    Check(SeaGreen);
    Check(SeaShell);
    Check(Sienna);
    Check(Silver);
    Check(SkyBlue);
    Check(SlateBlue);
    Check(SlateGray);
    Check(Snow);
    Check(SpringGreen);
    Check(SteelBlue);
    Check(Tan);
    Check(Teal);
    Check(Thistle);
    Check(Tomato);
    Check(Turquoise);
    Check(Violet);
    Check(Wheat);
    Check(White);
    Check(WhiteSmoke);
    Check(Yellow);
    Check(YellowGreen);

    ezColorGammaUB cg = col;

    ezStringBuilder s;

    if (cg.a == 255)
    {
      s.Format("#{0}{1}{2}", ezArgU(cg.r, 2, true, 16, true), ezArgU(cg.g, 2, true, 16, true), ezArgU(cg.b, 2, true, 16, true));
    }
    else
    {
      s.Format("#{0}{1}{2}{3}", ezArgU(cg.r, 2, true, 16, true), ezArgU(cg.g, 2, true, 16, true), ezArgU(cg.b, 2, true, 16, true),
        ezArgU(cg.a, 2, true, 16, true));
    }

    return s;
  }

#undef Check

} // namespace ezConversionUtils


EZ_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_ConversionUtils);
