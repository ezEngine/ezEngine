#pragma once

#include <Foundation/Basics.h>
#include <TestFramework/Framework/Declarations.h>
#include <TestFramework/Framework/TestBaseClass.h>
#include <TestFramework/Framework/SimpleTest.h>

class EZ_TEST_DLL ezTestFramework
{
public:
  typedef void(*OutputHandler)(ezTestOutput::Enum Type, const char* szMsg);

  static void RegisterOutputHandler(OutputHandler Handler);

  static void GatherAllTests(std::deque<ezTestEntry>& out_AllTests);
  static void ExecuteAllTests(std::deque<ezTestEntry>& inout_TestsToExecute);

  static void Output(ezTestOutput::Enum Type, const char* szMsg, ...);

  static ezInt32 GetErrorCount();
  static ezInt32 GetTestsPassedCount() { return s_iTestsPassed; }
  static ezInt32 GetTestsFailedCount() { return s_iTestsFailed; }
  static bool GetAssertOnTestFail() { return s_bAssertOnTestFail; }
  static void SetAssertOnTestFail(bool m_bAssertOnTestFail) { s_bAssertOnTestFail = m_bAssertOnTestFail; }

  static const char* s_szTestBlockName;

private:
  static ezInt32 s_iErrorCount;
  static ezInt32 s_iTestsFailed;
  static ezInt32 s_iTestsPassed;
  static bool s_bAssertOnTestFail;
  static std::deque<OutputHandler> s_OutputHandlers;

};

// Starts a small test block inside a larger test. 
// First parameter allows to quickly disable a block depending on a condition (e.g. platform). 
// Second parameter just gives it a name for better error reporting.
// Also skipped tests are highlighted in the output, such that people can quickly see when a test is currently deactivated.
#define EZ_TEST_BLOCK(enable, name) \
  ezTestFramework::s_szTestBlockName = name; \
  if (!enable) \
  { \
    ezTestFramework::s_szTestBlockName = ""; \
    ezTestFramework::Output(ezTestOutput::Message, "Skipped Test Block '%s'", name); \
  } \
  else

#define EZ_TEST_DEBUG_BREAK if (ezTestFramework::GetAssertOnTestFail()) \
  EZ_DEBUG_BREAK

#define EZ_TEST(condition) EZ_TEST_MSG(condition, "")

#define EZ_TEST_MSG(condition, msg) if (!(condition)) \
{ \
  EZ_TEST_FAILURE("Test failed: " EZ_STRINGIZE(condition), msg) \
  EZ_TEST_DEBUG_BREAK \
}

#define EZ_TEST_FLOAT(f1, f2, epsilon) EZ_TEST_FLOAT_MSG(f1, f2, epsilon, "")

#define EZ_TEST_FLOAT_MSG(f1, f2, epsilon, msg) \
{ \
  const float r1 = (f1); \
  const float r2 = (f2); \
  const float fD = r1 - r2; \
  if (fD < -epsilon || fD > +epsilon) \
  { \
    char szLocal_TestMacro[256]; \
    sprintf (szLocal_TestMacro, "Failure: '%s' (%.8f) does not equal '%s' (%.8f) within an epsilon of %.8f", EZ_STRINGIZE(f1), r1, EZ_STRINGIZE(f2), r2, epsilon); \
    EZ_TEST_FAILURE(szLocal_TestMacro, msg); \
    EZ_TEST_DEBUG_BREAK \
  } \
}

#define EZ_TEST_INT(i1, i2) EZ_TEST_INT_MSG(i1, i2, "")

#define EZ_TEST_INT_MSG(i1, i2, msg) \
{ \
  const ezInt32 r1 = (ezInt32) (i1); \
  const ezInt32 r2 = (ezInt32) (i2); \
  \
  if (r1 != r2) \
  { \
    char szLocal_TestMacro[256]; \
    sprintf (szLocal_TestMacro, "Failure: '%s' (%i) does not equal '%s' (%i)", EZ_STRINGIZE(i1), r1, EZ_STRINGIZE(i2), r2); \
    EZ_TEST_FAILURE(szLocal_TestMacro, msg); \
    EZ_TEST_DEBUG_BREAK \
  } \
}

#define EZ_TEST_STRING(i1, i2) EZ_TEST_STRING_MSG(i1, i2, "")

#define EZ_TEST_STRING_MSG(s1, s2, msg) \
{ \
  const char* sz1 = s1; \
  const char* sz2 = s2; \
  \
  if (strcmp(sz1, sz2) != 0) \
  { \
    char szLocal_TestMacro[512]; \
    sprintf (szLocal_TestMacro, "Failure: '%s' (%s) does not equal '%s' (%s)", EZ_STRINGIZE(s1), sz1, EZ_STRINGIZE(s2), sz2); \
    EZ_TEST_FAILURE(szLocal_TestMacro, msg); \
    EZ_TEST_DEBUG_BREAK \
  } \
}

#define EZ_TEST_VEC2(i1, i2, epsilon) EZ_TEST_VEC2_MSG(i1, i2, epsilon, "")

#define EZ_TEST_VEC2_MSG(r1, r2, epsilon, msg) \
{ \
  const ezVec2 v1 = (ezVec2) (r1); \
  const ezVec2 v2 = (ezVec2) (r2); \
  \
  EZ_TEST_FLOAT_MSG(v1.x, v2.x, epsilon, msg); \
  EZ_TEST_FLOAT_MSG(v1.y, v2.y, epsilon, msg); \
}

#define EZ_TEST_VEC3(i1, i2, epsilon) EZ_TEST_VEC3_MSG(i1, i2, epsilon, "")

#define EZ_TEST_VEC3_MSG(r1, r2, epsilon, msg) \
{ \
  const ezVec3 v1 = (ezVec3) (r1); \
  const ezVec3 v2 = (ezVec3) (r2); \
  \
  EZ_TEST_FLOAT_MSG(v1.x, v2.x, epsilon, msg); \
  EZ_TEST_FLOAT_MSG(v1.y, v2.y, epsilon, msg); \
  EZ_TEST_FLOAT_MSG(v1.z, v2.z, epsilon, msg); \
}

#define EZ_TEST_VEC4(i1, i2, epsilon) EZ_TEST_VEC4_MSG(i1, i2, epsilon, "")

#define EZ_TEST_VEC4_MSG(r1, r2, epsilon, msg) \
{ \
  const ezVec4 v1 = (ezVec4) (r1); \
  const ezVec4 v2 = (ezVec4) (r2); \
  \
  EZ_TEST_FLOAT_MSG(v1.x, v2.x, epsilon, msg); \
  EZ_TEST_FLOAT_MSG(v1.y, v2.y, epsilon, msg); \
  EZ_TEST_FLOAT_MSG(v1.z, v2.z, epsilon, msg); \
  EZ_TEST_FLOAT_MSG(v1.w, v2.w, epsilon, msg); \
}