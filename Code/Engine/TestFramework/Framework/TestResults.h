#pragma once

#include <TestFramework/Basics.h>
#include <TestFramework/Framework/Declarations.h>
#include <string>
#include <deque>
#include <vector>

struct ezTestOutput
{
  /// \brief Defines the type of output message for ezTestOutputMessage.
  enum Enum
  {
    InvalidType = -1,
    StartOutput = 0,
    BeginBlock,
    EndBlock,
    ImportantInfo,
    Details,
    Success,
    Message,
    Error,
    Duration,
    FinalResult,
    AllOutputTypes
  };

  static const char* const s_Names[];
  static const char* ToString(Enum type);
  static Enum FromString(const char* szName);
};

/// \brief A message of type ezTestOutput::Enum, stored in ezResult.
struct ezTestErrorMessage
{
  ezTestErrorMessage() : m_iLine(-1) { }

  std::string m_sError;
  std::string m_sBlock;
  std::string m_sFile;
  ezInt32 m_iLine;
  std::string m_sFunction;
  std::string m_sMessage;
};

/// \brief A message of type ezTestOutput::Enum, stored in ezResult.
struct ezTestOutputMessage
{
  ezTestOutputMessage() : m_Type(ezTestOutput::ImportantInfo), m_iErrorIndex(-1) { }

  ezTestOutput::Enum m_Type;
  std::string m_sMessage;
  ezInt32 m_iErrorIndex;
};

struct ezTestResultQuery
{
  /// \brief Defines what information should be accumulated over the sub-tests in ezTestEntry::GetSubTestCount.
  enum Enum
  {
    Count,
    Executed,
    Success,
    Errors,
  };
};

/// \brief Stores the results of a test run. Used by both ezTestEntry and ezSubTestEntry.
struct ezTestResultData
{
  ezTestResultData() : m_bExecuted(false), m_bSuccess(false), m_iTestAsserts(0), m_fTestDuration(0.0), m_iFirstOutput(-1), m_iLastOutput(-1) { }
  void Reset();
  void AddOutput(ezInt32 iOutputIndex);

  std::string m_sName;
  bool m_bExecuted;       ///< Whether the test was executed. If false, the test was either deactivated or the test process crashed before executing it.
  bool m_bSuccess;        ///< Whether the test succeeded or not.
  int m_iTestAsserts;     ///< Asserts that were checked. For tests this includes the count of all of their sub-tests as well.
  double m_fTestDuration; ///< Duration of the test/sub-test. For tests, this includes the duration of all their sub-tests as well.
  ezInt32 m_iFirstOutput; ///< First output message. For tests, this range includes all messages of their sub-tests as well.
  ezInt32 m_iLastOutput;  ///< Last output message. For tests, this range includes all messages of their sub-tests as well.
};

struct ezTestConfiguration
{
  ezTestConfiguration();

  ezUInt64 m_uiInstalledMainMemory;
  ezUInt32 m_uiMemoryPageSize;
  ezUInt32 m_uiCPUCoreCount;
  bool m_b64BitOS;
  bool m_b64BitApplication;
  std::string m_sPlatformName;
  std::string m_sBuildConfiguration; ///< Debug, Release, etc
  ezInt64 m_iDateTime;               ///< in seconds since Linux epoch
  ezInt32 m_iRCSRevision;
  std::string m_sHostName;
};

class ezTestFrameworkResult
{
public:
  ezTestFrameworkResult() { }

  // Manage tests
  void Clear();
  void SetupTests(const std::deque<ezTestEntry>& tests, const ezTestConfiguration& config);
  void Reset();
  bool WriteJsonToFile(const char* szAbsFileName) const;

  // Result access
  ezUInt32 GetTestCount(ezTestResultQuery::Enum countQuery = ezTestResultQuery::Count) const;
  ezUInt32 GetSubTestCount(ezUInt32 uiTestIndex, ezTestResultQuery::Enum countQuery = ezTestResultQuery::Count) const;
  ezInt32 GetTestIndexByName(const char* szTestName) const;
  ezInt32 GetSubTestIndexByName(ezUInt32 uiTestIndex, const char* szSubTestName) const;
  double GetTotalTestDuration() const;
  const ezTestResultData& GetTestResultData(ezUInt32 uiTestIndex, ezInt32 iSubTestIndex) const;

  // Test output
  void TestOutput(ezUInt32 uiTestIndex, ezInt32 iSubTestIndex, ezTestOutput::Enum Type, const char* szMsg);
  void TestError(ezUInt32 uiTestIndex, ezInt32 iSubTestIndex,
                 const char* szError, const char* szBlock, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg);
  void TestResult(ezUInt32 uiTestIndex, ezInt32 iSubTestIndex, bool bSuccess, double fDuration);
  void AddAsserts(ezUInt32 uiTestIndex, ezInt32 iSubTestIndex, int iCount);

  // Messages / Errors
  ezUInt32 GetOutputMessageCount(ezInt32 iTestIndex = -1, ezInt32 iSubTestIndex = -1, ezTestOutput::Enum Type = ezTestOutput::AllOutputTypes) const;
  const ezTestOutputMessage* GetOutputMessage(ezUInt32 uiOutputMessageIdx) const;

  ezUInt32 GetErrorMessageCount(ezInt32 iTestIndex = -1, ezInt32 iSubTestIndex = -1) const;
  const ezTestErrorMessage* GetErrorMessage(ezUInt32 uiErrorMessageIdx) const;

private:
  struct ezSubTestResult
  {
    ezSubTestResult() { }
    ezSubTestResult(const char* szName)
    {
      m_Result.m_sName = szName;
    }

    ezTestResultData m_Result;
  };

  struct ezTestResult
  {
    ezTestResult() {}
    ezTestResult(const char* szName)
    {
      m_Result.m_sName = szName;
    }

    void Reset();

    ezTestResultData m_Result;
    std::deque<ezSubTestResult> m_SubTests;
  };

private:
  ezTestConfiguration m_config;
  std::deque<ezTestResult> m_Tests;
  std::deque<ezTestErrorMessage> m_Errors;
  std::deque<ezTestOutputMessage> m_TestOutput;
};


