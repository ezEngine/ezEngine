#pragma once

#include <TestFramework/Basics.h>
#include <deque>
#include <string>

class ezTestFramework;
class ezTestBaseClass;

struct ezTestOutput
{
  /// \brief Defines the type of output message for ezTestOutputMessage.
  enum Enum
  {
    StartOutput,
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

/// \brief A message of type ezTestOutput::Enum, stored in ezTestResult.
struct ezTestOutputMessage
{
  ezTestOutputMessage() : m_Type(ezTestOutput::ImportantInfo) { }

  ezTestOutput::Enum m_Type;
  std::string m_sMessage;
};

struct ezTestResultQuery
{
  /// \brief Defines what information should be accumulated over the sub-tests in ezTestEntry::GetSubTestCount.
  enum Enum
  {
    Count,
    Enabled,
    Executed,
    Success,
    Errors,
  };
};

/// \brief Stores the results of a test run. Used by both ezTestEntry and ezSubTestEntry.
struct ezTestResult
{
  ezTestResult() : m_bEnableTest(true), m_bExecuted(false), m_bSuccess(false), m_fTestDuration(0.0), m_uiErrorCount(0) { }
  void Reset()
  {
    m_bExecuted = false;
    m_bSuccess = false;
    m_fTestDuration = 0.0;
    m_uiErrorCount = 0;
    m_TestOutput.clear();
  }

  bool m_bEnableTest;

  bool m_bExecuted;
  bool m_bSuccess;
  double m_fTestDuration;
  ezUInt32 m_uiErrorCount;
  std::deque<ezTestOutputMessage> m_TestOutput;
};

/// \brief Stores the identification and result information of a sub-test.
struct ezSubTestEntry
{
  ezSubTestEntry() : m_iSubTestIdentifier(-1), m_szSubTestName("") { }

  ezInt32 m_iSubTestIdentifier;
  const char* m_szSubTestName;

  ezTestResult m_Result;
};

/// \brief Stores the identification and result information of a test.
struct ezTestEntry
{
  ezTestEntry() : m_pTest(NULL), m_szTestName("") { }

  /// \brief Accumulates the sub-test results of the type given by ezTestResultQuery::Enum.
  ezUInt32 GetSubTestCount(ezTestResultQuery::Enum countQuery = ezTestResultQuery::Count) const
  {
    ezUInt32 uiAccumulator = 0;
    const ezUInt32 uiSubTests = (ezUInt32)m_SubTests.size();
    if (countQuery == ezTestResultQuery::Count)
      return uiSubTests;

    for (ezUInt32 uiSubTest = 0; uiSubTest < uiSubTests; ++uiSubTest)
    {
      switch (countQuery)
      {
      case ezTestResultQuery::Enabled:
        uiAccumulator += m_SubTests[uiSubTest].m_Result.m_bEnableTest ? 1 : 0;
        break;
      case ezTestResultQuery::Executed:
        uiAccumulator += m_SubTests[uiSubTest].m_Result.m_bExecuted ? 1 : 0;
        break;
      case ezTestResultQuery::Success:
        uiAccumulator += m_SubTests[uiSubTest].m_Result.m_bSuccess ? 1 : 0;
        break;
      case ezTestResultQuery::Errors:
        uiAccumulator += m_SubTests[uiSubTest].m_Result.m_uiErrorCount;
        break;
      default:
        break;
      }
    }
    return uiAccumulator;
  }

  ezTestBaseClass* m_pTest;
  const char* m_szTestName;
  std::deque<ezSubTestEntry> m_SubTests;

  ezTestResult m_Result;
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

