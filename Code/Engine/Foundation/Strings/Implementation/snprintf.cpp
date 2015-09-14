#include <Foundation/PCH.h>
#include <Foundation/Strings/StringUtils.h>
#include <stdarg.h>

// This is an implementation of the sprintf function, with an additional buffer size
// On some systems this is implemented under the name 'snprintf'
// It tries to be as true to the specification as possible, where possible,
// though there are some ambiguities in the specification, which other implementations
// also interpret differently
// Spec taken from here: http://www.cplusplus.com/reference/clibrary/cstdio/sprintf/

// So far there is no support for wide character strings (and chars)
// There is no support for locales, the floating point dot is always a '.' even on layouts 
// that would otherwise use a ',' (e.g. German)

// Not Implemented:
//  Wide Character support for %s 
//  Everything about this function works with standard ASCII characters,
//  what sense does it make to embed a wide-character string into it? Shouldn't the formatting string then
//  also be wide-character ?
//
//  Length Specifier (h/l/L) for d/i/u/x/X/o
//  I don't get it what these things are useful for. Everything is promoted to int / double.
//  Small values (bytes / shorts) don't get any larger by this, I don't see anything that could be done
//  differently knowing these values are supposed to be bytes or shorts.

//#define USE_STRICT_SPECIFICATION

struct sprintfFlags
{
  enum Enum
  {
    None,
    LeftJustify   = EZ_BIT(0),  // -
    ForceSign     = EZ_BIT(1),  // +
    BlankSign     = EZ_BIT(2),  // (space)
    Hash          = EZ_BIT(3),  // #
    PadZeros      = EZ_BIT(4),  // 0
    ForceZeroSign = EZ_BIT(5),  // [internal] Prints a '+' even for zero
  };
};

struct sprintfLength
{
  enum Enum
  {
    Default,    // int, double, char, char*
    ShortInt,   // short
    LongInt,    // long int (?)
    LongDouble, // long double (?)
    LongLongInt,// long long int (64 Bit)
  };
};

// Reads all the 'Flags' from the formatting string
static unsigned int ReadFlags (const char* szFormat, unsigned int& uiReadPos, char& cNext)
{
  unsigned int Flags = sprintfFlags::None;
  bool bContinue = true;

  while (bContinue)
  {
    bContinue = false;

    switch (cNext)
    {
    case '-':
      Flags |= sprintfFlags::LeftJustify;
      bContinue = true;
      break;
    case '+':
      Flags |= sprintfFlags::ForceSign;
      bContinue = true;
      break;
    case ' ':
      Flags |= sprintfFlags::BlankSign;
      bContinue = true;
      break;
    case '#':
      Flags |= sprintfFlags::Hash;
      bContinue = true;
      break;
    case '0':
      Flags |= sprintfFlags::PadZeros;
      bContinue = true;
      break;
    }

    if (bContinue)
      cNext = szFormat[++uiReadPos];
  }

  return Flags;
}

// Reads the 'width' parameter from the formatting string
static int ReadWidth (const char* szFormat, unsigned int& uiReadPos, char& cNext)
{
  int iWidth = 0;

  if (cNext == '*')
  {
    iWidth = -1;
    cNext = szFormat[++uiReadPos];
    return iWidth;
  }

  if ((cNext >= '1') && (cNext <= '9')) // do not allow widths of zero, because that would collide with the flags specifier
  {
    iWidth = cNext - '0';
    cNext = szFormat[++uiReadPos];

    while ((cNext >= '0') && (cNext <= '9')) // also breaks upon '\0', so this is safe
    {
      iWidth *= 10;
      iWidth += cNext - '0';
      cNext = szFormat[++uiReadPos];
    }
  }

  return iWidth;
}

// Reads the 'precision' parameter from the formatting string.
static int ReadPrecision (const char* szFormat, unsigned int& uiReadPos, char& cNext)
{
  if (cNext != '.')
    return -1; // default if no precision is specified

  cNext = szFormat[++uiReadPos];

  if (cNext == '*')
  {
    cNext = szFormat[++uiReadPos];
    return -2;
  }

  // the default if no precision is specified, but the '.' is present
  int iPrecision = 0;

  while ((cNext >= '0') && (cNext <= '9')) // also breaks upon '\0', so this is safe
  {
    iPrecision *= 10;
    iPrecision += cNext - '0';
    cNext = szFormat[++uiReadPos];
  }

  return iPrecision;
}

// Reads the 'length' parameter from the formatting string
static sprintfLength::Enum ReadLength (const char* szFormat, unsigned int& uiReadPos, char& cNext)
{
  sprintfLength::Enum res = sprintfLength::Default;

  switch (cNext)
  {
  case 'h':
    res = sprintfLength::ShortInt;
    cNext = szFormat[++uiReadPos];
    break;
  case 'l':
    res = sprintfLength::LongInt;
    cNext = szFormat[++uiReadPos];

    if (cNext == 'l')
    {
      res = sprintfLength::LongLongInt;
      cNext = szFormat[++uiReadPos];
    }
    break;
  case 'L':
    res = sprintfLength::LongDouble;
    cNext = szFormat[++uiReadPos];
    break;
  }

  return res;
}

// Reads the 'specifier' parameter from the formatting string.
static char ReadSpecifier (const char* szFormat, unsigned int& uiReadPos, char& cNext, bool& bError)
{
  const char cRet = cNext;

  switch (cNext)
  {
  case 'c':
  case 'd':
  case 'i':
  case 'e':
  case 'E':
  case 'f':
  case 'g':
  case 'G':
  case 'o':
  case 's':
  case 'u':
  case 'x':
  case 'X':
  case 'p':
  case 'n':
  case '%':

  // 'b' for 'binary' does not exist in the original specification
  // However, for debugging, it is sometime quite useful, so I added it
  case 'b': 
    cNext = szFormat[++uiReadPos];
    break;

  default:
    bError = true;
    break;
  }
    
  return cRet;
}

static void OutputChar (char* szOutputBuffer, unsigned int uiBufferSize, unsigned int& uiWritePos, char c)
{
  if ((szOutputBuffer) && (uiWritePos + 1 < uiBufferSize))
    szOutputBuffer[uiWritePos] = c;

  // always increase the write position to know how much would have been written
  ++uiWritePos;
}

static void OutputPadding (char* szOutputBuffer, unsigned int uiBufferSize, unsigned int& uiWritePos, int iPadding, char cPad)
{
  for (int i = 0; i < iPadding; ++i)
    OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, cPad);
}

static void OutputPaddingByFlags (char* szOutputBuffer, unsigned int uiBufferSize, unsigned int& uiWritePos, int iPadding, unsigned int Flags)
{
  if (Flags & sprintfFlags::PadZeros)
    OutputPadding (szOutputBuffer, uiBufferSize, uiWritePos, iPadding, '0');
  else
    OutputPadding (szOutputBuffer, uiBufferSize, uiWritePos, iPadding, ' ');
}

static void OutputNullPtr (char* szOutputBuffer, unsigned int uiBufferSize, unsigned int& uiWritePos)
{
  OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, '(');
  OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, 'n');
  OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, 'u');
  OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, 'l');
  OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, 'l');
  OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, ')');
}

static void OutputNaN (char* szOutputBuffer, unsigned int uiBufferSize, unsigned int& uiWritePos)
{
  OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, 'N');
  OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, 'a');
  OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, 'N');
}

static void OutputInf (char* szOutputBuffer, unsigned int uiBufferSize, unsigned int& uiWritePos)
{
  OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, 'I');
  OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, 'n');
  OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, 'f');
  OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, 'i');
  OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, 'n');
  OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, 'i');
  OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, 't');
  OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, 'y');
}


static void OutputString (char* szOutputBuffer, unsigned int uiBufferSize, unsigned int& uiWritePos, const char* szString, unsigned int Flags, int iMinSize, int iMaxSize)
{
  if (!szString)
  {
    OutputNullPtr (szOutputBuffer, uiBufferSize, uiWritePos);
    return;
  }

  int iLen = 0;

  // if the string is supposed to be right-justified
  if (((Flags & sprintfFlags::LeftJustify) == 0) && (iMinSize > 0))
  {
    // get the string length
    while (szString[iLen] != '\0')
      ++iLen;

    if ((iMaxSize >= 0) && (iLen > iMaxSize))
      iLen = iMaxSize;

    // output as much padding as necessary
    OutputPaddingByFlags (szOutputBuffer, uiBufferSize, uiWritePos, iMinSize - iLen, Flags);
  }

  // a negative max size will have no effect (disabled)
  iLen = 0;
  while ((szString[iLen] != '\0') && (iMaxSize != 0))
  {
    OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, szString[iLen]);
    --iMaxSize;
    ++iLen;
  }

  // if the text is supposed to be left justified, add required padding
  if (((Flags & sprintfFlags::LeftJustify) != 0) && (iMinSize > 0))
    OutputPadding (szOutputBuffer, uiBufferSize, uiWritePos, iMinSize - iLen, ' ');
}

static void OutputReverseString (char* szOutputBuffer, unsigned int uiBufferSize, unsigned int& uiWritePos, const char* szString, int iStringLength)
{
  while (iStringLength > 0)
  {
    --iStringLength;

    OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, szString[iStringLength]);
  }
}

static void OutputIntSign (char* szOutputBuffer, unsigned int uiBufferSize, unsigned int& uiWritePos, long long int value, unsigned int Flags, int iBase, bool bUpperCase, int iPrecision)
{
  if (value < 0)
    OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, '-');
  else
  if (Flags & sprintfFlags::ForceSign)
  {
    if (value > 0)
      OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, '+');
    else
      OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, ' ');
  }
  else
  if (Flags & sprintfFlags::BlankSign)
    OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, ' ');
  else
  if (Flags & sprintfFlags::ForceZeroSign)
    OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, '+');

  if (Flags & sprintfFlags::Hash)
  {
    // With precision set to 'zero', nothing shall be printed if the value is zero
    if ((value == 0) && (iPrecision == 0))
      return;

    // in octal mode, print a '0' in front of it
    if (iBase == 8)
      OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, '0');

    // in hexadecimal mode, print a '0x' or '0X' in front of it
    if (iBase == 16)
    {
      OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, '0');

      if (bUpperCase)
        OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, 'X');
      else
        OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, 'x');
    }
  }
}

static void OutputIntSignAndPadding (char* szOutputBuffer, unsigned int uiBufferSize, unsigned int& uiWritePos, long long int value, unsigned int Flags, int iPadding, int iBase, bool bUpperCase, int iPrecision)
{
  if (Flags & sprintfFlags::PadZeros)
  {
    // if we pad with zeros, FIRST write the sign
    OutputIntSign (szOutputBuffer, uiBufferSize, uiWritePos, value, Flags, iBase, bUpperCase, iPrecision);
    OutputPadding (szOutputBuffer, uiBufferSize, uiWritePos, iPadding, '0');
  }
  else
  {
    // if we pad with spaces, pad first, THEN write the sign
    OutputPadding (szOutputBuffer, uiBufferSize, uiWritePos, iPadding, ' ');
    OutputIntSign (szOutputBuffer, uiBufferSize, uiWritePos, value, Flags, iBase, bUpperCase, iPrecision);
  }
}

static void FormatUInt (char* szOutputBuffer, int& iNumDigits, unsigned long long int uiValue, unsigned int uiBase, bool bUpperCase, int iPrecision)
{
  // this function will write the number in reverse order to the string buffer

  iNumDigits = 0;

  // if no precision is given, we store this as '-1'
  // this means that at least one digit should be written, thus change it to 1 (default by the spec)
  if (iPrecision == -1)
    iPrecision = 1;

  // if value is zero and precision is zero, do not write anything
  if ((iPrecision == 0) && (uiValue == 0))
  {
    szOutputBuffer[0] = '\0';
    return;
  }

  // make sure we do not write too much data into the temporary buffer
  if (iPrecision > 64)
    iPrecision = 64;

  while (uiValue > 0)
  {
    const unsigned int digit = uiValue % uiBase;

    if (digit <= 9)
      szOutputBuffer[iNumDigits] = '0' + digit;
    else
    if (digit <= 15)
    {
      if (bUpperCase)
        szOutputBuffer[iNumDigits] = 'A' + (digit - 10);
      else
        szOutputBuffer[iNumDigits] = 'a' + (digit - 10);
    }

    uiValue /= uiBase;
    ++iNumDigits;
  }

  // if too few digits were written, pad the result with zero (precision modifier)
  while (iNumDigits < iPrecision)
  {
    szOutputBuffer[iNumDigits] = '0';
    ++iNumDigits;
  }

  // write the final zero terminator
  szOutputBuffer[iNumDigits] = '\0';
}

static int GetSignSize (long long int value, unsigned int Flags, int iBase, int iPrecision)
{
  int iSize = 0;

  if (value < 0)
    iSize = 1;

  if (Flags & (sprintfFlags::ForceSign | sprintfFlags::BlankSign))
    iSize = 1;

  if ((Flags & sprintfFlags::Hash) && ((value != 0) || (iPrecision != 0)))
  {
    if (iBase == 8)
      iSize += 1;
    if (iBase == 16)
      iSize += 2;
  }

  return iSize;
}

static void OutputInt (char* szOutputBuffer, unsigned int uiBufferSize, unsigned int& uiWritePos, long long int value, int iWidth, int iPrecision, unsigned int Flags, int iBase)
{
  // for a 32 Bit Integer one needs at most 10 digits
  // for a 64 Bit Integer one needs at most 20 digits
  // However, FormatUInt allows to add extra padding, so make the buffer a bit larger
  char s[128];
  int iNumDigits = 0;

  unsigned long long int absval = value < 0 ? -value : value;

  FormatUInt (s, iNumDigits, absval, iBase, false, iPrecision);

  int iNumberWidth = GetSignSize (value, Flags, iBase, iPrecision) + iNumDigits;

  if (Flags & sprintfFlags::LeftJustify)
  {
    OutputIntSign (szOutputBuffer, uiBufferSize, uiWritePos, value, Flags, iBase, false, iPrecision);
    OutputReverseString (szOutputBuffer, uiBufferSize, uiWritePos, s, iNumDigits);
    OutputPadding (szOutputBuffer, uiBufferSize, uiWritePos, iWidth - iNumberWidth, ' ');
  }
  else
  {
    OutputIntSignAndPadding (szOutputBuffer, uiBufferSize, uiWritePos, value, Flags, iWidth - iNumberWidth, iBase, false, iPrecision);
    OutputReverseString (szOutputBuffer, uiBufferSize, uiWritePos, s, iNumDigits);
  }    
}

static void OutputUInt (char* szOutputBuffer, unsigned int uiBufferSize, unsigned int& uiWritePos, unsigned long long int value, int iWidth, int iPrecision, unsigned int Flags, int iBase, bool bUpperCase)
{
  #ifndef USE_STRICT_SPECIFICATION
    // In the non-strict implementation unsigned values will never be preceded by signs (+ or space)
    Flags &= ~ (sprintfFlags::BlankSign | sprintfFlags::ForceSign);
  #endif

  // for a 32 Bit Integer one needs at most 10 digits
  // for a 64 Bit Integer one needs at most 20 digits
  // However, FormatUInt allows to add extra padding, so make the buffer a bit larger
  char s[128];
  int iNumDigits = 0;

  FormatUInt (s, iNumDigits, value, iBase, bUpperCase, iPrecision);

  // since the uint is casted to an int, we must make sure very large values are not mapped to negative ints
  // therefore do the ' > 0 ? 1 : 0 ' test to just pass on the proper sign, instead of casting it to an int
  int iNumberWidth = GetSignSize (value > 0 ? 1 : 0, Flags, iBase, iPrecision) + iNumDigits;

  if (Flags & sprintfFlags::LeftJustify)
  {
    OutputIntSign (szOutputBuffer, uiBufferSize, uiWritePos, value > 0 ? 1 : 0, Flags, iBase, bUpperCase, iPrecision);
    OutputReverseString (szOutputBuffer, uiBufferSize, uiWritePos, s, iNumDigits);
    OutputPadding (szOutputBuffer, uiBufferSize, uiWritePos, iWidth - iNumberWidth, ' ');
  }
  else
  {
    OutputIntSignAndPadding (szOutputBuffer, uiBufferSize, uiWritePos, value > 0 ? 1 : 0, Flags, iWidth - iNumberWidth, iBase, bUpperCase, iPrecision);
    OutputReverseString (szOutputBuffer, uiBufferSize, uiWritePos, s, iNumDigits);
  }    
}

static bool RoundUpDigits (char* szBuffer, int iLastDigits)
{
  for (int i = iLastDigits - 1; i >= 0; --i)
  {
    szBuffer[i] = szBuffer[i] + 1;

    if (szBuffer[i] == '9' + 1)
      szBuffer[i] = '0';
    else
      return false;
  }

  return true;
}

static void RemoveTrailingZeros (char* szBuffer, int& iWritePos)
{
  const int iEnd = iWritePos;
  for (int i = iEnd - 1; i >= 0; --i)
  {
    if (szBuffer[i] == '0')
    {
      szBuffer[i] = '\0';
      --iWritePos;
    }
    else
      break;
  }
}

union Int64DoubleUnion
{
  unsigned long long int i;
  double f;
};

static bool IsFinite(double value)
{
  // Check the 11 exponent bits.
  // NAN -> (exponent = all 1, mantissa = non-zero)
  // INF -> (exponent = all 1, mantissa = zero)

  Int64DoubleUnion i2f;
  i2f.f = value;
  return ((i2f.i & 0x7FF0000000000000LL) != 0x7FF0000000000000LL);
}

static bool IsNaN(double value)
{
  // Check the 11 exponent bits.
  // NAN -> (exponent = all 1, mantissa = non-zero)
  // INF -> (exponent = all 1, mantissa = zero)

  Int64DoubleUnion i2f;
  i2f.f = value;

  return (((i2f.i & 0x7FF0000000000000LL) == 0x7FF0000000000000LL) && ((i2f.i & 0xFFFFFFFFFFFFFLL) != 0));
}

static bool FormatUFloat (char* szOutputBuffer, unsigned int uiBufferSize, unsigned int& uiWritePos, double& value0, int iPrecision, unsigned int Flags, bool bRemoveZeroes)
{
  if (IsNaN (value0))
  {
    value0 = 0;
    OutputNaN (szOutputBuffer, uiBufferSize, uiWritePos);
    return false;
  }
  else if (!IsFinite (value0))
  {
    OutputInf (szOutputBuffer, uiBufferSize, uiWritePos);
    return false;
  }

  double value = value0;

  if (value < 0)
    value = - value;

  bool bRoundedUp = false;

  long long int uiIntPart = (long long int) value;

  char szFraction[128];
  int iFractionDigits = 0;

  double dRemainder = value - (double) uiIntPart;

  // make sure we don't write outside our buffer
  if (iPrecision > 64)
    iPrecision = 64;

  // When no precision is given a maximum of 6 fractional digits is written
  // However, all trailing zeros will be removed again, to shorten the number as much as possible
  const bool bRemoveTrailingZeros = bRemoveZeroes || (iPrecision < 0);
    
  if (iPrecision < 0)
    iPrecision = 6;

  for (int i = 0; i < iPrecision; ++i)
  {
    dRemainder *= 10.0f;
    long long int digit = (long long int) dRemainder;

    char cDigit = '0' + (char) (digit);
    szFraction[iFractionDigits++] = cDigit;

    dRemainder -= (double) digit;
  }

  // zero terminate the string
  szFraction[iFractionDigits] = '\0';

  // if the NEXT (not written digit) is 5 or larger, round the whole number up
  if (dRemainder >= 0.5)
  {
    if (RoundUpDigits (szFraction, iFractionDigits))
    {
      bRoundedUp = true;
      ++uiIntPart;
    }
  }

  if (bRemoveTrailingZeros)
    RemoveTrailingZeros (szFraction, iFractionDigits);


  {
    char szBuffer[128];
    int iNumDigits = 0;

    // for a 32 Bit Integer one needs at most 10 digits
    // for a 64 Bit Integer one needs at most 20 digits
    // However, FormatUInt allows to add extra padding, so make the buffer a bit larger
    FormatUInt (szBuffer, iNumDigits, uiIntPart, 10, false, 1);

    OutputReverseString (szOutputBuffer, uiBufferSize, uiWritePos, szBuffer, iNumDigits);
  }

  // if there is any fractional digit, or the user specifically requested it,
  // add the '.'
  if ((iFractionDigits > 0) || (Flags & sprintfFlags::Hash))
    OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, '.');
    
  OutputString (szOutputBuffer, uiBufferSize, uiWritePos, szFraction, 0, 0, -1);

  return bRoundedUp;
}
static bool WouldRoundToTen (double value, int iPrecision)
{
  if (value == 0.0)
    return false;

  // get the absolute value
  value = value < 0.0 ? -value : value;

  // change the number until it is at least n.xxx with n > 0
  while (value < 1.0)
    value *= 10.0;

  // now remove the whole part
  // the number will only be rounded up to the next digit, if all digits are '9'
  // otherwise some digit will just increase by one but no continuous overflow will happen
  {
    long long int iWhole = (long long int) value;

    // remove the whole part
    value -= (double) iWhole;

    // get the next digit
    // if it is not '9', no overflow will occur
    while (iWhole > 0)
    {
      int iDigit = iWhole % 10;

      if (iDigit < 9)
        return false;

      iWhole /= 10;
    }
  }

  // now check iPrecision digits of the fractional part, whether they are all '9'
  for (int i = 0; i < iPrecision; ++i)
  {
    value *= 10.0;

    int iDigit = (int) value;

    if (iDigit != 9)
      return false;

    value -= (double) iDigit;
  }

  // finally check the next digit (which would not be printed anymore)
  value *= 10.0;
    
  int iDigit = (int) value;

  // if it is >= 5, the number will be rounded up, thus triggering a cascade of overflows (everything else is '9')
  if (iDigit < 5)
    return false;

  return true;
}


static void FormatUFloatScientific (char* szOutputBuffer, unsigned int uiBufferSize, unsigned int& uiWritePos, double& value0, int iPrecision, unsigned int Flags, bool bUpperCase, bool bRemoveZeroes)
{
  if (IsNaN (value0))
  {
    value0 = 0;
    OutputNaN (szOutputBuffer, uiBufferSize, uiWritePos);
    return;
  }
  else if (!IsFinite (value0))
  {
    OutputInf (szOutputBuffer, uiBufferSize, uiWritePos);
    return;
  }

  double value = value0;

  double dSci = value > 0.0 ? value : -value;
  int exp = 0;

  if (dSci != 0.0)
  {
    while (dSci >= 10.0)
    {
      dSci /= 10.0;
      exp++;
    }

    while (dSci < 1.0)
    {
      dSci *= 10.0;
      exp--;
    }
  }

  if (WouldRoundToTen (value, iPrecision))
  {
    dSci /= 10.0;
    exp++;
  }

  if (FormatUFloat (szOutputBuffer, uiBufferSize, uiWritePos, dSci, iPrecision, Flags, bRemoveZeroes))
  {
  }

  OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, bUpperCase ? 'E' : 'e');
  OutputInt (szOutputBuffer, uiBufferSize, uiWritePos, exp, 0, 3, sprintfFlags::ForceZeroSign, 10);
}

static void OutputFloat (char* szOutputBuffer, unsigned int uiBufferSize, unsigned int& uiWritePos, double value, int iWidth, int iPrecision, unsigned int Flags, bool bUpperCase, bool bScientific, bool bRemoveZeroes)
{
  char szBuffer[128];
  unsigned int iNumDigits = 0;

  // Input values that are outside the int64 range cannot be output in non-scientific form
  if (bScientific || value >= 9223372036854775807.0 || value <= -9223372036854775808.0)
    FormatUFloatScientific (szBuffer, 128, iNumDigits, value, iPrecision, Flags, bUpperCase, bRemoveZeroes);
  else
    FormatUFloat (szBuffer, 128, iNumDigits, value, iPrecision, Flags, bRemoveZeroes);
    
  szBuffer[iNumDigits] = '\0';

  int iNumberWidth = GetSignSize ((long long int) value, Flags, 10, iPrecision) + iNumDigits;

  // when right justifying, first output the padding
  if ((Flags & sprintfFlags::LeftJustify) == 0)
    OutputIntSignAndPadding (szOutputBuffer, uiBufferSize, uiWritePos, value < 0 ? -1 : 1, Flags, iWidth - iNumberWidth, 10, false, iPrecision);
  else
    OutputIntSign (szOutputBuffer, uiBufferSize, uiWritePos, value < 0 ? -1 : 1, Flags, 10, false, 1);

  OutputString (szOutputBuffer, uiBufferSize, uiWritePos, szBuffer, Flags, 0, -1);

  // when left justifying, first output the string, then output the padding
  if ((Flags & sprintfFlags::LeftJustify) != 0)
    OutputPadding (szOutputBuffer, uiBufferSize, uiWritePos, iWidth - iNumberWidth, ' ');
}

static void OutputFloat_Short (char* szOutputBuffer, unsigned int uiBufferSize, unsigned int& uiWritePos, double value, int iWidth, int iPrecision, unsigned int Flags, bool bUpperCase)
{
  //bool bScientific = false;

  //double absval = value < 0.0 ? -value : value;

  //if (absval >= 1000000)
  //  bScientific = true;
  //if (absval <= 0.00001)
  //  bScientific = true;

  //OutputFloat (szOutputBuffer, uiBufferSize, uiWritePos, value, iWidth, iPrecision, Flags, bUpperCase, bScientific, false);


  char szBuffer[128], szBuffer2[128];
  unsigned int iNumDigitsF = 0;
  unsigned int iNumDigitsE = 0;

  const int iPrecF = iPrecision < 0 ? 12 : iPrecision;
  const int iPrecE = iPrecision < 0 ? 5 : iPrecision;

  FormatUFloatScientific (szBuffer, 128, iNumDigitsE, value, iPrecE, Flags, bUpperCase, iPrecision < 0);
  FormatUFloat (szBuffer2, 128, iNumDigitsF, value, iPrecF, Flags, iPrecision < 0);

  bool bScientific = (iNumDigitsE < iNumDigitsF);

  if (value != 0.0)
  {
    bool bAllZero = true;
    for (int i = 0; i < (int) iNumDigitsF; ++i)
    {
      if ((szBuffer2[i] != '0') && (szBuffer2[i] != '.'))
      {
        bAllZero = false;
        break;
      }
    }

    if (bAllZero)
      bScientific = true;
  }

  // Input values that are outside the int64 range cannot be output in non-scientific form
  if (bScientific || value >= 9223372036854775807.0 || value <= -9223372036854775808.0)
    OutputFloat (szOutputBuffer, uiBufferSize, uiWritePos, value, iWidth, iPrecE, Flags, bUpperCase, bScientific, iPrecision < 0);
  else
    OutputFloat (szOutputBuffer, uiBufferSize, uiWritePos, value, iWidth, iPrecF, Flags, bUpperCase, bScientific, iPrecision < 0);
}

int ezStringUtils::vsnprintf(char* szOutputBuffer, unsigned int uiBufferSize, const char* szFormat, va_list args0)
{
  va_list args;
  va_copy(args, args0);

  EZ_ASSERT_DEV(ezUnicodeUtils::IsValidUtf8(szFormat), "The sprintf format string must be valid Utf8.");

  // make sure the last character is a \0
  if ((szOutputBuffer) && (uiBufferSize > 0))
    szOutputBuffer[uiBufferSize - 1] = '\0';

  unsigned int uiReadPos = 0;
  unsigned int uiWritePos = 0;
  bool bError = false;

  while (szFormat[uiReadPos] != '\0')
  {
    const char c = szFormat[uiReadPos];

    ++uiReadPos;

    // if c is not %, just print it out and be done with it
    if (c != '%')
    {
      OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, c);
      continue;
    }

    // otherwise parse the formatting string to find out what has to be done
    char cNext = szFormat[uiReadPos];

    // *** parse the format string ***

    // first read the flags (as many as there are)
    unsigned int Flags = ReadFlags (szFormat, uiReadPos, cNext);
    // read the width of the field
    int iWidth = ReadWidth (szFormat, uiReadPos, cNext);
    // read the precision specifier
    int iPrecision = ReadPrecision (szFormat, uiReadPos, cNext);
    // read the input data 'length' (short / int / long it, float / double)
    const sprintfLength::Enum Length = ReadLength (szFormat, uiReadPos, cNext);
    // read the 'specifier' type
    const char cSpecifier = ReadSpecifier (szFormat, uiReadPos, cNext, bError);

    if (bError)
    {
      snprintf (szOutputBuffer, uiBufferSize, "Error in formatting string at position %u ('%c').", uiReadPos, cSpecifier);
      va_end(args);
      return -1;
    }

    // if 'width' was specified as '*', read it from an extra parameter
    if (iWidth < 0)
      iWidth = va_arg (args, int);

    // if 'precision' was specified as '*', read it from an extra parameter
    if (iPrecision == -2)
      iPrecision = va_arg (args, int);

    // % Sign
    if (cSpecifier == '%')
    {
      OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, '%');
      continue;
    }

    // Nothing: Writes the current write position back to an int pointer
    if (cSpecifier == 'n')
    {
      int* p = va_arg (args, int*);
      if (p) 
        *p = (int) uiWritePos;
        
      continue;
    }

    // String
    if (cSpecifier == 's')
    {
      const char* s = va_arg (args, const char*);
      OutputString (szOutputBuffer, uiBufferSize, uiWritePos, s, Flags, iWidth, iPrecision);
      continue;
    }

    // Character
    if (cSpecifier == 'c')
    {
      char c2 = va_arg (args, int);
      char s[2] = {c2, 0};
      OutputString (szOutputBuffer, uiBufferSize, uiWritePos, s, Flags, iWidth, iPrecision);
      continue;
    }

    // Signed Decimal
    if ((cSpecifier == 'i') || (cSpecifier == 'd'))
    {
      long long int i = 0;
        
      if (Length == sprintfLength::LongLongInt)
        i = va_arg (args, long long int);
      else
      if (Length == sprintfLength::LongInt)
        i = va_arg (args, long int);
      else
        i = va_arg (args, int);

      OutputInt (szOutputBuffer, uiBufferSize, uiWritePos, i, iWidth, iPrecision, Flags, 10);
      continue;
    }

    // Unsigned Decimal, Octal, Hexadecimal or Binary
    if ((cSpecifier == 'u') ||(cSpecifier == 'o') || (cSpecifier == 'b') || (cSpecifier == 'x') || (cSpecifier == 'X'))
    {
      // the spec says that octal values are signed, but hexadecimal values are unsigned
      // I believe this is a typo in the specification, other implementations also always
      // assume unsigned values for octal numbers
      // therefore this implementation does this too

      unsigned long long int i = 0;
        
      if (Length == sprintfLength::LongLongInt)
        i = va_arg (args, unsigned long long int);
      else
      if (Length == sprintfLength::LongInt)
        i = va_arg (args, unsigned long int);
      else
        i = va_arg (args, unsigned int);

      int iBase = 10;

      switch (cSpecifier)
      {
      case 'o':
        iBase = 8;
        break;
      case 'b':
        iBase = 2;
        break;
      case 'x':
      case 'X':
        iBase = 16;
        break;
      }

      OutputUInt (szOutputBuffer, uiBufferSize, uiWritePos, i, iWidth, iPrecision, Flags, iBase, (cSpecifier == 'X'));
      continue;
    }

    // Float
    if ((cSpecifier == 'f') || (cSpecifier == 'e') || (cSpecifier == 'E'))
    {
      double f = va_arg (args, double);
      OutputFloat (szOutputBuffer, uiBufferSize, uiWritePos, f, iWidth, iPrecision, Flags, (cSpecifier == 'E'), (cSpecifier != 'f'), false);
      continue;
    }

    // Float, shorter Version either 'f' or 'e'
    if ((cSpecifier == 'g') || (cSpecifier == 'G'))
    {
      double f = va_arg (args, double);
      OutputFloat_Short (szOutputBuffer, uiBufferSize, uiWritePos, f, iWidth, iPrecision, Flags, (cSpecifier == 'G'));
      continue;
    }

    // pointer address
    if (cSpecifier == 'p')
    {
      iPrecision = (int) sizeof (void*) * 2;

      #ifndef USE_STRICT_SPECIFICATION
        // In the non-strict implementation, pointer addresses will not be preceded by additional signs, hex-indicators (0x..)
        // or padded with zeros
        Flags &= ~(sprintfFlags::BlankSign | sprintfFlags::ForceSign | sprintfFlags::PadZeros | sprintfFlags::Hash);
      #endif

      void* p = va_arg (args, void*);
      OutputUInt (szOutputBuffer, uiBufferSize, uiWritePos, (unsigned long long int) p, iWidth, iPrecision, Flags, 16, true);
      continue;
    }
  }

  // write the final \0
  OutputChar (szOutputBuffer, uiBufferSize, uiWritePos, '\0');

  va_end(args);

  // return the number of characters that would have been written
  // minus the terminating zero
  return uiWritePos - 1;
}

int ezStringUtils::snprintf (char* szOutputBuffer, unsigned int uiBufferSize, const char* szFormat, ...)
{
  va_list args;
  va_start (args, szFormat);

  int ret = vsnprintf (szOutputBuffer, uiBufferSize, szFormat, args);

  va_end (args);

  return ret;
}
  






EZ_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_snprintf);

