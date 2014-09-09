#include <PCH.h>
#include <Foundation/Utilities/ConversionUtils.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Utility);

EZ_CREATE_SIMPLE_TEST(Utility, ConversionUtils)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "StringToInt")
  {
    const char* szString = "1a";
    const char* szResultPos = nullptr;

    ezInt32 iRes = 42;
    szString = "01234";
    EZ_TEST_BOOL(ezConversionUtils::StringToInt(szString, iRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_INT(iRes, 1234);
    EZ_TEST_BOOL(szResultPos == szString + 5);

    iRes = 42;
    szString = "0";
    EZ_TEST_BOOL(ezConversionUtils::StringToInt(szString, iRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_INT(iRes, 0);
    EZ_TEST_BOOL(szResultPos == szString + ezStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "0000";
    EZ_TEST_BOOL(ezConversionUtils::StringToInt(szString, iRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_INT(iRes, 0);
    EZ_TEST_BOOL(szResultPos == szString + ezStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "-999999";
    EZ_TEST_BOOL(ezConversionUtils::StringToInt(szString, iRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_INT(iRes, -999999);
    EZ_TEST_BOOL(szResultPos == szString + ezStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "-+999999";
    EZ_TEST_BOOL(ezConversionUtils::StringToInt(szString, iRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_INT(iRes, -999999);
    EZ_TEST_BOOL(szResultPos == szString + ezStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "--999999";
    EZ_TEST_BOOL(ezConversionUtils::StringToInt(szString, iRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_INT(iRes, 999999);
    EZ_TEST_BOOL(szResultPos == szString + ezStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "++---+--+--999999";
    EZ_TEST_BOOL(ezConversionUtils::StringToInt(szString, iRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_INT(iRes, -999999);
    EZ_TEST_BOOL(szResultPos == szString + ezStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "++--+--+--999999";
    EZ_TEST_BOOL(ezConversionUtils::StringToInt(szString, iRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_INT(iRes, 999999);
    EZ_TEST_BOOL(szResultPos == szString + ezStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "123+456";
    EZ_TEST_BOOL(ezConversionUtils::StringToInt(szString, iRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_INT(iRes, 123);
    EZ_TEST_BOOL(szResultPos == szString + 3);

    iRes = 42;
    szString = "123_456";
    EZ_TEST_BOOL(ezConversionUtils::StringToInt(szString, iRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_INT(iRes, 123);
    EZ_TEST_BOOL(szResultPos == szString + 3);

    iRes = 42;
    szString = "-123-456";
    EZ_TEST_BOOL(ezConversionUtils::StringToInt(szString, iRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_INT(iRes, -123);
    EZ_TEST_BOOL(szResultPos == szString + 4);


    iRes = 42;
    EZ_TEST_BOOL(ezConversionUtils::StringToInt(nullptr, iRes) == EZ_FAILURE);
    EZ_TEST_INT(iRes, 42);

    iRes = 42;
    EZ_TEST_BOOL(ezConversionUtils::StringToInt("", iRes) == EZ_FAILURE);
    EZ_TEST_INT(iRes, 42);

    iRes = 42;
    EZ_TEST_BOOL(ezConversionUtils::StringToInt("a", iRes) == EZ_FAILURE);
    EZ_TEST_INT(iRes, 42);

    iRes = 42;
    EZ_TEST_BOOL(ezConversionUtils::StringToInt("a15", iRes) == EZ_FAILURE);
    EZ_TEST_INT(iRes, 42);

    iRes = 42;
    EZ_TEST_BOOL(ezConversionUtils::StringToInt("+", iRes) == EZ_FAILURE);
    EZ_TEST_INT(iRes, 42);

    iRes = 42;
    EZ_TEST_BOOL(ezConversionUtils::StringToInt("-", iRes) == EZ_FAILURE);
    EZ_TEST_INT(iRes, 42);

    iRes = 42;
    szString = "1a";
    EZ_TEST_BOOL(ezConversionUtils::StringToInt(szString, iRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_INT(iRes, 1);
    EZ_TEST_BOOL(szResultPos == szString + 1);

    iRes = 42;
    szString = "0 23";
    EZ_TEST_BOOL(ezConversionUtils::StringToInt(szString, iRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_INT(iRes, 0);
    EZ_TEST_BOOL(szResultPos == szString + 1);

    // overflow check

    iRes = 42;
    szString = "0002147483647"; // valid
    EZ_TEST_BOOL(ezConversionUtils::StringToInt(szString, iRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_INT(iRes, 2147483647);
    EZ_TEST_BOOL(szResultPos == szString + ezStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "-2147483648"; // valid
    EZ_TEST_BOOL(ezConversionUtils::StringToInt(szString, iRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_INT(iRes, (ezInt32)0x80000000);
    EZ_TEST_BOOL(szResultPos == szString + ezStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "0002147483648"; // invalid
    EZ_TEST_BOOL(ezConversionUtils::StringToInt(szString, iRes, &szResultPos) == EZ_FAILURE);
    EZ_TEST_INT(iRes, 42);

    iRes = 42;
    szString = "-2147483649"; // invalid
    EZ_TEST_BOOL(ezConversionUtils::StringToInt(szString, iRes, &szResultPos) == EZ_FAILURE);
    EZ_TEST_INT(iRes, 42);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "StringToInt64")
  {
    // overflow check
    ezInt64 iRes = 42;
    const char* szString = "0002147483639"; // valid
    const char* szResultPos = nullptr;

    EZ_TEST_BOOL(ezConversionUtils::StringToInt64(szString, iRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_INT(iRes, 2147483639);
    EZ_TEST_BOOL(szResultPos == szString + ezStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "0002147483640"; // also valid with 64bit
    EZ_TEST_BOOL(ezConversionUtils::StringToInt64(szString, iRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_INT(iRes, 2147483640);
    EZ_TEST_BOOL(szResultPos == szString + ezStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "0009223372036854775807"; // last valid positive number
    EZ_TEST_BOOL(ezConversionUtils::StringToInt64(szString, iRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_INT(iRes, 9223372036854775807);
    EZ_TEST_BOOL(szResultPos == szString + ezStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "0009223372036854775808"; // invalid
    EZ_TEST_BOOL(ezConversionUtils::StringToInt64(szString, iRes, &szResultPos) == EZ_FAILURE);
    EZ_TEST_INT(iRes, 42);

    iRes = 42;
    szString = "-9223372036854775808"; // last valid negative number
    EZ_TEST_BOOL(ezConversionUtils::StringToInt64(szString, iRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_INT(iRes, (ezInt64)0x8000000000000000);
    EZ_TEST_BOOL(szResultPos == szString + ezStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "-9223372036854775809"; // invalid
    EZ_TEST_BOOL(ezConversionUtils::StringToInt64(szString, iRes, &szResultPos) == EZ_FAILURE);
    EZ_TEST_INT(iRes, 42);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "StringToFloat")
  {
    const char* szString = nullptr;
    const char* szResultPos = nullptr;

    double fRes = 42;
    szString =  "23.45";
    EZ_TEST_BOOL(ezConversionUtils::StringToFloat(szString, fRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_DOUBLE(fRes, 23.45, 0.00001);
    EZ_TEST_BOOL(szResultPos == szString + ezStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString =  "-2345";
    EZ_TEST_BOOL(ezConversionUtils::StringToFloat(szString, fRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_DOUBLE(fRes, -2345.0, 0.00001);
    EZ_TEST_BOOL(szResultPos == szString + ezStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString =  "-0";
    EZ_TEST_BOOL(ezConversionUtils::StringToFloat(szString, fRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_DOUBLE(fRes, 0.0, 0.00001);
    EZ_TEST_BOOL(szResultPos == szString + ezStringUtils::GetStringElementCount(szString));  

    fRes = 42;
    szString =  "00000.000000";
    EZ_TEST_BOOL(ezConversionUtils::StringToFloat(szString, fRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_DOUBLE(fRes, 0.0, 0.00001);
    EZ_TEST_BOOL(szResultPos == szString + ezStringUtils::GetStringElementCount(szString));  

    fRes = 42;
    szString =  ".123456789";
    EZ_TEST_BOOL(ezConversionUtils::StringToFloat(szString, fRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_DOUBLE(fRes, 0.123456789, 0.00001);
    EZ_TEST_BOOL(szResultPos == szString + ezStringUtils::GetStringElementCount(szString));  

    fRes = 42;
    szString =  "+123E1";
    EZ_TEST_BOOL(ezConversionUtils::StringToFloat(szString, fRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_DOUBLE(fRes, 1230.0, 0.00001);
    EZ_TEST_BOOL(szResultPos == szString + ezStringUtils::GetStringElementCount(szString));  

    fRes = 42;
    szString =  "  \r\t 123e0";
    EZ_TEST_BOOL(ezConversionUtils::StringToFloat(szString, fRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_DOUBLE(fRes, 123.0, 0.00001);
    EZ_TEST_BOOL(szResultPos == szString + ezStringUtils::GetStringElementCount(szString));  

    fRes = 42;
    szString =  "\n123e6";
    EZ_TEST_BOOL(ezConversionUtils::StringToFloat(szString, fRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_DOUBLE(fRes, 123000000.0, 0.00001);
    EZ_TEST_BOOL(szResultPos == szString + ezStringUtils::GetStringElementCount(szString));  

    fRes = 42;
    szString =  "  123E-6";
    EZ_TEST_BOOL(ezConversionUtils::StringToFloat(szString, fRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_DOUBLE(fRes, 0.000123, 0.00001);
    EZ_TEST_BOOL(szResultPos == szString + ezStringUtils::GetStringElementCount(szString));  

    fRes = 42;
    szString =  " + - -+-123.45e-10";
    EZ_TEST_BOOL(ezConversionUtils::StringToFloat(szString, fRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_DOUBLE(fRes, -0.000000012345, 0.0000001);
    EZ_TEST_BOOL(szResultPos == szString + ezStringUtils::GetStringElementCount(szString));  

    fRes = 42;
    szString =  nullptr;
    EZ_TEST_BOOL(ezConversionUtils::StringToFloat(szString, fRes, &szResultPos) == EZ_FAILURE);
    EZ_TEST_DOUBLE(fRes, 42.0, 0.00001);

    fRes = 42;
    szString =  "";
    EZ_TEST_BOOL(ezConversionUtils::StringToFloat(szString, fRes, &szResultPos) == EZ_FAILURE);
    EZ_TEST_DOUBLE(fRes, 42.0, 0.00001);

    fRes = 42;
    szString =  "-----";
    EZ_TEST_BOOL(ezConversionUtils::StringToFloat(szString, fRes, &szResultPos) == EZ_FAILURE);
    EZ_TEST_DOUBLE(fRes, 42.0, 0.00001);

    fRes = 42;
    szString =  " + - +++ - \r \n";
    EZ_TEST_BOOL(ezConversionUtils::StringToFloat(szString, fRes, &szResultPos) == EZ_FAILURE);
    EZ_TEST_DOUBLE(fRes, 42.0, 0.00001);


    fRes = 42;
    szString =  "65.345789xabc";
    EZ_TEST_BOOL(ezConversionUtils::StringToFloat(szString, fRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_DOUBLE(fRes, 65.345789, 0.000001);
    EZ_TEST_BOOL(szResultPos == szString + 9);  

    fRes = 42;
    szString =  " \n \r \t + - 2314565.345789ff xabc";
    EZ_TEST_BOOL(ezConversionUtils::StringToFloat(szString, fRes, &szResultPos) == EZ_SUCCESS);
    EZ_TEST_DOUBLE(fRes, -2314565.345789, 0.000001);
    EZ_TEST_BOOL(szResultPos == szString + 25);  
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "StringToBool")
  {
    const char* szString = "";
    const char* szResultPos = nullptr;
    bool bRes = false;

    // true / false
    {
      bRes = false;
      szString = "true,";
      szResultPos = nullptr;
      EZ_TEST_BOOL(ezConversionUtils::StringToBool(szString, bRes, &szResultPos) == EZ_SUCCESS);
      EZ_TEST_BOOL(bRes);
      EZ_TEST_BOOL(*szResultPos == ',');

      bRes = true;
      szString = "FALSe,";
      szResultPos = nullptr;
      EZ_TEST_BOOL(ezConversionUtils::StringToBool(szString, bRes, &szResultPos) == EZ_SUCCESS);
      EZ_TEST_BOOL(!bRes);
      EZ_TEST_BOOL(*szResultPos == ',');
    }

    // on / off
    {
      bRes = false;
      szString = "\n on,";
      szResultPos = nullptr;
      EZ_TEST_BOOL(ezConversionUtils::StringToBool(szString, bRes, &szResultPos) == EZ_SUCCESS);
      EZ_TEST_BOOL(bRes);
      EZ_TEST_BOOL(*szResultPos == ',');

      bRes = true;
      szString = "\t\t \toFf,";
      szResultPos = nullptr;
      EZ_TEST_BOOL(ezConversionUtils::StringToBool(szString, bRes, &szResultPos) == EZ_SUCCESS);
      EZ_TEST_BOOL(!bRes);
      EZ_TEST_BOOL(*szResultPos == ',');
    }

    // 1 / 0
    {
      bRes = false;
      szString = "\r1,";
      szResultPos = nullptr;
      EZ_TEST_BOOL(ezConversionUtils::StringToBool(szString, bRes, &szResultPos) == EZ_SUCCESS);
      EZ_TEST_BOOL(bRes);
      EZ_TEST_BOOL(*szResultPos == ',');

      bRes = true;
      szString = "0,";
      szResultPos = nullptr;
      EZ_TEST_BOOL(ezConversionUtils::StringToBool(szString, bRes, &szResultPos) == EZ_SUCCESS);
      EZ_TEST_BOOL(!bRes);
      EZ_TEST_BOOL(*szResultPos == ',');
    }

    // yes / no
    {
      bRes = false;
      szString = "yes,";
      szResultPos = nullptr;
      EZ_TEST_BOOL(ezConversionUtils::StringToBool(szString, bRes, &szResultPos) == EZ_SUCCESS);
      EZ_TEST_BOOL(bRes);
      EZ_TEST_BOOL(*szResultPos == ',');

      bRes = true;
      szString = "NO,";
      szResultPos = nullptr;
      EZ_TEST_BOOL(ezConversionUtils::StringToBool(szString, bRes, &szResultPos) == EZ_SUCCESS);
      EZ_TEST_BOOL(!bRes);
      EZ_TEST_BOOL(*szResultPos == ',');
    }

    // enable / disable
    {
      bRes = false;
      szString = "enable,";
      szResultPos = nullptr;
      EZ_TEST_BOOL(ezConversionUtils::StringToBool(szString, bRes, &szResultPos) == EZ_SUCCESS);
      EZ_TEST_BOOL(bRes);
      EZ_TEST_BOOL(*szResultPos == ',');

      bRes = true;
      szString = "disABle,";
      szResultPos = nullptr;
      EZ_TEST_BOOL(ezConversionUtils::StringToBool(szString, bRes, &szResultPos) == EZ_SUCCESS);
      EZ_TEST_BOOL(!bRes);
      EZ_TEST_BOOL(*szResultPos == ',');
    }

    bRes = false;

    szString = "of,";
    szResultPos = nullptr;
    EZ_TEST_BOOL(ezConversionUtils::StringToBool(szString, bRes, &szResultPos) == EZ_FAILURE);
    EZ_TEST_BOOL(szResultPos == nullptr);

    szString = "aon";
    szResultPos = nullptr;
    EZ_TEST_BOOL(ezConversionUtils::StringToBool(szString, bRes, &szResultPos) == EZ_FAILURE);
    EZ_TEST_BOOL(szResultPos == nullptr);

    szString = "";
    szResultPos = nullptr;
    EZ_TEST_BOOL(ezConversionUtils::StringToBool(szString, bRes, &szResultPos) == EZ_FAILURE);
    EZ_TEST_BOOL(szResultPos == nullptr);

    szString = nullptr;
    szResultPos = nullptr;
    EZ_TEST_BOOL(ezConversionUtils::StringToBool(szString, bRes, &szResultPos) == EZ_FAILURE);
    EZ_TEST_BOOL(szResultPos == nullptr);

    szString = "tut";
    szResultPos = nullptr;
    EZ_TEST_BOOL(ezConversionUtils::StringToBool(szString, bRes, &szResultPos) == EZ_FAILURE);
    EZ_TEST_BOOL(szResultPos == nullptr);

  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "HexCharacterToIntValue")
  {
    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('0'), 0);
    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('1'), 1);
    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('2'), 2);
    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('3'), 3);
    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('4'), 4);
    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('5'), 5);
    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('6'), 6);
    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('7'), 7);
    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('8'), 8);
    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('9'), 9);

    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('a'), 10);
    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('b'), 11);
    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('c'), 12);
    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('d'), 13);
    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('e'), 14);
    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('f'), 15);

    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('A'), 10);
    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('B'), 11);
    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('C'), 12);
    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('D'), 13);
    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('E'), 14);
    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('F'), 15);

    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('g'), -1);
    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('h'), -1);
    EZ_TEST_INT(ezConversionUtils::HexCharacterToIntValue('i'), -1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ConvertHexStringToUInt32")
  {
    EZ_TEST_INT(ezConversionUtils::ConvertHexStringToUInt32(""), 0);
    EZ_TEST_INT(ezConversionUtils::ConvertHexStringToUInt32("0x"), 0);
    EZ_TEST_INT(ezConversionUtils::ConvertHexStringToUInt32("0"), 0);
    EZ_TEST_INT(ezConversionUtils::ConvertHexStringToUInt32("0x0"), 0);
    EZ_TEST_INT(ezConversionUtils::ConvertHexStringToUInt32("a"), 10);
    EZ_TEST_INT(ezConversionUtils::ConvertHexStringToUInt32("0xb"), 11);
    EZ_TEST_INT(ezConversionUtils::ConvertHexStringToUInt32("000c"), 12);
    EZ_TEST_INT(ezConversionUtils::ConvertHexStringToUInt32("000c"), 12);
    EZ_TEST_INT(ezConversionUtils::ConvertHexStringToUInt32("AA"), 170);
    EZ_TEST_INT(ezConversionUtils::ConvertHexStringToUInt32("aAbB"), 43707);
    EZ_TEST_INT(ezConversionUtils::ConvertHexStringToUInt32("FFFFffff"), 4294967295);
    EZ_TEST_INT(ezConversionUtils::ConvertHexStringToUInt32("0000FFFFffff"), 4294967295);
    EZ_TEST_INT(ezConversionUtils::ConvertHexStringToUInt32("100000000"), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExtractFloatsFromString")
  {
    float v[16];

    const char* szText = "This 1 is 2.3 or 3.141 tests in 1.2 strings, maybe 4.5,6.78or9.101!";

    ezMemoryUtils::ZeroFill(v, 16);
    EZ_TEST_INT(ezConversionUtils::ExtractFloatsFromString(szText, 0, v), 0);
    EZ_TEST_FLOAT(v[0], 0.0f, 0.0f);

    ezMemoryUtils::ZeroFill(v, 16);
    EZ_TEST_INT(ezConversionUtils::ExtractFloatsFromString(szText, 3, v), 3);
    EZ_TEST_FLOAT(v[0], 1.0f,  0.0001f);
    EZ_TEST_FLOAT(v[1], 2.3f,  0.0001f);
    EZ_TEST_FLOAT(v[2], 3.141f, 0.0001f);
    EZ_TEST_FLOAT(v[3], 0.0f, 0.0f);

    ezMemoryUtils::ZeroFill(v, 16);
    EZ_TEST_INT(ezConversionUtils::ExtractFloatsFromString(szText, 6, v), 6);
    EZ_TEST_FLOAT(v[0], 1.0f, 0.0001f);
    EZ_TEST_FLOAT(v[1], 2.3f, 0.0001f);
    EZ_TEST_FLOAT(v[2], 3.141f, 0.0001f);
    EZ_TEST_FLOAT(v[3], 1.2f, 0.0001f);
    EZ_TEST_FLOAT(v[4], 4.5f, 0.0001f);
    EZ_TEST_FLOAT(v[5], 6.78f, 0.0001f);
    EZ_TEST_FLOAT(v[6], 0.0f, 0.0f);

    ezMemoryUtils::ZeroFill(v, 16);
    EZ_TEST_INT(ezConversionUtils::ExtractFloatsFromString(szText, 10, v), 7);
    EZ_TEST_FLOAT(v[0], 1.0f, 0.0001f);
    EZ_TEST_FLOAT(v[1], 2.3f, 0.0001f);
    EZ_TEST_FLOAT(v[2], 3.141f, 0.0001f);
    EZ_TEST_FLOAT(v[3], 1.2f, 0.0001f);
    EZ_TEST_FLOAT(v[4], 4.5f, 0.0001f);
    EZ_TEST_FLOAT(v[5], 6.78f, 0.0001f);
    EZ_TEST_FLOAT(v[6], 9.101f, 0.0001f);
    EZ_TEST_FLOAT(v[7], 0.0f, 0.0f);
  }
}


