#include <TestFramework/PCH.h>
#include <TestFramework/Framework/TestFramework.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/OSFile.h>

ezTestFramework* ezTestFramework::s_pInstance = NULL;
bool ezTestFramework::s_bAssertOnTestFail = true;

const char* ezTestFramework::s_szTestBlockName = "";


////////////////////////////////////////////////////////////////////////
// ezTestFramework public functions
////////////////////////////////////////////////////////////////////////

ezTestFramework::ezTestFramework(const char* szTestName, const char* szAbsTestDir)
  : m_sTestName(szTestName), m_sAbsTestDir(szAbsTestDir), m_iErrorCount(0), m_iTestsFailed(0), m_iTestsPassed(0), m_iCurrentTestIndex(-1), m_iCurrentSubTestIndex(-1), m_bTestsRunning(false)
{
  s_pInstance = this;

  // figure out which tests exist
  GatherAllTests();

  // load the test order from file, if that file does not exist, the array is not modified
  LoadTestOrder();

  // save the current order back to the same file
  SaveTestOrder();
}

ezTestFramework::~ezTestFramework()
{
  s_pInstance = NULL;
}

const char* ezTestFramework::GetTestName() const
{
  return m_sTestName.c_str();
}

const char* ezTestFramework::GetAbsOutputPath() const
{
  return m_sAbsTestDir.c_str();
}

void ezTestFramework::RegisterOutputHandler(OutputHandler Handler) 
{
  // do not register a handler twice
  for (ezUInt32 i = 0; i < m_OutputHandlers.size(); ++i)
  {
    if (m_OutputHandlers[i] == Handler)
      return;
  }

  m_OutputHandlers.push_back(Handler); 
}


void ezTestFramework::GatherAllTests()
{
  m_TestEntries.clear();

  m_iErrorCount = 0;
  m_iTestsFailed = 0;
  m_iTestsPassed = 0;

  // first let all simple tests register themselves
  {
    ezRegisterSimpleTestHelper* pHelper = ezRegisterSimpleTestHelper::GetFirstInstance();

    while (pHelper)
    {
      pHelper->RegisterTest();

      pHelper = pHelper->GetNextInstance();
    }
  }

  ezTestBaseClass* pTestClass = ezTestBaseClass::GetFirstInstance();

  while (pTestClass)
  {
    pTestClass->ClearSubTests();
    pTestClass->SetupSubTests();

    ezTestEntry e;
    e.m_pTest = pTestClass;
    e.m_szTestName = pTestClass->GetTestName();

    for (ezUInt32 i = 0; i < pTestClass->m_Entries.size(); ++i)
    {
      ezSubTestEntry st;
      st.m_szSubTestName = pTestClass->m_Entries[i].m_szName;
      st.m_iSubTestIdentifier = pTestClass->m_Entries[i].m_iIdentifier;

      e.m_SubTests.push_back(st);
    }

    m_TestEntries.push_back(e);

    pTestClass = pTestClass->GetNextInstance();
  }
  ::SortTestsAlphabetically(m_TestEntries);
}

void ezTestFramework::LoadTestOrder()
{
  std::string sTestSettingsFile = m_sAbsTestDir + std::string("/TestSettings.txt");
  ::LoadTestOrder(sTestSettingsFile.c_str(), m_TestEntries, m_Settings);
}

void ezTestFramework::CreateOutputFolder()
{
  ezStartup::StartupBase();
  ezOSFile::CreateDirectoryStructure(m_sAbsTestDir.c_str());
  ezStartup::ShutdownBase();
}

void ezTestFramework::SaveTestOrder()
{
  CreateOutputFolder();

  std::string sTestSettingsFile = m_sAbsTestDir + std::string("/TestSettings.txt");
  ::SaveTestOrder(sTestSettingsFile.c_str(), m_TestEntries, m_Settings);
}

void ezTestFramework::SetAllTestsEnabledStatus(bool bEnable)
{
  const ezUInt32 uiTestCount = GetTestCount();
  for (ezUInt32 uiTestIdx = 0; uiTestIdx < uiTestCount; ++uiTestIdx)
  {
    m_TestEntries[uiTestIdx].m_Result.m_bEnableTest = bEnable;
    const ezUInt32 uiSubTestCount = (ezUInt32)m_TestEntries[uiTestIdx].m_SubTests.size();
    for (ezUInt32 uiSubTest = 0; uiSubTest < uiSubTestCount; ++uiSubTest)
    {
      m_TestEntries[uiTestIdx].m_SubTests[uiSubTest].m_Result.m_bEnableTest = bEnable;
    }
  }
}

void ezTestFramework::ResetTests()
{
  m_iErrorCount = 0;
  m_iTestsFailed = 0;
  m_iTestsPassed = 0;

  const ezUInt32 uiTestCount = GetTestCount();
  for (ezUInt32 uiTestIdx = 0; uiTestIdx < uiTestCount; ++uiTestIdx)
  {
    m_TestEntries[uiTestIdx].m_Result.Reset();
    const ezUInt32 uiSubTestCount = (ezUInt32)m_TestEntries[uiTestIdx].m_SubTests.size();
    for (ezUInt32 uiSubTest = 0; uiSubTest < uiSubTestCount; ++uiSubTest)
    {
      m_TestEntries[uiTestIdx].m_SubTests[uiSubTest].m_Result.Reset();
    }
  }
}

void ezTestFramework::ExecuteAllTests()
{
  StartTests();

  for (ezUInt32 i = 0; i < m_TestEntries.size(); ++i)
  {
    ExecuteTest(i);
  }

  EndTests();
}

void ezTestFramework::StartTests()
{
  ResetTests();
  m_bTestsRunning = true;
  // As this variable is hit a lot during tests we cache it in the static variable
  // so we don't have to go through the lock at every assert.
  s_bAssertOnTestFail = m_Settings.m_bAssertOnTestFail;
  
  ezTestFramework::Output(ezTestOutput::StartOutput, "");
}

// Redirects engine warnings / errors to testframework output
static void LogWriter(const ezLoggingEventData& e)
{
  switch (e.m_EventType)
  {
  case ezLogMsgType::ErrorMsg:
    ezTestFramework::Output(ezTestOutput::Error, "ezLog Error: %s", e.m_szText);
    break;
  case ezLogMsgType::SeriousWarningMsg:
    ezTestFramework::Output(ezTestOutput::Error, "ezLog Serious Warning: %s", e.m_szText);
    break;
  case ezLogMsgType::WarningMsg:
    ezTestFramework::Output(ezTestOutput::ImportantInfo, "ezLog Warning: %s", e.m_szText);
    break;
  case ezLogMsgType::InfoMsg:
  case ezLogMsgType::DevMsg:
  case ezLogMsgType::DebugMsg:
    {
      if (ezStringUtils::IsEqual_NoCase(e.m_szTag, "test"))
        ezTestFramework::Output(ezTestOutput::Details, e.m_szText);
    }
    break;

  default:
    return;
  }
}

void ezTestFramework::ExecuteTest(ezUInt32 uiTestIndex)
{
  if (uiTestIndex >= GetTestCount())
    return;

  if (!m_TestEntries[uiTestIndex].m_Result.m_bEnableTest)
    return;

  ezTestEntry& TestEntry = m_TestEntries[uiTestIndex];
  ezTestBaseClass* pTestClass = m_TestEntries[uiTestIndex].m_pTest;

  // Execute test
  {
    m_iCurrentTestIndex = (ezInt32)uiTestIndex;
    // Log writer translates engine warnings / errors into test framework error messages.
    ezGlobalLog::AddLogWriter(LogWriter);

    const ezInt32 iTestErrorCount = GetTotalErrorCount();

    ezTestFramework::Output(ezTestOutput::BeginBlock, "Executing Test: '%s'", TestEntry.m_szTestName);
  
    double fTotalTestDuration = 0.0f;
    // *** Test Initialization ***
    if (pTestClass->DoTestInitialization() == EZ_SUCCESS)
    {    
      for (ezUInt32 st = 0; st < TestEntry.m_SubTests.size(); ++st)
      {
        ezSubTestEntry& subTest = TestEntry.m_SubTests[st];
        ezInt32 iSubTestIdentifier = subTest.m_iSubTestIdentifier;
        subTest.m_Result.m_fTestDuration = 0.0;

        if (!subTest.m_Result.m_bEnableTest)
        {
          //ezTestFramework::Output(ezTestOutput::Message, "Skipping deactivated Sub-Test: '%s'", subTest.m_szSubTestName);
          continue;
        }
    
        m_iCurrentSubTestIndex = st;
        ezTestFramework::Output(ezTestOutput::BeginBlock, "Executing Sub-Test: '%s'", subTest.m_szSubTestName);
    
        // *** Sub-Test Initialization ***
        if (pTestClass->DoSubTestInitialization(iSubTestIdentifier) == EZ_SUCCESS)
        {
          // *** Run Sub-Test ***
          const double fDuration = pTestClass->DoSubTestRun(iSubTestIdentifier);

          // *** Sub-Test De-Initialization ***
          pTestClass->DoSubTestDeInitialization(iSubTestIdentifier);

          bool bSubTestSuccess = subTest.m_Result.m_uiErrorCount == 0;
          ezTestFramework::TestResult(iSubTestIdentifier, bSubTestSuccess, fDuration);
       
          fTotalTestDuration += fDuration;
        }

        ezTestFramework::Output(ezTestOutput::EndBlock, "");
        m_iCurrentSubTestIndex = -1;
      }
    }

    // *** Test De-Initialization ***
    pTestClass->DoTestDeInitialization();

    ezGlobalLog::RemoveLogWriter(LogWriter);

    bool bTestSuccess = iTestErrorCount == GetTotalErrorCount();
    ezTestFramework::TestResult(-1, bTestSuccess, fTotalTestDuration);
    ezTestFramework::Output(ezTestOutput::EndBlock, "");
    m_iCurrentTestIndex = -1;
  } 
}

void ezTestFramework::EndTests()
{
  m_bTestsRunning = false;
  if (GetTestsFailedCount() == 0)
    ezTestFramework::Output(ezTestOutput::FinalResult, "All tests passed.");
  else
    ezTestFramework::Output(ezTestOutput::FinalResult, "Tests failed: %i. Tests passed: %i", GetTestsFailedCount(), GetTestsPassedCount());
}

ezUInt32 ezTestFramework::GetTestCount(ezTestResultQuery::Enum countQuery) const
{
  ezUInt32 uiAccumulator = 0;
  const ezUInt32 uiTests = (ezUInt32)m_TestEntries.size();
  if (countQuery == ezTestResultQuery::Count)
    return uiTests;

  for (ezUInt32 uiTest = 0; uiTest < uiTests; ++uiTest)
  {
    switch (countQuery)
    {
    case ezTestResultQuery::Enabled:
      uiAccumulator += m_TestEntries[uiTest].m_Result.m_bEnableTest ? 1 : 0;
      break;
    case ezTestResultQuery::Executed:
      uiAccumulator += m_TestEntries[uiTest].m_Result.m_bExecuted ? 1 : 0;
      break;
    case ezTestResultQuery::Success:
      uiAccumulator += m_TestEntries[uiTest].m_Result.m_bSuccess ? 1 : 0;
      break;
    case ezTestResultQuery::Errors:
      uiAccumulator += m_TestEntries[uiTest].m_Result.m_uiErrorCount;
      break;
    default:
      break;
    }
  }
  return uiAccumulator;
}

ezInt32 ezTestFramework::SubTestIdentifierToSubTestIndex(ezUInt32 uiSubTestIdentifier) const
{
  const int iSubTestCount = (int)m_TestEntries[m_iCurrentTestIndex].m_SubTests.size();
  for (int iSubTest = 0; iSubTest < iSubTestCount; ++iSubTest)
  {
    if (m_TestEntries[m_iCurrentTestIndex].m_SubTests[iSubTest].m_iSubTestIdentifier == uiSubTestIdentifier)
    {
      return iSubTest;
    }
  }
  return -1;
}

ezTestEntry* ezTestFramework::GetTest(ezUInt32 uiTestIndex)
{
  if (uiTestIndex >= GetTestCount())
    return NULL;

  return &m_TestEntries[uiTestIndex];
}

TestSettings ezTestFramework::GetSettings() const
{
  return m_Settings;
}

void ezTestFramework::SetSettings(const TestSettings& settings)
{
  m_Settings = settings;
}

ezInt32 ezTestFramework::GetTotalErrorCount() const
{
  return m_iErrorCount;
}

ezInt32 ezTestFramework::GetTestsPassedCount() const
{
  return m_iTestsPassed;
}

ezInt32 ezTestFramework::GetTestsFailedCount() const
{
  return m_iTestsFailed; 
}

double ezTestFramework::GetTotalTestDuration() const
{
  double fTotalTestDuration = 0.0;
  const ezUInt32 uiTests = (ezUInt32)m_TestEntries.size();
  for (ezUInt32 uiTest = 0; uiTest < uiTests; ++uiTest)
  {
    fTotalTestDuration += m_TestEntries[uiTest].m_Result.m_fTestDuration;
  }
  return fTotalTestDuration;
}

////////////////////////////////////////////////////////////////////////
// ezTestFramework protected functions
////////////////////////////////////////////////////////////////////////

void ezTestFramework::OutputImpl(ezTestOutput::Enum Type, const char* szMsg)
{
  // pass the output to all the registered output handlers, which will then write it to the console, file, etc.
  for (ezUInt32 i = 0; i < m_OutputHandlers.size(); ++i)
  {
    m_OutputHandlers[i](Type, szMsg);
  }

  if (m_iCurrentTestIndex == -1)
    return;

  ezTestResult& Result = (m_iCurrentSubTestIndex == -1) ? m_TestEntries[m_iCurrentTestIndex].m_Result : m_TestEntries[m_iCurrentTestIndex].m_SubTests[m_iCurrentSubTestIndex].m_Result;

  switch (Type)
  {
  case ezTestOutput::Error:
    {
      Result.m_uiErrorCount++;
      m_iErrorCount++;
      break;
    }
  default:
    break;
  }

  if (m_iCurrentTestIndex != -1)
  {
    Result.m_TestOutput.push_back(ezTestOutputMessage());
    Result.m_TestOutput.rbegin()->m_Type = Type;
    Result.m_TestOutput.rbegin()->m_sMessage.assign(szMsg);
  }
}

void ezTestFramework::TestResultImpl(ezInt32 iSubTestIdentifier, bool bSuccess, double fDuration)
{
  ezTestResult& Result = (iSubTestIdentifier == -1) ? m_TestEntries[m_iCurrentTestIndex].m_Result : m_TestEntries[m_iCurrentTestIndex].m_SubTests[SubTestIdentifierToSubTestIndex(iSubTestIdentifier)].m_Result;

  Result.m_bExecuted = true;
  Result.m_bSuccess = bSuccess;
  Result.m_fTestDuration = fDuration;

  const ezUInt32 uiMin = (ezUInt32) (fDuration / 1000.0 / 60.0);
  const ezUInt32 uiSec = (ezUInt32) (fDuration / 1000.0 - uiMin * 60.0);
  const ezUInt32 uiMS  = (ezUInt32) (fDuration - uiSec * 1000.0);

  ezTestFramework::Output(ezTestOutput::Duration, "%i:%02i:%03i", uiMin, uiSec, uiMS);

  if (iSubTestIdentifier == -1)
  {
    const char* szTestName = m_TestEntries[m_iCurrentTestIndex].m_szTestName;
    if (bSuccess)
    {
      m_iTestsPassed++;
      ezTestFramework::Output(ezTestOutput::Success, "Test '%s' succeeded", szTestName);
    }
    else
    {
      m_iTestsFailed++;
      ezTestFramework::Output(ezTestOutput::Error, "Test '%s' failed: %i Errors.", szTestName, Result.m_uiErrorCount);
    }
  }
  else
  {
    // Accumulate sub-test duration onto test duration to get duration feedback while the sub-tests are running.
    // Final time will be set again once the entire test finishes and currently these times are identical as
    // init and de-init times aren't measured at the moment due to missing timer when engine is shut down.
    m_TestEntries[m_iCurrentTestIndex].m_Result.m_fTestDuration += fDuration;

    const char* szSubTestName = m_TestEntries[m_iCurrentTestIndex].m_SubTests[SubTestIdentifierToSubTestIndex(iSubTestIdentifier)].m_szSubTestName;
    if (bSuccess)
    {
      ezTestFramework::Output(ezTestOutput::Success, "Sub-Test '%s' succeeded.", szSubTestName);
    }
    else
    {
      ezTestFramework::Output(ezTestOutput::Error, "Sub-Test '%s' failed: %i Errors.", szSubTestName, Result.m_uiErrorCount);
    }
  }
}


////////////////////////////////////////////////////////////////////////
// ezTestFramework static functions
////////////////////////////////////////////////////////////////////////

void ezTestFramework::Output(ezTestOutput::Enum Type, const char* szMsg, ...)
{
  // format the output text
  va_list args;
  va_start (args, szMsg);

  char szBuffer[1024 * 10];
  vsprintf (szBuffer, szMsg, args);
  va_end (args);

  GetInstance()->OutputImpl(Type, szBuffer);
}

void ezTestFramework::TestResult(ezInt32 iSubTestIdentifier, bool bSuccess, double fDuration)
{
  GetInstance()->TestResultImpl(iSubTestIdentifier, bSuccess, fDuration);
}



EZ_STATICLINK_REFPOINT(TestFramework_Framework_TestFramework);

