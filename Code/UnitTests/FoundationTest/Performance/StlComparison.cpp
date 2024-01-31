#include <FoundationTest/FoundationTestPCH.h>

#if EZ_ENABLED(EZ_INTEROP_STL_STRINGS)

#  include <Foundation/Time/Stopwatch.h>
#  include <filesystem>
#  include <string>
#  include <string_view>

namespace fs = std::filesystem;

EZ_CREATE_SIMPLE_TEST_GROUP(StlComparison);

using Func = void (*)();

void Measure(const char* szTitle, Func f)
{
  // pre-warm
  f();

  ezStopwatch sw;
  // measure
  f();
  const ezTime t = sw.Checkpoint();

  ezStringBuilder s;
  s.SetFormat("{}: {}ms", szTitle, ezArgF(t.GetMilliseconds(), 2, false, 10));

  ezTestFramework::Output(ezTestOutput::Details, s.GetData());
}

const ezUInt32 itersDefault = 1000000;
const ezUInt32 itersFew = 100000;

const char* g_Cstr[2] = {"Path/Substitution1/Test/Substitution2-42.png", "Path/Substitution1/Test/Substitution2-42.png"};
std::string g_StdStr[2] = {g_Cstr[0], g_Cstr[1]};

void ezAssign()
{
  ezStringBuilder sb;
  for (ezUInt32 i = 0; i < itersDefault; ++i)
  {
    sb = g_Cstr[i & 1];
  }
}

void StdAssign()
{
  for (ezUInt32 i = 0; i < itersDefault; ++i)
  {
    std::string s = g_Cstr[i & 1];
  }
}

ezString ezGetString()
{
  return g_StdStr[1];
}

std::string StdGetString()
{
  return g_StdStr[1];
}

void ezReturnString()
{
  ezString s;
  for (ezUInt32 i = 0; i < itersFew; ++i)
  {
    s = ezGetString();
  }
}

void StdReturnString()
{
  std::string s;
  for (ezUInt32 i = 0; i < itersFew; ++i)
  {
    s = StdGetString();
  }
}

void ezAppend1()
{
  ezStringBuilder sb;

  for (ezUInt32 i = 0; i < itersDefault; ++i)
  {
    sb.Set(g_StdStr[0], g_StdStr[1]);
  }
}

void StdAppend1()
{
  for (ezUInt32 i = 0; i < itersDefault; ++i)
  {
    std::string s = g_StdStr[0] + g_StdStr[1];
  }
}

void ezAppend2()
{
  ezStringBuilder sb;

  for (ezUInt32 i = 0; i < itersDefault; ++i)
  {
    sb.Set(g_Cstr[0], g_Cstr[1]);
  }
}

void StdAppend2()
{
  for (ezUInt32 i = 0; i < itersDefault; ++i)
  {
    std::string s = std::string(g_Cstr[0]) + std::string(g_Cstr[1]);
  }
}

void ezAppend3()
{
  ezStringBuilder sb;

  for (ezUInt32 i = 0; i < itersDefault; ++i)
  {
    sb = g_Cstr[0];
    sb.Append(g_Cstr[1]);
  }
}

void StdAppend3()
{
  for (ezUInt32 i = 0; i < itersDefault; ++i)
  {
    std::string s = std::string(g_Cstr[0]);
    s += std::string(g_Cstr[1]);
  }
}

void ezPath1()
{
  ezStringBuilder sb;

  for (ezUInt32 i = 0; i < itersDefault; ++i)
  {
    sb.Clear();
    sb.AppendPath("C:", "folder", u8"чепуха", "file.txt");
  }
}

void StdPath1()
{
  for (ezUInt32 i = 0; i < itersDefault; ++i)
  {
    fs::path s = fs::path("C:") / "folder" / u8"чепуха" / "file.txt";
  }
}

void ezPath2()
{
  ezString out;

  for (ezUInt32 i = 0; i < itersDefault; ++i)
  {
    ezStringBuilder sb;
    sb.AppendPath("C:", "folder", u8"чепуха", "file.txt");

    out = sb;
  }
}

void StdPath2()
{
  fs::path s;
  for (ezUInt32 i = 0; i < itersDefault; ++i)
  {
    s = fs::path("C:") / "folder" / u8"чепуха" / "file.txt";
  }
}

EZ_CREATE_SIMPLE_TEST(StlComparison, Strings)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "std::string Comparison")
  {
    Measure("StdAssign", StdAssign);
    Measure(" ezAssign", ezAssign);
    Measure("StdReturnString", StdReturnString);
    Measure(" ezReturnString", ezReturnString);
    Measure("StdAppend1", StdAppend1);
    Measure(" ezAppend1", ezAppend1);
    Measure("StdAppend2", StdAppend2);
    Measure(" ezAppend2", ezAppend2);
    Measure("StdAppend3", StdAppend3);
    Measure(" ezAppend3", ezAppend3);
    Measure("StdPath1", StdPath1);
    Measure(" ezPath1", ezPath1);
    Measure("StdPath2", StdPath2);
    Measure(" ezPath2", ezPath2);
  }
}



#endif
