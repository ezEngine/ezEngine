#include <PCH.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

void TestFormat(const ezFormatString& str, const char* szExpected)
{
  ezStringBuilder sb;
  const char* szText = str.GetText(sb);

  EZ_TEST_STRING(szText, szExpected);
}

void CompareSnprintf(ezStringBuilder& log, const ezFormatString& str, const char* szFormat, ...)
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
      const char* szText = str.GetText(sb);
    }

    t3 = sw.Checkpoint();
  }

  log.AppendPrintf("ez: %.2f msec, std: %.2f msec, ezFmt: %.2f msec : %s -> %s\n", t1.GetMilliseconds(), t2.GetMilliseconds(), t3.GetMilliseconds(), szFormat, Temp1);

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
    TestFormat(ezFmt("'{0}, {1}'", "inl", sz), "'inl, sz'");
    TestFormat(ezFmt("'{0}'", string), "'string'");
    TestFormat(ezFmt("'{0}'", sb), "'builder'");
    TestFormat(ezFmt("'{0}'", sv), "'view'");

    TestFormat(ezFmt("{3}, {1}, {0}, {2}", ezArgF(23.12345f, 1), ezArgI(42), 17, 12.34f), "12.34, 42, 23.1, 17");
  }

  EZ_TEST_BLOCK(ezTestBlock::Disabled, "Compare")
  {
    CompareSnprintf(perfLog, ezFmt("Hello {0}, i = {1}, f = {2}", "World", 42, ezArgF(3.141f, 2)), "Hello %s, i = %i, f = %.2f", "World", 42, 3.141f);
    CompareSnprintf(perfLog, ezFmt("No formatting at all"), "No formatting at all");
    CompareSnprintf(perfLog, ezFmt("{0}, {1}, {2}, {3}, {4}", "AAAAAA", "BBBBBBB", "CCCCCC", "DDDDDDDDDDDDD", "EE"), "%s, %s, %s, %s, %s", "AAAAAA", "BBBBBBB", "CCCCCC", "DDDDDDDDDDDDD", "EE");
    CompareSnprintf(perfLog, ezFmt("{0}", 23), "%i", 23);
    CompareSnprintf(perfLog, ezFmt("{0}", 23.123456789), "%f", 23.123456789);
    CompareSnprintf(perfLog, ezFmt("{0}", ezArgF(23.123456789, 2)), "%.2f", 23.123456789);
    CompareSnprintf(perfLog, ezFmt("{0}", ezArgI(123456789, 20, true)), "%020i", 123456789);
    CompareSnprintf(perfLog, ezFmt("{0}", ezArgI(123456789, 20, true, 16)), "%020X", 123456789);
    CompareSnprintf(perfLog, ezFmt("{0}", ezArgI(1234567890987ll, 30, false, 16)), "% 30x", 1234567890987ll);
    CompareSnprintf(perfLog, ezFmt("{0}, {1}, {2}, {3}, {4}", 0, 1, 2, 3, 4), "%i, %i, %i, %i, %i", 0, 1, 2, 3, 4);
    CompareSnprintf(perfLog, ezFmt("{0}, {1}, {2}, {3}, {4}", 0.1, 1.1, 2.1, 3.1, 4.1), "%.1f, %.1f, %.1f, %.1f, %.1f", 0.1, 1.1, 2.1, 3.1, 4.1);
    CompareSnprintf(perfLog, ezFmt("{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9}", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9), "%i, %i, %i, %i, %i, %i, %i, %i, %i, %i", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    CompareSnprintf(perfLog, ezFmt("{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9}", 0.1, 1.1, 2.1, 3.1, 4.1, 5.1, 6.1, 7.1, 8.1, 9.1), "%.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f", 0.1, 1.1, 2.1, 3.1, 4.1, 5.1, 6.1, 7.1, 8.1, 9.1);

    //FILE* file = fopen("D:\\snprintf_perf.txt", "wb");
    //if (file)
    //{
    //  fwrite(perfLog.GetData(), 1, perfLog.GetElementCount(), file);
    //  fclose(file);
    //}
  }
}

