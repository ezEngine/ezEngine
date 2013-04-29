#pragma once

#include <TestFramework/Basics.h>
#include <deque>

class ezTestFramework;
class ezTestBaseClass;

struct ezSubTestEntry
{
  ezSubTestEntry() : m_iSubTestIdentifier(-1), m_szSubTestName(""), m_bEnableTest(true) { }

  ezInt32 m_iSubTestIdentifier;
  const char* m_szSubTestName;
  bool m_bEnableTest;
  double m_fTestDuration;
};

struct ezTestEntry
{
  ezTestEntry() : m_pTest(NULL), m_szTestName(""), m_bEnableTest(true), m_fTotalDuration(0.0) { }

  ezTestBaseClass* m_pTest;
  const char* m_szTestName;
  bool m_bEnableTest;
  std::deque<ezSubTestEntry> m_SubTests;
  double m_fTotalDuration;
};

struct ezTestOutput
{
  enum Enum
  {
    BeginBlock,
    EndBlock,
    ImportantInfo,
    Details,
    Success,
    Message,
    Error,
    Duration,
    FinalResult
  };
};

#define EZ_TEST_FAILURE(erroroutput, msg) \
{\
  ezTestFramework::Output(ezTestOutput::Error, erroroutput);\
  ezTestFramework::Output(ezTestOutput::BeginBlock, "");\
  if ((ezTestFramework::s_szTestBlockName != NULL) && (ezTestFramework::s_szTestBlockName[0] != '\0')) \
    ezTestFramework::Output(ezTestOutput::Message, "Block: '%s'", ezTestFramework::s_szTestBlockName);\
  ezTestFramework::Output(ezTestOutput::ImportantInfo, "File: %s", EZ_SOURCE_FILE);\
  ezTestFramework::Output(ezTestOutput::ImportantInfo, "Line: %i", EZ_SOURCE_LINE);\
  ezTestFramework::Output(ezTestOutput::ImportantInfo, "Function: %s", EZ_SOURCE_FUNCTION);\
  if ((msg != NULL) && (msg[0] != '\0'))\
    ezTestFramework::Output(ezTestOutput::Message, "Message: %s", msg);\
  ezTestFramework::Output(ezTestOutput::EndBlock, "");\
}

