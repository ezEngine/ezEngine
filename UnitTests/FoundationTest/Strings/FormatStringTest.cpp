#include <PCH.h>
#include <Foundation/Strings/FormatString.h>

#if (__cplusplus >= 201402L || _MSC_VER >= 1900)

void TestFormat(const ezFormatString& str, const char* szExpected)
{
  ezStringBuilder sb;
  const char* szText = str.GetText(sb);

  EZ_TEST_STRING(szText, szExpected);
}

EZ_CREATE_SIMPLE_TEST(Strings, FormatString)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Basics")
  {
    const char* tmp = "stringviewstuff";

    const char* sz = "sz";
    ezString string = "string";
    ezStringBuilder sb = "builder";
    ezStringView sv(tmp + 6, tmp + 10);

    TestFormat(ezFmt("{0}, {1}, {2}, {3}", ezInt8(-1), ezInt16(-2), ezInt32(-3), ezInt64(-4)), "-1, -2, -3, -4");
    TestFormat(ezFmt("{0}, {1}, {2}, {3}", ezUInt8(1), ezUInt16(2), ezUInt32(3), ezUInt64(4)), "1, 2, 3, 4");
    TestFormat(ezFmt("'{0}, {1}'", "inl", sz), "'inl, sz'");
    TestFormat(ezFmt("'{0}'", string), "'string'");
    TestFormat(ezFmt("'{0}'", sb), "'builder'");
    TestFormat(ezFmt("'{0}'", sv), "'view'");

    TestFormat(ezFmt("{3}, {1}, {0}, {2}", ezArgF(23.12345f, 1), ezArgI(42), 17, 12.34f), "12.34, 42, 23.1, 17");
  }
}

#endif
