#include <TestFramework/PCH.h>
#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestOrder.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Strings/StringBuilder.h>
#include <CoreUtils/Image/Image.h>
#include <CoreUtils/Image/ImageUtils.h>
#include <CoreUtils/Image/ImageConversion.h>

ezTestFramework* ezTestFramework::s_pInstance = nullptr;

const char* ezTestFramework::s_szTestBlockName = "";
int ezTestFramework::s_iAssertCounter = 0;

static bool TestAssertHandler(const char* szSourceFile, ezUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  ezTestFramework::Error(szExpression, szSourceFile, (ezInt32)uiLine, szFunction, szAssertMsg);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  // if a debugger is attached, one typically always wants to know about asserts
  if (IsDebuggerPresent())
    return true;
#endif

  return ezTestFramework::GetAssertOnTestFail();
}

////////////////////////////////////////////////////////////////////////
// ezTestFramework public functions
////////////////////////////////////////////////////////////////////////

ezTestFramework::ezTestFramework(const char* szTestName, const char* szAbsTestDir, int argc, const char** argv)
  : m_sTestName(szTestName), m_sAbsTestDir(szAbsTestDir), m_iErrorCount(0), m_iTestsFailed(0), m_iTestsPassed(0), m_PreviousAssertHandler(nullptr), m_iCurrentTestIndex(-1), m_iCurrentSubTestIndex(-1), m_bTestsRunning(false)
{
  s_pInstance = this;

  GetTestSettingsFromCommandLine(argc, argv);

  // figure out which tests exist
  GatherAllTests();

  // load the test order from file, if that file does not exist, the array is not modified
  LoadTestOrder();

  // save the current order back to the same file
  SaveTestOrder();
}

ezTestFramework::~ezTestFramework()
{
  s_pInstance = nullptr;
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

bool ezTestFramework::GetAssertOnTestFail()
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  if (!IsDebuggerPresent())
    return false;
#endif

  return s_pInstance->m_Settings.m_bAssertOnTestFail;
}

void ezTestFramework::GatherAllTests()
{
  m_TestEntries.clear();

  m_iErrorCount = 0;
  m_iTestsFailed = 0;
  m_iTestsPassed = 0;
  m_iExecutingTest = -1;
  m_iExecutingSubTest = -1;
  m_bSubTestInitialized = false;

  // first let all simple tests register themselves
  {
    ezRegisterSimpleTestHelper* pHelper = ezRegisterSimpleTestHelper::GetFirstInstance();

    while (pHelper)
    {
      pHelper->RegisterTest();

      pHelper = pHelper->GetNextInstance();
    }
  }

  ezTestConfiguration config;
  ezTestBaseClass* pTestClass = ezTestBaseClass::GetFirstInstance();

  while (pTestClass)
  {
    pTestClass->ClearSubTests();
    pTestClass->SetupSubTests();
    pTestClass->UpdateConfiguration(config);

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

  m_Result.SetupTests(m_TestEntries, config);
}

void ezTestFramework::GetTestSettingsFromCommandLine(int argc, const char** argv)
{
  ezStartup::StartupBase();
  {
    ezCommandLineUtils cmd;
    cmd.SetCommandLine(argc, argv);

    m_Settings.m_bRunTests         = cmd.GetBoolOption("-run", false);
    m_Settings.m_bNoSaving         = cmd.GetBoolOption("-nosave", false);
    m_Settings.m_bCloseOnSuccess   = cmd.GetBoolOption("-close", false);
    m_Settings.m_bNoGUI            = cmd.GetBoolOption("-nogui", false);

    m_Settings.m_bAssertOnTestFail = cmd.GetBoolOption("-assert", m_Settings.m_bAssertOnTestFail);
    m_Settings.m_bOpenHtmlOutput   = cmd.GetBoolOption("-html", m_Settings.m_bOpenHtmlOutput);
    m_Settings.m_bKeepConsoleOpen  = cmd.GetBoolOption("-console", m_Settings.m_bKeepConsoleOpen);
    m_Settings.m_bShowMessageBox   = cmd.GetBoolOption("-msgbox", m_Settings.m_bShowMessageBox);
    m_Settings.m_iRevision         = cmd.GetIntOption("-rev", -1);
    m_Settings.m_bEnableAllTests   = cmd.GetBoolOption("-all", false);
    m_Settings.m_uiFullPasses      = cmd.GetIntOption("-passes", 1, false);
    if (cmd.GetStringOptionArguments("-json") == 1)
      m_Settings.m_sJsonOutput     = cmd.GetStringOption("-json", 0, "");

    m_uiPassesLeft = m_Settings.m_uiFullPasses;
  }
}

void ezTestFramework::LoadTestOrder()
{
  std::string sTestSettingsFile = m_sAbsTestDir + std::string("/TestSettings.txt");
  ::LoadTestOrder(sTestSettingsFile.c_str(), m_TestEntries, m_Settings);
  if (m_Settings.m_bEnableAllTests)
    SetAllTestsEnabledStatus(true);
}

void ezTestFramework::CreateOutputFolder()
{
  ezStartup::StartupBase();
  ezOSFile::CreateDirectoryStructure(m_sAbsTestDir.c_str());
}

void ezTestFramework::SaveTestOrder()
{
  if (m_Settings.m_bNoSaving)
    return;

  CreateOutputFolder();

  std::string sTestSettingsFile = m_sAbsTestDir + std::string("/TestSettings.txt");
  ::SaveTestOrder(sTestSettingsFile.c_str(), m_TestEntries, m_Settings);
}

void ezTestFramework::SetAllTestsEnabledStatus(bool bEnable)
{
  const ezUInt32 uiTestCount = GetTestCount();
  for (ezUInt32 uiTestIdx = 0; uiTestIdx < uiTestCount; ++uiTestIdx)
  {
    m_TestEntries[uiTestIdx].m_bEnableTest = bEnable;
    const ezUInt32 uiSubTestCount = (ezUInt32)m_TestEntries[uiTestIdx].m_SubTests.size();
    for (ezUInt32 uiSubTest = 0; uiSubTest < uiSubTestCount; ++uiSubTest)
    {
      m_TestEntries[uiTestIdx].m_SubTests[uiSubTest].m_bEnableTest = bEnable;
    }
  }
}

void ezTestFramework::ResetTests()
{
  m_iErrorCount = 0;
  m_iTestsFailed = 0;
  m_iTestsPassed = 0;
  m_iExecutingTest = -1;
  m_iExecutingSubTest = -1;
  m_bSubTestInitialized = false;
  m_bAbortTests = false;

  m_Result.Reset();
}

ezTestAppRun ezTestFramework::RunTestExecutionLoop()
{
  if (m_iExecutingTest < 0)
  {
    StartTests();
    m_iExecutingTest = 0;
    EZ_ASSERT_DEV(m_iExecutingSubTest == -1, "Invalid test framework state");
    EZ_ASSERT_DEV(!m_bSubTestInitialized, "Invalid test framework state");
  }

  ExecuteNextTest();

  if (m_iExecutingTest >= (ezInt32) m_TestEntries.size())
  {
    EndTests();

    if (m_uiPassesLeft > 1 && !m_bAbortTests)
    {
      --m_uiPassesLeft;

      m_iExecutingTest = -1;
      m_iExecutingSubTest = -1;

      return ezTestAppRun::Continue;
    }

    return ezTestAppRun::Quit;
  }

  return ezTestAppRun::Continue;
}

void ezTestFramework::StartTests()
{
  ResetTests();
  m_bTestsRunning = true;

  ezTestFramework::Output(ezTestOutput::StartOutput, "");

  m_PreviousAssertHandler = ezGetAssertHandler();

  ezSetAssertHandler(TestAssertHandler);
}

// Redirects engine warnings / errors to test-framework output
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

void ezTestFramework::ExecuteNextTest()
{
  EZ_ASSERT_DEV(m_iExecutingTest >= 0, "Invalid current test.");
  EZ_ASSERT_DEV(m_iExecutingTest < (ezInt32) GetTestCount(), "Invalid current test.");

  if (!m_TestEntries[m_iExecutingTest].m_bEnableTest)
  {
    // next time run the next test and start with the first subtest
    m_iExecutingTest++;
    m_iExecutingSubTest = -1;
    return;
  }

  ezTestEntry& TestEntry = m_TestEntries[m_iExecutingTest];
  ezTestBaseClass* pTestClass = m_TestEntries[m_iExecutingTest].m_pTest;

  // Execute test
  {
    if (m_iExecutingSubTest == -1) // no subtest has run yet, so initialize the test first
    {
      if (m_bAbortTests)
      {
        m_iExecutingTest = (ezInt32) m_TestEntries.size(); // skip to the end of all tests
        m_iExecutingSubTest = -1;
        return;
      }

      m_iExecutingSubTest = 0;
      m_fTotalTestDuration = 0.0;

      // Reset assert counter. This variable is used to reduce the overhead of counting millions of asserts.
      s_iAssertCounter = 0;
      m_iCurrentTestIndex = m_iExecutingTest;
      // Log writer translates engine warnings / errors into test framework error messages.
      ezGlobalLog::AddLogWriter(LogWriter);

      m_iErrorCountBeforeTest = GetTotalErrorCount();

      ezTestFramework::Output(ezTestOutput::BeginBlock, "Executing Test: '%s'", TestEntry.m_szTestName);

      // *** Test Initialization ***
      if (pTestClass->DoTestInitialization().Failed())
      {
        m_iExecutingSubTest = (ezInt32) TestEntry.m_SubTests.size(); // make sure all subtests are skipped
      }
    }

    if (m_iExecutingSubTest < (ezInt32) TestEntry.m_SubTests.size())
    {
      ezSubTestEntry& subTest = TestEntry.m_SubTests[m_iExecutingSubTest];
      ezInt32 iSubTestIdentifier = subTest.m_iSubTestIdentifier;

      if (!subTest.m_bEnableTest)
      {
        /// \todo Don't we want to output this ?
        //ezTestFramework::Output(ezTestOutput::Message, "Skipping deactivated Sub-Test: '%s'", subTest.m_szSubTestName);
        ++m_iExecutingSubTest;
        return;
      }

      if (!m_bSubTestInitialized)
      {
        if (m_bAbortTests)
        {
          // tests shall be aborted, so do not start a new one

          m_iExecutingTest = (ezInt32) m_TestEntries.size(); // skip to the end of all tests
          m_iExecutingSubTest = -1;
          return;
        }

        m_fTotalSubTestDuration = 0.0;
        m_iImageCounter = 0;

        // First flush of assert counter, these are all asserts during test init.
        FlushAsserts();
        m_iCurrentSubTestIndex = m_iExecutingSubTest;
        ezTestFramework::Output(ezTestOutput::BeginBlock, "Executing Sub-Test: '%s'", subTest.m_szSubTestName);

        // *** Sub-Test Initialization ***
        m_bSubTestInitialized = pTestClass->DoSubTestInitialization(iSubTestIdentifier).Succeeded();
      }

      ezTestAppRun subTestResult = ezTestAppRun::Quit;

      if (m_bSubTestInitialized)
      {
        // *** Run Sub-Test ***
        double fDuration = 0.0;
        subTestResult = pTestClass->DoSubTestRun(iSubTestIdentifier, fDuration);

        // I guess we can require that tests are written in a way that they can be interrupted
        if (m_bAbortTests)
          subTestResult = ezTestAppRun::Quit;

        m_fTotalSubTestDuration += fDuration;
      }

      // this is executed when sub-test initialization failed or the sub-test reached its end
      if (subTestResult == ezTestAppRun::Quit)
      {
        // *** Sub-Test De-Initialization ***
        pTestClass->DoSubTestDeInitialization(iSubTestIdentifier);

        bool bSubTestSuccess = m_bSubTestInitialized && (m_Result.GetErrorMessageCount(m_iExecutingTest, m_iExecutingSubTest) == 0);
        ezTestFramework::TestResult(m_iExecutingSubTest, bSubTestSuccess, m_fTotalSubTestDuration);

        m_fTotalTestDuration += m_fTotalSubTestDuration;

        // advance to the next (sub) test
        m_bSubTestInitialized = false;
        ++m_iExecutingSubTest;

        // Second flush of assert counter, these are all asserts for the current subtest.
        FlushAsserts();
        ezTestFramework::Output(ezTestOutput::EndBlock, "");
        m_iCurrentSubTestIndex = -1;
      }
    }

    if (m_iExecutingSubTest >= (ezInt32) TestEntry.m_SubTests.size())
    {
      // *** Test De-Initialization ***
      pTestClass->DoTestDeInitialization();
      // Third and last flush of assert counter, these are all asserts for the test de-init.
      FlushAsserts();

      ezGlobalLog::RemoveLogWriter(LogWriter);

      bool bTestSuccess = m_iErrorCountBeforeTest == GetTotalErrorCount();
      ezTestFramework::TestResult(-1, bTestSuccess, m_fTotalTestDuration);
      ezTestFramework::Output(ezTestOutput::EndBlock, "");
      m_iCurrentTestIndex = -1;

      // advance to the next test
      m_iExecutingTest++;
      m_iExecutingSubTest = -1;
    }
  }
}

void ezTestFramework::EndTests()
{
  ezSetAssertHandler(m_PreviousAssertHandler);
  m_PreviousAssertHandler = nullptr;

  m_bTestsRunning = false;
  if (GetTestsFailedCount() == 0)
    ezTestFramework::Output(ezTestOutput::FinalResult, "All tests passed.");
  else
    ezTestFramework::Output(ezTestOutput::FinalResult, "Tests failed: %i. Tests passed: %i", GetTestsFailedCount(), GetTestsPassedCount());

  if (!m_Settings.m_sJsonOutput.empty())
    m_Result.WriteJsonToFile(m_Settings.m_sJsonOutput.c_str());

  m_iExecutingTest = -1;
  m_iExecutingSubTest = -1;
  m_bAbortTests = false;
}

void ezTestFramework::AbortTests()
{
  m_bAbortTests = true;
}

ezUInt32 ezTestFramework::GetTestCount() const
{
  return (ezUInt32)m_TestEntries.size();
}

ezUInt32 ezTestFramework::GetTestEnabledCount() const
{
  ezUInt32 uiEnabledCount = 0;
  const ezUInt32 uiTests = GetTestCount();
  for (ezUInt32 uiTest = 0; uiTest < uiTests; ++uiTest)
  {
    uiEnabledCount += m_TestEntries[uiTest].m_bEnableTest ? 1 : 0;
  }
  return uiEnabledCount;
}

ezUInt32 ezTestFramework::GetSubTestEnabledCount(ezUInt32 uiTestIndex) const
{
  if (uiTestIndex >= GetTestCount())
    return 0;

  ezUInt32 uiEnabledCount = 0;
  const ezUInt32 uiSubTests = (ezUInt32)m_TestEntries[uiTestIndex].m_SubTests.size();
  for (ezUInt32 uiSubTest = 0; uiSubTest < uiSubTests; ++uiSubTest)
  {
    uiEnabledCount += m_TestEntries[uiTestIndex].m_SubTests[uiSubTest].m_bEnableTest ? 1 : 0;
  }
  return uiEnabledCount;
}

bool ezTestFramework::IsTestEnabled(ezUInt32 uiTestIndex) const
{
  if (uiTestIndex >= GetTestCount())
    return false;

  return m_TestEntries[uiTestIndex].m_bEnableTest;
}

bool ezTestFramework::IsSubTestEnabled(ezUInt32 uiTestIndex, ezUInt32 uiSubTestIndex) const
{
  if (uiTestIndex >= GetTestCount())
    return false;

  const ezUInt32 uiSubTests = (ezUInt32)m_TestEntries[uiTestIndex].m_SubTests.size();
  if (uiSubTestIndex >= uiSubTests)
    return false;

  return m_TestEntries[uiTestIndex].m_SubTests[uiSubTestIndex].m_bEnableTest;
}

void ezTestFramework::SetTestEnabled(ezUInt32 uiTestIndex, bool bEnabled)
{
  if (uiTestIndex >= GetTestCount())
    return;

  m_TestEntries[uiTestIndex].m_bEnableTest = bEnabled;
}

void ezTestFramework::SetSubTestEnabled(ezUInt32 uiTestIndex, ezUInt32 uiSubTestIndex, bool bEnabled)
{
  if (uiTestIndex >= GetTestCount())
    return;

  const ezUInt32 uiSubTests = (ezUInt32)m_TestEntries[uiTestIndex].m_SubTests.size();
  if (uiSubTestIndex >= uiSubTests)
    return;

  m_TestEntries[uiTestIndex].m_SubTests[uiSubTestIndex].m_bEnableTest = bEnabled;
}

ezTestEntry* ezTestFramework::GetTest(ezUInt32 uiTestIndex)
{
  if (uiTestIndex >= GetTestCount())
    return nullptr;

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

ezTestFrameworkResult& ezTestFramework::GetTestResult()
{
  return m_Result;
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
  return m_Result.GetTotalTestDuration();
}

////////////////////////////////////////////////////////////////////////
// ezTestFramework protected functions
////////////////////////////////////////////////////////////////////////

static bool g_bBlockOutput = false;

void ezTestFramework::OutputImpl(ezTestOutput::Enum Type, const char* szMsg)
{
  // pass the output to all the registered output handlers, which will then write it to the console, file, etc.
  for (ezUInt32 i = 0; i < m_OutputHandlers.size(); ++i)
  {
    m_OutputHandlers[i](Type, szMsg);
  }

  if (g_bBlockOutput)
    return;

  m_Result.TestOutput(m_iCurrentTestIndex, m_iCurrentSubTestIndex, Type, szMsg);
}

void ezTestFramework::ErrorImpl(const char* szError, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg)
{
  m_Result.TestError(m_iCurrentTestIndex, m_iCurrentSubTestIndex, szError, ezTestFramework::s_szTestBlockName, szFile, iLine, szFunction, szMsg);

  g_bBlockOutput = true;
  ezTestFramework::Output(ezTestOutput::Error, szError);
  ezTestFramework::Output(ezTestOutput::BeginBlock, "");
  {
    if ((ezTestFramework::s_szTestBlockName != nullptr) && (ezTestFramework::s_szTestBlockName[0] != '\0'))
      ezTestFramework::Output(ezTestOutput::Message, "Block: '%s'", ezTestFramework::s_szTestBlockName);

    ezTestFramework::Output(ezTestOutput::ImportantInfo, "File: %s", szFile);
    ezTestFramework::Output(ezTestOutput::ImportantInfo, "Line: %i", iLine);
    ezTestFramework::Output(ezTestOutput::ImportantInfo, "Function: %s", szFunction);

    if ((szMsg != nullptr) && (szMsg[0] != '\0'))
      ezTestFramework::Output(ezTestOutput::Message, "Message: %s", szMsg);
  }
  ezTestFramework::Output(ezTestOutput::EndBlock, "");
  g_bBlockOutput = false;

  m_iErrorCount++;
}

void ezTestFramework::TestResultImpl(ezInt32 iSubTestIndex, bool bSuccess, double fDuration)
{
  m_Result.TestResult(m_iCurrentTestIndex, iSubTestIndex, bSuccess, fDuration);

  const ezUInt32 uiMin = (ezUInt32) (fDuration / 1000.0 / 60.0);
  const ezUInt32 uiSec = (ezUInt32) (fDuration / 1000.0 - uiMin * 60.0);
  const ezUInt32 uiMS  = (ezUInt32) (fDuration - uiSec * 1000.0);

  ezTestFramework::Output(ezTestOutput::Duration, "%i:%02i:%03i", uiMin, uiSec, uiMS);

  if (iSubTestIndex == -1)
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
      ezTestFramework::Output(ezTestOutput::Error, "Test '%s' failed: %i Errors.", szTestName, (ezUInt32)m_Result.GetErrorMessageCount(m_iCurrentTestIndex, iSubTestIndex));
    }
  }
  else
  {
    const char* szSubTestName = m_TestEntries[m_iCurrentTestIndex].m_SubTests[iSubTestIndex].m_szSubTestName;
    if (bSuccess)
    {
      ezTestFramework::Output(ezTestOutput::Success, "Sub-Test '%s' succeeded.", szSubTestName);
    }
    else
    {
      ezTestFramework::Output(ezTestOutput::Error, "Sub-Test '%s' failed: %i Errors.", szSubTestName, (ezUInt32)m_Result.GetErrorMessageCount(m_iCurrentTestIndex, iSubTestIndex));
    }
  }
}

void ezTestFramework::FlushAsserts()
{
  m_Result.AddAsserts(m_iCurrentTestIndex, m_iCurrentSubTestIndex, s_iAssertCounter);
  s_iAssertCounter = 0;
}


bool ezTestFramework::CompareImages(ezUInt32 uiMaxError, char* szErrorMsg)
{
  const char* szTestName = GetTest(GetCurrentTestIndex())->m_szTestName;
  const char* szSubTestName = GetTest(GetCurrentTestIndex())->m_SubTests[GetCurrentSubTestIndex()].m_szSubTestName;

  ezStringBuilder sImgName, sImgPathReference, sImgPathResult;
  sImgName.Format("%s_%s_%03i", szTestName, szSubTestName, m_iImageCounter);
  ++m_iImageCounter;

  ezImage img, imgRGB, imgSmall;
  if (GetTest(GetCurrentTestIndex())->m_pTest->GetImage(img).Failed())
  {
    safeprintf(szErrorMsg, 512, "Image '%s' could not be captured", sImgName.GetData());
    return false;
  }

  if (ezImageConversionBase::Convert(img, imgRGB, ezImageFormat::B8G8R8_UNORM).Failed())
  {
    safeprintf(szErrorMsg, 512, "Captured Image '%s' could not be converted to BGR8", sImgName.GetData());
    return false;
  }

  ezImageUtils::ScaleDownHalf(imgRGB, imgSmall);

  sImgPathReference.Format("Images_Reference/%s.tga", sImgName.GetData());
  sImgPathResult.Format("Images_Result/%s.tga", sImgName.GetData());

  ezImage imgExp, imgExpRGB;
  if (imgExp.LoadFrom(sImgPathReference).Failed())
  {
    imgSmall.SaveTo(sImgPathResult);

    safeprintf(szErrorMsg, 512, "Comparison Image '%s' could not be read", sImgPathReference.GetData());
    return false;
  }

  if (ezImageConversionBase::Convert(imgExp, imgExpRGB, ezImageFormat::B8G8R8_UNORM).Failed())
  {
    imgSmall.SaveTo(sImgPathResult);

    safeprintf(szErrorMsg, 512, "Comparison Image '%s' could not be converted to BGR8", sImgPathReference.GetData());
    return false;
  }

  ezImage imgDiff;
  ezImageUtils::ComputeImageDifferenceABS(imgExpRGB, imgSmall, imgDiff);

  const ezUInt32 uiMeanError = ezImageUtils::ComputeMeanSquareError(imgDiff, 32);

  if (uiMeanError > uiMaxError)
  {
    imgSmall.SaveTo(sImgPathResult);

    ezStringBuilder sImgDiffName;
    sImgDiffName.Format("Images_Diff/%s.tga", sImgName.GetData());

    imgDiff.SaveTo(sImgDiffName);

    safeprintf(szErrorMsg, 512, "Image Comparison Failed: Error of %u exceeds threshold of %u for image '%s'", uiMeanError, uiMaxError, sImgName.GetData());
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////////
// ezTestFramework static functions
////////////////////////////////////////////////////////////////////////

void ezTestFramework::Output(ezTestOutput::Enum Type, const char* szMsg, ...)
{
  // format the output text
  va_list args;
  va_start(args, szMsg);

  char szBuffer[1024 * 10];
  ezStringUtils::vsnprintf(szBuffer, EZ_ARRAY_SIZE(szBuffer), szMsg, args);
  va_end(args);

  GetInstance()->OutputImpl(Type, szBuffer);
}

void ezTestFramework::Error(const char* szError, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  // format the output text
  va_list args;
  va_start(args, szMsg);

  char szBuffer[1024 * 10];
  ezStringUtils::vsnprintf(szBuffer, EZ_ARRAY_SIZE(szBuffer), szMsg, args);
  va_end(args);

  GetInstance()->ErrorImpl(szError, szFile, iLine, szFunction, szBuffer);
}

void ezTestFramework::TestResult(ezInt32 iSubTestIndex, bool bSuccess, double fDuration)
{
  GetInstance()->TestResultImpl(iSubTestIndex, bSuccess, fDuration);
}



EZ_STATICLINK_FILE(TestFramework, TestFramework_Framework_TestFramework);

