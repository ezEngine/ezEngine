#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Time/Timestamp.h>

#include <Foundation/Types/ScopeExit.h>
#include <stdarg.h>

void TestFormat(const ezFormatString& str, const char* szExpected)
{
  ezStringBuilder sb;
  ezStringView szText = str.GetText(sb);

  EZ_TEST_STRING(szText, szExpected);
}

void TestFormatWChar(const ezFormatString& str, const wchar_t* pExpected)
{
  ezStringBuilder sb;
  ezStringView szText = str.GetText(sb);

  EZ_TEST_WSTRING(ezStringWChar(szText), pExpected);
}

void CompareSnprintf(ezStringBuilder& ref_sLog, const ezFormatString& str, const char* szFormat, ...)
{
  va_list args;
  va_start(args, szFormat);

  char Temp1[256];
  char Temp2[256];

  // reusing args list crashes on GCC / Clang
  ezStringUtils::vsnprintf(Temp1, 256, szFormat, args);
  vsnprintf(Temp2, 256, szFormat, args);
  EZ_TEST_STRING(Temp1, Temp2);

  ezTime t1, t2, t3;
  ezStopwatch sw;
  {
    sw.StopAndReset();

    for (ezUInt32 i = 0; i < 10000; ++i)
    {
      ezStringUtils::vsnprintf(Temp1, 256, szFormat, args);
    }

    t1 = sw.Checkpoint();
  }

  {
    sw.StopAndReset();

    for (ezUInt32 i = 0; i < 10000; ++i)
    {
      vsnprintf(Temp2, 256, szFormat, args);
    }

    t2 = sw.Checkpoint();
  }

  {
    ezStringBuilder sb;

    sw.StopAndReset();
    for (ezUInt32 i = 0; i < 10000; ++i)
    {
      ezStringView sText = str.GetText(sb);
    }

    t3 = sw.Checkpoint();
  }

  ref_sLog.AppendFormat("ez: {0} msec, std: {1} msec, ezFmt: {2} msec : {3} -> {4}\n", ezArgF(t1.GetMilliseconds(), 2), ezArgF(t2.GetMilliseconds(), 2),
    ezArgF(t3.GetMilliseconds(), 2), szFormat, Temp1);

  va_end(args);
}

EZ_CREATE_SIMPLE_TEST(Strings, FormatString)
{
  ezStringBuilder perfLog;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Basics")
  {
    const char* tmp = "stringviewstuff";

    const char* sz = "sz";
    ezString string = "string";
    ezStringBuilder sb = "builder";
    ezStringView sv(tmp + 6, tmp + 10);

    TestFormat(ezFmt("{0}, {1}, {2}, {3}", ezInt8(-1), ezInt16(-2), ezInt32(-3), ezInt64(-4)), "-1, -2, -3, -4");
    TestFormat(ezFmt("{0}, {1}, {2}, {3}", ezUInt8(1), ezUInt16(2), ezUInt32(3), ezUInt64(4)), "1, 2, 3, 4");

    TestFormat(ezFmt("{0}, {1}", ezArgHumanReadable(0ll), ezArgHumanReadable(1ll)), "0, 1");
    TestFormat(ezFmt("{0}, {1}", ezArgHumanReadable(-0ll), ezArgHumanReadable(-1ll)), "0, -1");
    TestFormat(ezFmt("{0}, {1}", ezArgHumanReadable(999ll), ezArgHumanReadable(1000ll)), "999, 1.00K");
    TestFormat(ezFmt("{0}, {1}", ezArgHumanReadable(-999ll), ezArgHumanReadable(-1000ll)), "-999, -1.00K");
    // 999.999 gets rounded up for precision 2, so result is 1000.00K not 999.99K
    TestFormat(ezFmt("{0}, {1}", ezArgHumanReadable(999'999ll), ezArgHumanReadable(1'000'000ll)), "1000.00K, 1.00M");
    TestFormat(ezFmt("{0}, {1}", ezArgHumanReadable(-999'999ll), ezArgHumanReadable(-1'000'000ll)), "-1000.00K, -1.00M");

    TestFormat(ezFmt("{0}, {1}", ezArgFileSize(0u), ezArgFileSize(1u)), "0B, 1B");
    TestFormat(ezFmt("{0}, {1}", ezArgFileSize(1023u), ezArgFileSize(1024u)), "1023B, 1.00KB");
    // 1023.999 gets rounded up for precision 2, so result is 1024.00KB not 1023.99KB
    TestFormat(ezFmt("{0}, {1}", ezArgFileSize(1024u * 1024u - 1u), ezArgFileSize(1024u * 1024u)), "1024.00KB, 1.00MB");

    const char* const suffixes[] = {" Foo", " Bar", " Foobar"};
    const ezUInt32 suffixCount = EZ_ARRAY_SIZE(suffixes);
    TestFormat(ezFmt("{0}", ezArgHumanReadable(0ll, 25u, suffixes, suffixCount)), "0 Foo");
    TestFormat(ezFmt("{0}", ezArgHumanReadable(25ll, 25u, suffixes, suffixCount)), "1.00 Bar");
    TestFormat(ezFmt("{0}", ezArgHumanReadable(25ll * 25ll * 2ll, 25u, suffixes, suffixCount)), "2.00 Foobar");

    TestFormat(ezFmt("{0}", ezArgHumanReadable(-0ll, 25u, suffixes, suffixCount)), "0 Foo");
    TestFormat(ezFmt("{0}", ezArgHumanReadable(-25ll, 25u, suffixes, suffixCount)), "-1.00 Bar");
    TestFormat(ezFmt("{0}", ezArgHumanReadable(-25ll * 25ll * 2ll, 25u, suffixes, suffixCount)), "-2.00 Foobar");

    TestFormat(ezFmt("'{0}, {1}'", "inl", sz), "'inl, sz'");
    TestFormat(ezFmt("'{0}'", string), "'string'");
    TestFormat(ezFmt("'{0}'", sb), "'builder'");
    TestFormat(ezFmt("'{0}'", sv), "'view'");

    TestFormat(ezFmt("{3}, {1}, {0}, {2}", ezArgF(23.12345f, 1), ezArgI(42), 17, 12.34f), "12.34, 42, 23.1, 17");

    const wchar_t* wsz = L"wsz";
    TestFormatWChar(ezFmt("'{0}, {1}'", "inl", wsz), L"'inl, wsz'");
    TestFormatWChar(ezFmt("'{0}, {1}'", L"inl", wsz), L"'inl, wsz'");
    // Temp buffer limit is 63 byte (64 including trailing zero). Each character in UTF-8 can potentially use 4 byte.
    // All input characters are 1 byte, so the 60th character is the last with 4 bytes left in the buffer.
    // Thus we end up with truncation after 60 characters.
    const wchar_t* wszTooLong = L"123456789.123456789.123456789.123456789.123456789.123456789.WAAAAAAAAAAAAAAH";
    const wchar_t* wszTooLongExpected = L"123456789.123456789.123456789.123456789.123456789.123456789.";
    const wchar_t* wszTooLongExpected2 =
      L"'123456789.123456789.123456789.123456789.123456789.123456789., 123456789.123456789.123456789.123456789.123456789.123456789.'";
    TestFormatWChar(ezFmt("{0}", wszTooLong), wszTooLongExpected);
    TestFormatWChar(ezFmt("'{0}, {1}'", wszTooLong, wszTooLong), wszTooLongExpected2);
  }

  EZ_TEST_BLOCK(ezTestBlock::DisabledNoWarning, "Compare Performance")
  {
    CompareSnprintf(perfLog, ezFmt("Hello {0}, i = {1}, f = {2}", "World", 42, ezArgF(3.141f, 2)), "Hello %s, i = %i, f = %.2f", "World", 42, 3.141f);
    CompareSnprintf(perfLog, ezFmt("No formatting at all"), "No formatting at all");
    CompareSnprintf(perfLog, ezFmt("{0}, {1}, {2}, {3}, {4}", "AAAAAA", "BBBBBBB", "CCCCCC", "DDDDDDDDDDDDD", "EE"), "%s, %s, %s, %s, %s", "AAAAAA",
      "BBBBBBB", "CCCCCC", "DDDDDDDDDDDDD", "EE");
    CompareSnprintf(perfLog, ezFmt("{0}", 23), "%i", 23);
    CompareSnprintf(perfLog, ezFmt("{0}", 23.123456789), "%f", 23.123456789);
    CompareSnprintf(perfLog, ezFmt("{0}", ezArgF(23.123456789, 2)), "%.2f", 23.123456789);
    CompareSnprintf(perfLog, ezFmt("{0}", ezArgI(123456789, 20, true)), "%020i", 123456789);
    CompareSnprintf(perfLog, ezFmt("{0}", ezArgI(123456789, 20, true, 16)), "%020X", 123456789);
    CompareSnprintf(perfLog, ezFmt("{0}", ezArgU(1234567890987ll, 30, false, 16)), "%30llx", 1234567890987ll);
    CompareSnprintf(perfLog, ezFmt("{0}", ezArgU(1234567890987ll, 30, false, 16, true)), "%30llX", 1234567890987ll);
    CompareSnprintf(perfLog, ezFmt("{0}, {1}, {2}, {3}, {4}", 0, 1, 2, 3, 4), "%i, %i, %i, %i, %i", 0, 1, 2, 3, 4);
    CompareSnprintf(perfLog, ezFmt("{0}, {1}, {2}, {3}, {4}", 0.1, 1.1, 2.1, 3.1, 4.1), "%.1f, %.1f, %.1f, %.1f, %.1f", 0.1, 1.1, 2.1, 3.1, 4.1);
    CompareSnprintf(perfLog, ezFmt("{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9}", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9),
      "%i, %i, %i, %i, %i, %i, %i, %i, %i, %i", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    CompareSnprintf(perfLog, ezFmt("{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9}", 0.1, 1.1, 2.1, 3.1, 4.1, 5.1, 6.1, 7.1, 8.1, 9.1),
      "%.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f", 0.1, 1.1, 2.1, 3.1, 4.1, 5.1, 6.1, 7.1, 8.1, 9.1);
    CompareSnprintf(perfLog, ezFmt("{0}", ezArgC('z')), "%c", 'z');

    CompareSnprintf(perfLog, ezFmt("{}, {}, {}, {}, {}, {}, {}, {}, {}, {}", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9), "%i, %i, %i, %i, %i, %i, %i, %i, %i, %i",
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9);

    // FILE* file = fopen("D:\\snprintf_perf.txt", "wb");
    // if (file)
    //{
    //  fwrite(perfLog.GetData(), 1, perfLog.GetElementCount(), file);
    //  fclose(file);
    //}
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Auto Increment")
  {
    TestFormat(ezFmt("{}{}{}{}", ezInt8(1), ezInt16(2), ezInt32(3), ezInt64(4)), "1234");
    TestFormat(ezFmt("{3}{2}{1}{0}", ezInt8(1), ezInt16(2), ezInt32(3), ezInt64(4)), "4321");

    TestFormat(ezFmt("{}, {}, {}, {}", ezInt8(-1), ezInt16(-2), ezInt32(-3), ezInt64(-4)), "-1, -2, -3, -4");
    TestFormat(ezFmt("{}, {}, {}, {}", ezUInt8(1), ezUInt16(2), ezUInt32(3), ezUInt64(4)), "1, 2, 3, 4");

    TestFormat(ezFmt("{0}, {}, {}, {}", ezUInt8(1), ezUInt16(2), ezUInt32(3), ezUInt64(4)), "1, 2, 3, 4");

    TestFormat(ezFmt("{1}, {}, {}, {}", ezUInt8(1), ezUInt16(2), ezUInt32(3), ezUInt64(4), ezUInt64(5)), "2, 3, 4, 5");

    TestFormat(ezFmt("{2}, {}, {1}, {}", ezUInt8(1), ezUInt16(2), ezUInt32(3), ezUInt64(4), ezUInt64(5)), "3, 4, 2, 3");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezTime")
  {
    TestFormat(ezFmt("{}", ezTime()), "0ns");
    TestFormat(ezFmt("{}", ezTime::MakeFromNanoseconds(999)), "999ns");
    TestFormat(ezFmt("{}", ezTime::MakeFromNanoseconds(999.1)), "999.1ns");
    TestFormat(ezFmt("{}", ezTime::MakeFromMicroseconds(999)), (const char*)u8"999\u00B5s");     // Utf-8 encoding for the microsecond sign
    TestFormat(ezFmt("{}", ezTime::MakeFromMicroseconds(999.2)), (const char*)u8"999.2\u00B5s"); // Utf-8 encoding for the microsecond sign
    TestFormat(ezFmt("{}", ezTime::MakeFromMilliseconds(-999)), "-999ms");
    TestFormat(ezFmt("{}", ezTime::MakeFromMilliseconds(-999.3)), "-999.3ms");
    TestFormat(ezFmt("{}", ezTime::MakeFromSeconds(59)), "59sec");
    TestFormat(ezFmt("{}", ezTime::MakeFromSeconds(-59.9)), "-59.9sec");
    TestFormat(ezFmt("{}", ezTime::MakeFromSeconds(75)), "1min 15sec");
    TestFormat(ezFmt("{}", ezTime::MakeFromSeconds(-75.4)), "-1min 15sec");
    TestFormat(ezFmt("{}", ezTime::MakeFromMinutes(59)), "59min 0sec");
    TestFormat(ezFmt("{}", ezTime::MakeFromMinutes(-1)), "-1min 0sec");
    TestFormat(ezFmt("{}", ezTime::MakeFromMinutes(90)), "1h 30min 0sec");
    TestFormat(ezFmt("{}", ezTime::MakeFromMinutes(-90.5)), "-1h 30min 30sec");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezDateTime")
  {
    {
      ezDateTime dt;
      dt.SetYear(2019);
      dt.SetMonth(6);
      dt.SetDay(12);
      dt.SetHour(13);
      dt.SetMinute(26);
      dt.SetSecond(51);
      dt.SetMicroseconds(7000);

      TestFormat(ezFmt("{}", dt), "2019-06-12_13-26-51-007");
    }

    {
      ezDateTime dt;
      dt.SetYear(0);
      dt.SetMonth(1);
      dt.SetDay(1);
      dt.SetHour(0);
      dt.SetMinute(0);
      dt.SetSecond(0);
      dt.SetMicroseconds(0);

      TestFormat(ezFmt("{}", dt), "0000-01-01_00-00-00-000");
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Sensitive Info")
  {
    auto prev = ezArgSensitive::s_BuildStringCB;
    EZ_SCOPE_EXIT(ezArgSensitive::s_BuildStringCB = prev);

    ezArgSensitive::s_BuildStringCB = ezArgSensitive::BuildString_SensitiveUserData_Hash;

    ezStringBuilder fmt;

    fmt.SetFormat("Password: {}", ezArgSensitive("hunter2", "pwd"));
    EZ_TEST_STRING(fmt, "Password: sud:pwd#96d66ce6($7)");

    fmt.SetFormat("Password: {}", ezArgSensitive("hunter2"));
    EZ_TEST_STRING(fmt, "Password: sud:#96d66ce6($7)");
  }
}
