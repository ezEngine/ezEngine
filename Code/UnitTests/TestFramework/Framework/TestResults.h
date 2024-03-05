#pragma once

#include <TestFramework/Framework/Declarations.h>
#include <TestFramework/TestFrameworkDLL.h>
#include <deque>
#include <string>
#include <vector>

struct EZ_TEST_DLL ezTestOutput
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
    Warning,
    Error,
    ImageDiffFile,
    Duration,
    FinalResult,
    AllOutputTypes
  };

  static const char* const s_Names[];
  static const char* ToString(Enum type);
  static Enum FromString(const char* szName);
};

/// \brief A message of type ezTestOutput::Enum, stored in ezResult.
struct EZ_TEST_DLL ezTestErrorMessage
{
  ezTestErrorMessage() = default;

  std::string m_sError;
  std::string m_sBlock;
  std::string m_sFile;
  ezInt32 m_iLine = -1;
  std::string m_sFunction;
  std::string m_sMessage;
};

/// \brief A message of type ezTestOutput::Enum, stored in ezResult.
struct EZ_TEST_DLL ezTestOutputMessage
{
  ezTestOutputMessage() = default;

  ezTestOutput::Enum m_Type = ezTestOutput::ImportantInfo;
  std::string m_sMessage;
  ezInt32 m_iErrorIndex = -1;
};

struct EZ_TEST_DLL ezTestResultQuery
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
struct EZ_TEST_DLL ezTestResultData
{
  ezTestResultData() = default;

  void Reset();
  void AddOutput(ezInt32 iOutputIndex);

  std::string m_sName;
  bool m_bExecuted = false;     ///< Whether the test was executed. If false, the test was either deactivated or the test process crashed before
                                ///< executing it.
  bool m_bSuccess = false;      ///< Whether the test succeeded or not.
  int m_iTestAsserts = 0;       ///< Asserts that were checked. For tests this includes the count of all of their sub-tests as well.
  double m_fTestDuration = 0.0; ///< Duration of the test/sub-test. For tests, this includes the duration of all their sub-tests as well.
  ezInt32 m_iFirstOutput = -1;  ///< First output message. For tests, this range includes all messages of their sub-tests as well.
  ezInt32 m_iLastOutput = -1;   ///< Last output message. For tests, this range includes all messages of their sub-tests as well.
  std::string m_sCustomStatus;  ///< If this is not empty, the UI will display this instead of "Pending"
};

struct EZ_TEST_DLL ezTestConfiguration
{
  ezTestConfiguration();

  ezUInt64 m_uiInstalledMainMemory = 0;
  ezUInt32 m_uiMemoryPageSize = 0;
  ezUInt32 m_uiCPUCoreCount = 0;
  bool m_b64BitOS = false;
  bool m_b64BitApplication = false;
  std::string m_sPlatformName;
  std::string m_sBuildConfiguration; ///< Debug, Release, etc
  ezInt64 m_iDateTime = 0;           ///< in seconds since Linux epoch
  ezInt32 m_iRCSRevision = -1;
  std::string m_sHostName;
};

class EZ_TEST_DLL ezTestFrameworkResult
{
public:
  ezTestFrameworkResult() = default;

  // Manage tests
  void Clear();
  void SetupTests(const std::deque<ezTestEntry>& tests, const ezTestConfiguration& config);
  void Reset();
  bool WriteJsonToFile(const char* szFileName) const;

  // Result access
  ezUInt32 GetTestCount(ezTestResultQuery::Enum countQuery = ezTestResultQuery::Count) const;
  ezUInt32 GetSubTestCount(ezUInt32 uiTestIndex, ezTestResultQuery::Enum countQuery = ezTestResultQuery::Count) const;
  ezUInt32 GetTestIndexByName(const char* szTestName) const;
  ezUInt32 GetSubTestIndexByName(ezUInt32 uiTestIndex, const char* szSubTestName) const;
  double GetTotalTestDuration() const;
  const ezTestResultData& GetTestResultData(ezUInt32 uiTestIndex, ezUInt32 uiSubTestIndex) const;

  // Test output
  void TestOutput(ezUInt32 uiTestIndex, ezUInt32 uiSubTestIndex, ezTestOutput::Enum type, const char* szMsg);
  void TestError(ezUInt32 uiTestIndex, ezUInt32 uiSubTestIndex, const char* szError, const char* szBlock, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg);
  void TestResult(ezUInt32 uiTestIndex, ezUInt32 uiSubTestIndex, bool bSuccess, double fDuration);
  void AddAsserts(ezUInt32 uiTestIndex, ezUInt32 uiSubTestIndex, int iCount);
  void SetCustomStatus(ezUInt32 uiTestIndex, ezUInt32 uiSubTestIndex, const char* szCustomStatus);

  // Messages / Errors
  ezUInt32 GetOutputMessageCount(ezUInt32 uiTestIndex = ezInvalidIndex, ezUInt32 uiSubTestIndex = ezInvalidIndex, ezTestOutput::Enum type = ezTestOutput::AllOutputTypes) const;
  const ezTestOutputMessage* GetOutputMessage(ezUInt32 uiOutputMessageIdx) const;

  ezUInt32 GetErrorMessageCount(ezUInt32 uiTestIndex = ezInvalidIndex, ezUInt32 uiSubTestIndex = ezInvalidIndex) const;
  const ezTestErrorMessage* GetErrorMessage(ezUInt32 uiErrorMessageIdx) const;

private:
  struct ezSubTestResult
  {
    ezSubTestResult() = default;
    ezSubTestResult(const char* szName) { m_Result.m_sName = szName; }

    ezTestResultData m_Result;
  };

  struct ezTestResult
  {
    ezTestResult() = default;
    ezTestResult(const char* szName) { m_Result.m_sName = szName; }

    void Reset();

    ezTestResultData m_Result;
    std::deque<ezSubTestResult> m_SubTests;
  };

private:
  ezTestConfiguration m_Config;
  std::deque<ezTestResult> m_Tests;
  std::deque<ezTestErrorMessage> m_Errors;
  std::deque<ezTestOutputMessage> m_TestOutput;
};
