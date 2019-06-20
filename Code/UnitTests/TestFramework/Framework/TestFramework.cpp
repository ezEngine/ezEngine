#include <TestFrameworkPCH.h>

#include <Texture/Image/Formats/ImageFileFormat.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Utilities/ExceptionHandler.h>
#include <Foundation/Utilities/StackTracer.h>
#include <TestFramework/Utilities/TestOrder.h>

#include <cstdlib>

#ifdef EZ_TESTFRAMEWORK_USE_FILESERVE
#  include <FileservePlugin/Client/FileserveClient.h>
#  include <FileservePlugin/Client/FileserveDataDir.h>
#  include <FileservePlugin/FileservePluginDLL.h>
#endif

ezTestFramework* ezTestFramework::s_pInstance = nullptr;

const char* ezTestFramework::s_szTestBlockName = "";
int ezTestFramework::s_iAssertCounter = 0;
bool ezTestFramework::s_bCallstackOnAssert = false;

constexpr int s_iMaxErrorMessageLength = 512;

static void PrintCallstack(const char* szText)
{
  printf("%s", szText);
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  OutputDebugStringW(ezStringWChar(szText).GetData());
#endif
  fflush(stdout);
  fflush(stderr);
};

static bool TestAssertHandler(
  const char* szSourceFile, ezUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  if (ezTestFramework::s_bCallstackOnAssert)
  {
    void* pBuffer[64];
    ezArrayPtr<void*> tempTrace(pBuffer);
    const ezUInt32 uiNumTraces = ezStackTracer::GetStackTrace(tempTrace, nullptr);
    ezStackTracer::ResolveStackTrace(tempTrace.GetSubArray(0, uiNumTraces), &PrintCallstack);
  }
  ezTestFramework::Error(szExpression, szSourceFile, (ezInt32)uiLine, szFunction, szAssertMsg);

  // if a debugger is attached, one typically always wants to know about asserts
  if (ezSystemInformation::IsDebuggerAttached())
    return true;

  return ezTestFramework::GetAssertOnTestFail();
}

static bool EmptyAssertHandler(
  const char* szSourceFile, ezUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  return false;
}

////////////////////////////////////////////////////////////////////////
// ezTestFramework public functions
////////////////////////////////////////////////////////////////////////

ezTestFramework::ezTestFramework(
  const char* szTestName, const char* szAbsTestOutputDir, const char* szRelTestDataDir, int argc, const char** argv)
  : m_sTestName(szTestName)
  , m_sAbsTestOutputDir(szAbsTestOutputDir)
  , m_sRelTestDataDir(szRelTestDataDir)
{
  s_pInstance = this;

  ezCommandLineUtils::GetGlobalInstance()->SetCommandLine(argc, argv, ezCommandLineUtils::PreferOsArgs);

  GetTestSettingsFromCommandLine(argc, argv);
}

ezTestFramework::~ezTestFramework()
{
  if (m_bIsInitialized)
    DeInitialize();
  s_pInstance = nullptr;
}

void ezTestFramework::Initialize()
{
  if (m_Settings.m_bNoGUI)
  {
    // if the UI is run with GUI disabled, set the environment variable EZ_SILENT_ASSERTS
    // to make sure that no child process that the tests launch shows an assert dialog in case of a crash
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    _putenv("EZ_SILENT_ASSERTS=1");
#else
    putenv("EZ_SILENT_ASSERTS=1");
#endif
  }

  // Don't do this, it will spam the log with sub-system messages
  // ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  // ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  ezStartup::AddApplicationTag("testframework");
  ezStartup::StartupCoreSystems();

  // if tests need to write data back through Fileserve (e.g. image comparison results), they can do that through a data dir mounted with
  // this path
  ezFileSystem::SetSpecialDirectory("eztest", ezTestFramework::GetInstance()->GetAbsOutputPath());

  // Setting ez assert handler
  m_PreviousAssertHandler = ezGetAssertHandler();
  ezSetAssertHandler(TestAssertHandler);

  CreateOutputFolder();

  // figure out which tests exist
  GatherAllTests();

  // load the test order from file, if that file does not exist, the array is not modified
  LoadTestOrder();

  // save the current order back to the same file
  AutoSaveTestOrder();

  m_bIsInitialized = true;

  ezFileSystem::DetectSdkRootDirectory();
}

void ezTestFramework::DeInitialize()
{
  m_bIsInitialized = false;

  ezSetAssertHandler(m_PreviousAssertHandler);
  m_PreviousAssertHandler = nullptr;
}

const char* ezTestFramework::GetTestName() const
{
  return m_sTestName.c_str();
}

const char* ezTestFramework::GetAbsOutputPath() const
{
  return m_sAbsTestOutputDir.c_str();
}


const char* ezTestFramework::GetRelTestDataPath() const
{
  return m_sRelTestDataDir.c_str();
}


const char* ezTestFramework::GetAbsTestSettingsFilePath() const
{
  return m_sAbsTestSettingsFilePath.c_str();
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


void ezTestFramework::SetImageDiffExtraInfoCallback(ImageDiffExtraInfoCallback provider)
{
  m_ImageDiffExtraInfoCallback = provider;
}

bool ezTestFramework::GetAssertOnTestFail()
{
  if (!ezSystemInformation::IsDebuggerAttached())
    return false;

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
    e.m_available = pTestClass->IsTestAvailable();

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
  // use a local instance of ezCommandLineUtils as global instance is not guaranteed to have been set up
  // for all call sites of this method.
  ezCommandLineUtils cmd;
  cmd.SetCommandLine(argc, argv, ezCommandLineUtils::PreferOsArgs);

  m_Settings.m_bRunTests = cmd.GetBoolOption("-run", false);
  m_Settings.m_bCloseOnSuccess = cmd.GetBoolOption("-close", false);
  m_Settings.m_bNoGUI = cmd.GetBoolOption("-nogui", false);

  m_Settings.m_bAssertOnTestFail = cmd.GetBoolOption("-assert", m_Settings.m_bAssertOnTestFail);
  m_Settings.m_bOpenHtmlOutput = cmd.GetBoolOption("-html", m_Settings.m_bOpenHtmlOutput);
  m_Settings.m_bKeepConsoleOpen = cmd.GetBoolOption("-console", m_Settings.m_bKeepConsoleOpen);
  m_Settings.m_bShowMessageBox = cmd.GetBoolOption("-msgbox", m_Settings.m_bShowMessageBox);
  m_Settings.m_iRevision = cmd.GetIntOption("-rev", -1);
  m_Settings.m_bEnableAllTests = cmd.GetBoolOption("-all", false);
  m_Settings.m_uiFullPasses = cmd.GetIntOption("-passes", 1, false);

  if (cmd.GetStringOptionArguments("-json") == 1)
    m_Settings.m_sJsonOutput = cmd.GetStringOption("-json", 0, "");

  if (cmd.GetStringOptionArguments("-outputDir") == 1)
  {
    m_sAbsTestOutputDir = cmd.GetStringOption("-outputDir", 0, "");
  }

  if (cmd.GetStringOptionArguments("-settings") == 1)
  {
    m_sAbsTestSettingsFilePath = cmd.GetStringOption("-settings", 0, "");
    // If a custom settings file was provided, default to -nosave as to not overwrite that file with additional
    // parameters from command line. Use "-nosave false" to explicitely enable auto save in this case.
    m_Settings.m_bNoAutomaticSaving = cmd.GetBoolOption("-nosave", true);
  }
  else
  {
    m_sAbsTestSettingsFilePath = m_sAbsTestOutputDir + std::string("/TestSettings.txt");
    m_Settings.m_bNoAutomaticSaving = cmd.GetBoolOption("-nosave", false);
  }

  m_uiPassesLeft = m_Settings.m_uiFullPasses;
}

void ezTestFramework::LoadTestOrder()
{
  ::LoadTestOrder(m_sAbsTestSettingsFilePath.c_str(), m_TestEntries, m_Settings);
  if (m_Settings.m_bEnableAllTests)
    SetAllTestsEnabledStatus(true);
}

void ezTestFramework::CreateOutputFolder()
{
  ezOSFile::CreateDirectoryStructure(m_sAbsTestOutputDir.c_str());

  EZ_ASSERT_RELEASE(
    ezOSFile::ExistsDirectory(m_sAbsTestOutputDir.c_str()), "Failed to create output directory '{0}'", m_sAbsTestOutputDir.c_str());
}

void ezTestFramework::AutoSaveTestOrder()
{
  if (m_Settings.m_bNoAutomaticSaving)
    return;

  SaveTestOrder(m_sAbsTestSettingsFilePath.c_str());
}


void ezTestFramework::SaveTestOrder(const char* const filePath)
{
  ::SaveTestOrder(filePath, m_TestEntries, m_Settings);
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

void ezTestFramework::SetAllFailedTestsEnabledStatus()
{
  const auto& LastResult = GetTestResult();

  const ezUInt32 uiTestCount = GetTestCount();
  for (ezUInt32 uiTestIdx = 0; uiTestIdx < uiTestCount; ++uiTestIdx)
  {
    const auto& TestRes = LastResult.GetTestResultData(uiTestIdx, -1);
    m_TestEntries[uiTestIdx].m_bEnableTest = TestRes.m_bExecuted && !TestRes.m_bSuccess;

    const ezUInt32 uiSubTestCount = (ezUInt32)m_TestEntries[uiTestIdx].m_SubTests.size();
    for (ezUInt32 uiSubTest = 0; uiSubTest < uiSubTestCount; ++uiSubTest)
    {
      const auto& SubTestRes = LastResult.GetTestResultData(uiTestIdx, uiSubTest);
      m_TestEntries[uiTestIdx].m_SubTests[uiSubTest].m_bEnableTest = SubTestRes.m_bExecuted && !SubTestRes.m_bSuccess;
    }
  }
}

void ezTestFramework::SetTestTimeout(ezUInt32 testTimeoutMS)
{
  std::scoped_lock lock(m_timeoutLock);
  m_timeoutMS = testTimeoutMS;
  m_timeoutCV.notify_one();
}

void ezTestFramework::TimeoutThread()
{
  std::unique_lock<std::mutex> lock(m_timeoutLock);
  while (m_useTimeout)
  {
    if (m_timeoutMS == 0)
    {
      // If no timeout is set, we simply put the thread to sleep.
      m_timeoutCV.wait(lock, [this] { return !m_useTimeout; });
    }
    // We want to be notified when we reach the timeout and not when we are spuriously woken up.
    // Thus we continue waiting via the predicate if we are still using a timeout until we are either
    // woken up via the CV or reach the timeout.
    else if (!m_timeoutCV.wait_for(lock, std::chrono::milliseconds(m_timeoutMS), [this] { return !m_useTimeout; }))
    {
      if (ezSystemInformation::IsDebuggerAttached())
      {
        // Should we attach a debugger mid run and reach the timeout we obviously do not want to terminate.
        continue;
      }

      // CV was not signaled until the timeout was reached.
      ezTestFramework::Output(ezTestOutput::Error, "Timeout reached, terminating app.");

      ezExceptionHandler::WriteDump();

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
      // Make sure that Windows doesn't show a default message box when we call abort
      _set_abort_behavior(0, _WRITE_ABORT_MSG);
#endif
      // The OS will still call destructors for our objects (even though we called abort ... what a pointless design).
      // Our code might assert on destruction, so make sure our assert handler doesn't show anything.
      ezSetAssertHandler(EmptyAssertHandler);
      abort();
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
  if (!m_bIsInitialized)
  {
    Initialize();

#ifdef EZ_TESTFRAMEWORK_USE_FILESERVE
    if (ezFileserveClient::GetSingleton() == nullptr)
    {
      EZ_DEFAULT_NEW(ezFileserveClient);

      if (ezFileserveClient::GetSingleton()->SearchForServerAddress().Failed())
      {
        ezFileserveClient::GetSingleton()->WaitForServerInfo();
      }
    }

    if (ezFileserveClient::GetSingleton()->EnsureConnected(ezTime::Seconds(-30)).Failed())
    {
      Error("Failed to establish a Fileserve connection", "", 0, "ezTestFramework::RunTestExecutionLoop", "");
      return ezTestAppRun::Quit;
    }
#endif
  }

#ifdef EZ_TESTFRAMEWORK_USE_FILESERVE
  ezFileserveClient::GetSingleton()->UpdateClient();
#endif


  if (m_iExecutingTest < 0)
  {
    StartTests();
    m_iExecutingTest = 0;
    EZ_ASSERT_DEV(m_iExecutingSubTest == -1, "Invalid test framework state");
    EZ_ASSERT_DEV(!m_bSubTestInitialized, "Invalid test framework state");
  }

  ExecuteNextTest();

  if (m_iExecutingTest >= (ezInt32)m_TestEntries.size())
  {
    EndTests();

    if (m_uiPassesLeft > 1 && !m_bAbortTests)
    {
      --m_uiPassesLeft;

      m_iExecutingTest = -1;
      m_iExecutingSubTest = -1;

      return ezTestAppRun::Continue;
    }

#ifdef EZ_TESTFRAMEWORK_USE_FILESERVE
    if (ezFileserveClient* pClient = ezFileserveClient::GetSingleton())
    {
      // shutdown the fileserve client
      EZ_DEFAULT_DELETE(pClient);
    }
#endif

    return ezTestAppRun::Quit;
  }

  return ezTestAppRun::Continue;
}

void ezTestFramework::StartTests()
{
  ResetTests();
  m_bTestsRunning = true;
  ezTestFramework::Output(ezTestOutput::StartOutput, "");

  // Start timeout thread.
  std::scoped_lock lock(m_timeoutLock);
  m_useTimeout = true;
  m_timeoutThread = std::thread(&ezTestFramework::TimeoutThread, this);
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
      ezTestFramework::Output(ezTestOutput::Warning, "ezLog Warning: %s", e.m_szText);
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

  if (m_iExecutingTest == (ezInt32)GetTestCount())
    return;

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
        m_iExecutingTest = (ezInt32)m_TestEntries.size(); // skip to the end of all tests
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
      if (TestEntry.m_available.Succeeded())
      {
        m_timeoutCV.notify_one();
        if (pTestClass->DoTestInitialization().Failed())
        {
          m_iExecutingSubTest = (ezInt32)TestEntry.m_SubTests.size(); // make sure all subtests are skipped
        }
      }
      else
      {
        ezTestFramework::Output(ezTestOutput::ImportantInfo, "Test not available: %s", TestEntry.m_available.m_sMessage.GetData());
        m_iExecutingSubTest = (ezInt32)TestEntry.m_SubTests.size(); // make sure all subtests are skipped
      }
    }

    if (m_iExecutingSubTest < (ezInt32)TestEntry.m_SubTests.size())
    {
      ezSubTestEntry& subTest = TestEntry.m_SubTests[m_iExecutingSubTest];
      ezInt32 iSubTestIdentifier = subTest.m_iSubTestIdentifier;

      if (!subTest.m_bEnableTest)
      {
        ++m_iExecutingSubTest;
        return;
      }

      if (!m_bSubTestInitialized)
      {
        if (m_bAbortTests)
        {
          // tests shall be aborted, so do not start a new one

          m_iExecutingTest = (ezInt32)m_TestEntries.size(); // skip to the end of all tests
          m_iExecutingSubTest = -1;
          return;
        }

        m_fTotalSubTestDuration = 0.0;
        m_uiSubTestInvocationCount = 0;

        // First flush of assert counter, these are all asserts during test init.
        FlushAsserts();
        m_iCurrentSubTestIndex = m_iExecutingSubTest;
        ezTestFramework::Output(ezTestOutput::BeginBlock, "Executing Sub-Test: '%s'", subTest.m_szSubTestName);

        // *** Sub-Test Initialization ***
        m_timeoutCV.notify_one();
        m_bSubTestInitialized = pTestClass->DoSubTestInitialization(iSubTestIdentifier).Succeeded();
      }

      ezTestAppRun subTestResult = ezTestAppRun::Quit;

      if (m_bSubTestInitialized)
      {
        // *** Run Sub-Test ***
        double fDuration = 0.0;

        // start with 1
        ++m_uiSubTestInvocationCount;

        m_timeoutCV.notify_one();
        subTestResult = pTestClass->DoSubTestRun(iSubTestIdentifier, fDuration, m_uiSubTestInvocationCount);

        if (m_bImageComparisonScheduled)
        {
          EZ_TEST_IMAGE(m_uiComparisonImageNumber, m_uiMaxImageComparisonError);
          m_bImageComparisonScheduled = false;
        }

        // I guess we can require that tests are written in a way that they can be interrupted
        if (m_bAbortTests)
          subTestResult = ezTestAppRun::Quit;

        m_fTotalSubTestDuration += fDuration;
      }

      // this is executed when sub-test initialization failed or the sub-test reached its end
      if (subTestResult == ezTestAppRun::Quit)
      {
        // *** Sub-Test De-Initialization ***
        m_timeoutCV.notify_one();
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

    if (m_iExecutingSubTest >= (ezInt32)TestEntry.m_SubTests.size())
    {
      // *** Test De-Initialization ***
      m_timeoutCV.notify_one();
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

  // Stop timeout thread.
  {
    std::scoped_lock lock(m_timeoutLock);
    m_useTimeout = false;
    m_timeoutCV.notify_one();
  }
  m_timeoutThread.join();
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

const ezStatus& ezTestFramework::IsTestAvailable(ezUInt32 uiTestIndex) const
{
  EZ_ASSERT_DEV(uiTestIndex < GetTestCount(), "Test index {0} is larger than number of tests {1}.", uiTestIndex, GetTestCount());
  return m_TestEntries[uiTestIndex].m_available;
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
  if (Type == ezTestOutput::Error)
  {
    m_iErrorCount++;
  }
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
  m_Result.TestError(
    m_iCurrentTestIndex, m_iCurrentSubTestIndex, szError, ezTestFramework::s_szTestBlockName, szFile, iLine, szFunction, szMsg);

  g_bBlockOutput = true;
  ezTestFramework::Output(ezTestOutput::Error, "%s", szError); // This will also increase the global error count.
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
}

void ezTestFramework::TestResultImpl(ezInt32 iSubTestIndex, bool bSuccess, double fDuration)
{
  m_Result.TestResult(m_iCurrentTestIndex, iSubTestIndex, bSuccess, fDuration);

  const ezUInt32 uiMin = (ezUInt32)(fDuration / 1000.0 / 60.0);
  const ezUInt32 uiSec = (ezUInt32)(fDuration / 1000.0 - uiMin * 60.0);
  const ezUInt32 uiMS = (ezUInt32)(fDuration - uiSec * 1000.0);

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
      ezTestFramework::Output(ezTestOutput::Error, "Test '%s' failed: %i Errors.", szTestName,
        (ezUInt32)m_Result.GetErrorMessageCount(m_iCurrentTestIndex, iSubTestIndex));
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
      ezTestFramework::Output(ezTestOutput::Error, "Sub-Test '%s' failed: %i Errors.", szSubTestName,
        (ezUInt32)m_Result.GetErrorMessageCount(m_iCurrentTestIndex, iSubTestIndex));
    }
  }
}

void ezTestFramework::FlushAsserts()
{
  m_Result.AddAsserts(m_iCurrentTestIndex, m_iCurrentSubTestIndex, s_iAssertCounter);
  s_iAssertCounter = 0;
}

void ezTestFramework::ScheduleImageComparison(ezUInt32 uiImageNumber, ezUInt32 uiMaxError)
{
  m_bImageComparisonScheduled = true;
  m_uiMaxImageComparisonError = uiMaxError;
  m_uiComparisonImageNumber = uiImageNumber;
}

void ezTestFramework::GenerateComparisonImageName(ezUInt32 uiImageNumber, ezStringBuilder& sImgName)
{
  const char* szTestName = GetTest(GetCurrentTestIndex())->m_szTestName;
  const char* szSubTestName = GetTest(GetCurrentTestIndex())->m_SubTests[GetCurrentSubTestIndex()].m_szSubTestName;

  sImgName.Format("{0}_{1}_{2}", szTestName, szSubTestName, ezArgI(uiImageNumber, 3, true));
  sImgName.ReplaceAll(" ", "_");
}

void ezTestFramework::GetCurrentComparisonImageName(ezStringBuilder& sImgName)
{
  GenerateComparisonImageName(m_uiComparisonImageNumber, sImgName);
}

void ezTestFramework::SetImageReferenceFolderName(const char* szFolderName)
{
  m_sImageReferenceFolderName = szFolderName;
}

void ezTestFramework::SetImageReferenceOverrideFolderName(const char* szFolderName)
{
  m_sImageReferenceOverrideFolderName = szFolderName;
}

static const ezUInt8 s_Base64EncodingTable[64] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
  'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
  't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

static const ezUInt8 BASE64_CHARS_PER_LINE = 76;

static ezUInt32 GetBase64EncodedLength(ezUInt32 inputLength, bool insertLineBreaks)
{
  ezUInt32 outputLength = (inputLength + 2) / 3 * 4;

  if (insertLineBreaks)
  {
    outputLength += outputLength / BASE64_CHARS_PER_LINE;
  }

  return outputLength;
}


static ezDynamicArray<char> ArrayToBase64(ezArrayPtr<const ezUInt8> in, bool insertLineBreaks = true)
{
  ezDynamicArray<char> out;
  out.SetCountUninitialized(GetBase64EncodedLength(in.GetCount(), insertLineBreaks));

  ezUInt32 offsetIn = 0;
  ezUInt32 offsetOut = 0;

  ezUInt32 blocksTillNewline = BASE64_CHARS_PER_LINE / 4;
  while (offsetIn < in.GetCount())
  {
    ezUInt8 ibuf[3] = {0};

    ezUInt32 ibuflen = ezMath::Min(in.GetCount() - offsetIn, 3u);

    for (ezUInt32 i = 0; i < ibuflen; ++i)
    {
      ibuf[i] = in[offsetIn++];
    }

    char obuf[4];
    obuf[0] = s_Base64EncodingTable[(ibuf[0] >> 2)];
    obuf[1] = s_Base64EncodingTable[((ibuf[0] << 4) & 0x30) | (ibuf[1] >> 4)];
    obuf[2] = s_Base64EncodingTable[((ibuf[1] << 2) & 0x3c) | (ibuf[2] >> 6)];
    obuf[3] = s_Base64EncodingTable[(ibuf[2] & 0x3f)];

    if (ibuflen >= 3)
    {
      out[offsetOut++] = obuf[0];
      out[offsetOut++] = obuf[1];
      out[offsetOut++] = obuf[2];
      out[offsetOut++] = obuf[3];
    }
    else // need to pad up to 4
    {
      switch (ibuflen)
      {
        case 1:
          out[offsetOut++] = obuf[0];
          out[offsetOut++] = obuf[1];
          out[offsetOut++] = '=';
          out[offsetOut++] = '=';
          break;
        case 2:
          out[offsetOut++] = obuf[0];
          out[offsetOut++] = obuf[1];
          out[offsetOut++] = obuf[2];
          out[offsetOut++] = '=';
          break;
      }
    }

    if (--blocksTillNewline == 0)
    {
      if (insertLineBreaks)
      {
        out[offsetOut++] = '\n';
      }
      blocksTillNewline = 19;
    }
  }

  EZ_ASSERT_DEV(offsetOut == out.GetCount(), "All output data should have been written");
  return out;
}

static void AppendImageData(ezStringBuilder& output, ezImage& img)
{
  ezImageFileFormat* format = ezImageFileFormat::GetWriterFormat("png");
  EZ_ASSERT_DEV(format != nullptr, "No PNG writer found");

  ezDynamicArray<ezUInt8> imgData;
  ezMemoryStreamContainerWrapperStorage<ezDynamicArray<ezUInt8>> storage(&imgData);
  ezMemoryStreamWriter writer(&storage);
  format->WriteImage(writer, img, ezLog::GetThreadLocalLogSystem(), "png");

  ezDynamicArray<char> imgDataBase64 = ArrayToBase64(imgData.GetArrayPtr());
  ezStringView imgDataBase64StringView(imgDataBase64.GetArrayPtr().GetPtr(), imgDataBase64.GetArrayPtr().GetEndPtr());
  output.AppendFormat("data:image/png;base64,{0}", imgDataBase64StringView);
}

void ezTestFramework::WriteImageDiffHtml(const char* fileName, ezImage& referenceImgRgb, ezImage& referenceImgAlpha,
  ezImage& capturedImgRgb, ezImage& capturedImgAlpha, ezImage& diffImgRgb, ezImage& diffImgAlpha, ezUInt32 uiError, ezUInt32 uiThreshold,
  ezUInt8 uiMinDiffRgb, ezUInt8 uiMaxDiffRgb, ezUInt8 uiMinDiffAlpha, ezUInt8 uiMaxDiffAlpha)
{

  ezFileWriter outputFile;
  if (outputFile.Open(fileName).Failed())
  {
    ezTestFramework::Output(ezTestOutput::Warning, "Could not open HTML diff file \"%s\" for writing.", fileName);
    return;
  }

  ezStringBuilder output;
  output.Append("<!DOCTYPE html PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
                "<!DOCTYPE html PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
                "<HTML> <HEAD>\n");
  const char* szTestName = GetTest(GetCurrentTestIndex())->m_szTestName;
  const char* szSubTestName = GetTest(GetCurrentTestIndex())->m_SubTests[GetCurrentSubTestIndex()].m_szSubTestName;
  output.AppendFormat("<TITLE>{} - {}</TITLE>\n", szTestName, szSubTestName);
  output.Append("<script type = \"text/javascript\">\n"
                "function showReferenceImage()\n"
                "{\n"
                "    document.getElementById('image_current_rgb').style.display = 'none'\n"
                "    document.getElementById('image_current_a').style.display = 'none'\n"
                "    document.getElementById('image_reference_rgb').style.display = 'inline-block'\n"
                "    document.getElementById('image_reference_a').style.display = 'inline-block'\n"
                "    document.getElementById('image_caption_rgb').innerHTML = 'Displaying: Reference Image RGB'\n"
                "    document.getElementById('image_caption_a').innerHTML = 'Displaying: Reference Image Alpha'\n"
                "}\n"
                "function showCurrentImage()\n"
                "{\n"
                "    document.getElementById('image_current_rgb').style.display = 'inline-block'\n"
                "    document.getElementById('image_current_a').style.display = 'inline-block'\n"
                "    document.getElementById('image_reference_rgb').style.display = 'none'\n"
                "    document.getElementById('image_reference_a').style.display = 'none'\n"
                "    document.getElementById('image_caption_rgb').innerHTML = 'Displaying: Current Image RGB'\n"
                "    document.getElementById('image_caption_a').innerHTML = 'Displaying: Current Image Alpha'\n"
                "}\n"
                "function imageover()\n"
                "{\n"
                "    var mode = document.querySelector('input[name=\"image_interaction_mode\"]:checked').value\n"
                "    if (mode == 'interactive')\n"
                "    {\n"
                "        showReferenceImage()\n"
                "    }\n"
                "}\n"
                "function imageout()\n"
                "{\n"
                "    var mode = document.querySelector('input[name=\"image_interaction_mode\"]:checked').value\n"
                "    if (mode == 'interactive')\n"
                "    {\n"
                "        showCurrentImage()\n"
                "    }\n"
                "}\n"
                "function handleModeClick(clickedItem)\n"
                "{\n"
                "    if (clickedItem.value == 'current_image' || clickedItem.value == 'interactive')\n"
                "    {\n"
                "        showCurrentImage()\n"
                "    }\n"
                "    else if (clickedItem.value == 'reference_image')\n"
                "    {\n"
                "        showReferenceImage()\n"
                "    }\n"
                "}\n"
                "</script>\n"
                "</HEAD>\n"
                "<BODY bgcolor=\"#ccdddd\">\n"
                "<div style=\"line-height: 1.5; margin-top: 0px; margin-left: 10px; font-family: sans-serif;\">\n");

  output.AppendFormat("<b>Test result for \"{} > {}\" from ", szTestName, szSubTestName);
  ezDateTime dateTime(ezTimestamp::CurrentTimestamp());
  output.AppendFormat("{}-{}-{} {}:{}:{}</b><br>\n", dateTime.GetYear(), ezArgI(dateTime.GetMonth(), 2, true),
    ezArgI(dateTime.GetDay(), 2, true), ezArgI(dateTime.GetHour(), 2, true), ezArgI(dateTime.GetMinute(), 2, true),
    ezArgI(dateTime.GetSecond(), 2, true));

  output.Append("<table cellpadding=\"0\" cellspacing=\"0\" border=\"0\">\n");

  if (m_ImageDiffExtraInfoCallback)
  {
    ezDynamicArray<std::pair<ezString, ezString>> extraInfo = m_ImageDiffExtraInfoCallback();

    for (const auto& labelValuePair : extraInfo)
    {
      output.AppendFormat("<tr>\n"
                          "<td>{}:</td>\n"
                          "<td align=\"right\" style=\"padding-left: 2em;\">{}</td>\n"
                          "</tr>\n",
        labelValuePair.first, labelValuePair.second);
    }
  }

  output.AppendFormat("<tr>\n"
                      "<td>Error metric:</td>\n"
                      "<td align=\"right\" style=\"padding-left: 2em;\">{}</td>\n"
                      "</tr>\n",
    uiError);
  output.AppendFormat("<tr>\n"
                      "<td>Error threshold:</td>\n"
                      "<td align=\"right\" style=\"padding-left: 2em;\">{}</td>\n"
                      "</tr>\n",
    uiThreshold);
  output.Append("</table>\n"
                "<div style=\"margin-top: 0.5em; margin-bottom: -0.75em\">\n"
                "    <input type=\"radio\" name=\"image_interaction_mode\" onclick=\"handleModeClick(this)\" value=\"interactive\" "
                "checked=\"checked\"> Mouse-Over Image Switching\n"
                "    <input type=\"radio\" name=\"image_interaction_mode\" onclick=\"handleModeClick(this)\" value=\"current_image\"> "
                "Current Image\n"
                "    <input type=\"radio\" name=\"image_interaction_mode\" onclick=\"handleModeClick(this)\" value=\"reference_image\"> "
                "Reference Image\n"
                "</div>\n");

  output.AppendFormat("<div style=\"width:{}px;display: inline-block;\">\n", capturedImgRgb.GetWidth());

  output.Append("<p id=\"image_caption_rgb\">Displaying: Current Image RGB</p>\n"

                "<div style=\"block;\" onmouseover=\"imageover()\" onmouseout=\"imageout()\">\n"
                "<img id=\"image_current_rgb\" alt=\"Captured Image RGB\" src=\"");
  AppendImageData(output, capturedImgRgb);
  output.Append("\" />\n"
                "<img id=\"image_reference_rgb\" style=\"display: none\" alt=\"Reference Image RGB\" src=\"");
  AppendImageData(output, referenceImgRgb);
  output.Append("\" />\n"
                "</div>\n"
                "<div style=\"display: block;\">\n");
  output.AppendFormat("<p>RGB Difference (min: {}, max: {}):</p>\n", uiMinDiffRgb, uiMaxDiffRgb);
  output.Append("<img alt=\"Diff Image RGB\" src=\"");
  AppendImageData(output, diffImgRgb);
  output.Append("\" />\n"
                "</div>\n"
                "</div>\n");

  output.AppendFormat("<div style=\"width:{}px;display: inline-block;\">\n", capturedImgAlpha.GetWidth());

  output.Append("<p id=\"image_caption_a\">Displaying: Current Image Alpha</p>\n"
                "<div style=\"display: block;\" onmouseover=\"imageover()\" onmouseout=\"imageout()\">\n"
                "<img id=\"image_current_a\" alt=\"Captured Image Alpha\" src=\"");
  AppendImageData(output, capturedImgAlpha);
  output.Append("\" />\n"
                "<img id=\"image_reference_a\" style=\"display: none\" alt=\"Reference Image Alpha\" src=\"");
  AppendImageData(output, referenceImgAlpha);
  output.Append("\" />\n"
                "</div>\n"
                "<div style=\"px;display: block;\">\n");
  output.AppendFormat("<p>Alpha Difference (min: {}, max: {}):</p>\n", uiMinDiffAlpha, uiMaxDiffAlpha);
  output.Append("<img alt=\"Diff Image Alpha\" src=\"");
  AppendImageData(output, diffImgAlpha);
  output.Append("\" />\n"
                "</div>\n"
                "</div>\n"
                "</div>\n"
                "</BODY> </HTML>");

  outputFile.WriteBytes(output.GetData(), output.GetCharacterCount());
  outputFile.Close();
}

bool ezTestFramework::PerformImageComparison(ezStringBuilder sImgName, const ezImage& img, ezUInt32 uiMaxError, char* szErrorMsg)
{
  ezImage imgRgba;
  if (ezImageConversion::Convert(img, imgRgba, ezImageFormat::R8G8B8A8_UNORM).Failed())
  {
    safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Captured Image '%s' could not be converted to RGBA8", sImgName.GetData());
    return false;
  }

  ezStringBuilder sImgPathReference, sImgPathResult;

  if (!m_sImageReferenceOverrideFolderName.empty())
  {
    sImgPathReference.Format("{0}/{1}.png", m_sImageReferenceOverrideFolderName.c_str(), sImgName);

    if (!ezFileSystem::ExistsFile(sImgPathReference))
    {
      // try the regular path
      sImgPathReference.Clear();
    }
  }

  if (sImgPathReference.IsEmpty())
  {
    sImgPathReference.Format("{0}/{1}.png", m_sImageReferenceFolderName.c_str(), sImgName);
  }

  sImgPathResult.Format(":imgout/Images_Result/{0}.png", sImgName);

  ezImage imgExp, imgExpRgba;
  if (imgExp.LoadFrom(sImgPathReference).Failed())
  {
    imgRgba.SaveTo(sImgPathResult);

    safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Comparison Image '%s' could not be read", sImgPathReference.GetData());
    return false;
  }

  if (ezImageConversion::Convert(imgExp, imgExpRgba, ezImageFormat::R8G8B8A8_UNORM).Failed())
  {
    imgRgba.SaveTo(sImgPathResult);

    safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Comparison Image '%s' could not be converted to RGBA8", sImgPathReference.GetData());
    return false;
  }

  if (imgRgba.GetWidth() != imgExpRgba.GetWidth() || imgRgba.GetHeight() != imgExpRgba.GetHeight())
  {
    imgRgba.SaveTo(sImgPathResult);

    safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Comparison Image '%s' size (%ix%i) does not match captured image size (%ix%i)",
      sImgPathReference.GetData(), imgRgba.GetWidth(), imgRgba.GetHeight(), imgExpRgba.GetWidth(), imgExpRgba.GetHeight());
    return false;
  }

  ezImage imgDiffRgba;
  ezImageUtils::ComputeImageDifferenceABS(imgExpRgba, imgRgba, imgDiffRgba);

  const ezUInt32 uiMeanError = ezImageUtils::ComputeMeanSquareError(imgDiffRgba, 32);

  if (uiMeanError > uiMaxError)
  {
    imgRgba.SaveTo(sImgPathResult);

    ezUInt8 uiMinDiffRgb, uiMaxDiffRgb, uiMinDiffAlpha, uiMaxDiffAlpha;
    ezImageUtils::Normalize(imgDiffRgba, uiMinDiffRgb, uiMaxDiffRgb, uiMinDiffAlpha, uiMaxDiffAlpha);

    ezImage imgDiffRgb;
    ezImageConversion::Convert(imgDiffRgba, imgDiffRgb, ezImageFormat::R8G8B8_UNORM);

    ezStringBuilder sImgDiffName;
    sImgDiffName.Format(":imgout/Images_Diff/{0}.png", sImgName);
    imgDiffRgb.SaveTo(sImgDiffName);

    ezImage imgDiffAlpha;
    ezImageUtils::ExtractAlphaChannel(imgDiffRgba, imgDiffAlpha);

    ezStringBuilder sImgDiffAlphaName;
    sImgDiffAlphaName.Format(":imgout/Images_Diff/{0}_alpha.png", sImgName);
    imgDiffAlpha.SaveTo(sImgDiffAlphaName);

    ezImage imgExpRgb;
    ezImageConversion::Convert(imgExpRgba, imgExpRgb, ezImageFormat::R8G8B8_UNORM);
    ezImage imgExpAlpha;
    ezImageUtils::ExtractAlphaChannel(imgExpRgba, imgExpAlpha);

    ezImage imgRgb;
    ezImageConversion::Convert(imgRgba, imgRgb, ezImageFormat::R8G8B8_UNORM);
    ezImage imgAlpha;
    ezImageUtils::ExtractAlphaChannel(imgRgba, imgAlpha);

    ezStringBuilder sDiffHtmlPath;
    sDiffHtmlPath.Format(":imgout/Html_Diff/{0}.html", sImgName);
    WriteImageDiffHtml(sDiffHtmlPath, imgExpRgb, imgExpAlpha, imgRgb, imgAlpha, imgDiffRgb, imgDiffAlpha, uiMeanError, uiMaxError,
      uiMinDiffRgb, uiMaxDiffRgb, uiMinDiffAlpha, uiMaxDiffAlpha);

    safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Image Comparison Failed: Error of %u exceeds threshold of %u for image '%s'.",
      uiMeanError, uiMaxError, sImgName.GetData());

    ezStringBuilder sDataDirRelativePath;
    ezFileSystem::ResolvePath(sDiffHtmlPath, nullptr, &sDataDirRelativePath);
    ezTestFramework::Output(ezTestOutput::ImageDiffFile, sDataDirRelativePath);
    return false;
  }

  return true;
}

bool ezTestFramework::CompareImages(ezUInt32 uiImageNumber, ezUInt32 uiMaxError, char* szErrorMsg)
{
  ezStringBuilder sImgName;
  GenerateComparisonImageName(uiImageNumber, sImgName);

  ezImage img;
  if (GetTest(GetCurrentTestIndex())->m_pTest->GetImage(img).Failed())
  {
    safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Image '%s' could not be captured", sImgName.GetData());
    return false;
  }

  bool bImagesMatch = true;
  if (img.GetNumArrayIndices() <= 1)
  {
    bImagesMatch = PerformImageComparison(sImgName, img, uiMaxError, szErrorMsg);
  }
  else
  {
    ezStringBuilder lastError;
    for (ezUInt32 i = 0; i < img.GetNumArrayIndices(); ++i)
    {
      ezStringBuilder subImageName;
      subImageName.AppendFormat("{0}_{1}", sImgName, i);
      if (!PerformImageComparison(subImageName, img.GetSubImageView(0, 0, i), uiMaxError, szErrorMsg))
      {
        bImagesMatch = false;
        if (!lastError.IsEmpty())
        {
          ezTestFramework::Output(ezTestOutput::Error, "%s", lastError.GetData());
        }
        lastError = szErrorMsg;
      }
    }
  }

  if (m_ImageComparisonCallback)
  {
    m_ImageComparisonCallback(bImagesMatch);
  }

  return bImagesMatch;
}

void ezTestFramework::SetImageComparisonCallback(const ImageComparisonCallback& callback)
{
  m_ImageComparisonCallback = callback;
}

ezResult ezTestFramework::CaptureRegressionStat(ezStringView testName, ezStringView name, ezStringView unit,
                                                float value, ezInt32 testId)
{
  ezStringBuilder strippedTestName = testName;
  strippedTestName.ReplaceAll(" ", "");

  ezStringBuilder perTestName;
  if (testId < 0)
  {
    perTestName.Format("{}_{}", strippedTestName, name);
  }
  else
  {
    perTestName.Format("{}_{}_{}", strippedTestName, name, testId);
  }

  {
    ezStringBuilder regression;
    // The 6 floating point digits are forced as per a requirement of the CI
    // feature that parses these values.
    regression.Format("[test][REGRESSION:{}:{}:{}]", perTestName, unit, ezArgF(value, 6));
    ezLog::Info(regression);
  }

  return EZ_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// ezTestFramework static functions
////////////////////////////////////////////////////////////////////////

void ezTestFramework::Output(ezTestOutput::Enum Type, const char* szMsg, ...)
{
  va_list args;
  va_start(args, szMsg);

  OutputArgs(Type, szMsg, args);

  va_end(args);
}

void ezTestFramework::OutputArgs(ezTestOutput::Enum Type, const char* szMsg, va_list args)
{
  // format the output text
  char szBuffer[1024 * 10];
  ezStringUtils::vsnprintf(szBuffer, EZ_ARRAY_SIZE(szBuffer), szMsg, args);

  GetInstance()->OutputImpl(Type, szBuffer);
}

void ezTestFramework::Error(const char* szError, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  va_list args;
  va_start(args, szMsg);

  Error(szError, szFile, iLine, szFunction, szMsg, args);

  va_end(args);
}

void ezTestFramework::Error(const char* szError, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg, va_list args)
{
  // format the output text
  char szBuffer[1024 * 10];
  ezStringUtils::vsnprintf(szBuffer, EZ_ARRAY_SIZE(szBuffer), szMsg, args);

  GetInstance()->ErrorImpl(szError, szFile, iLine, szFunction, szBuffer);
}

void ezTestFramework::TestResult(ezInt32 iSubTestIndex, bool bSuccess, double fDuration)
{
  GetInstance()->TestResultImpl(iSubTestIndex, bSuccess, fDuration);
}

////////////////////////////////////////////////////////////////////////
// EZ_TEST_... macro functions
////////////////////////////////////////////////////////////////////////

#define OUTPUT_TEST_ERROR                                                                                                                  \
  {                                                                                                                                        \
    va_list args;                                                                                                                          \
    va_start(args, szMsg);                                                                                                                 \
    ezTestFramework::Error(szErrorText, szFile, iLine, szFunction, szMsg, args);                                                           \
    EZ_TEST_DEBUG_BREAK                                                                                                                    \
    va_end(args);                                                                                                                          \
    return EZ_FAILURE;                                                                                                                     \
  }

ezResult ezTestBool(
  bool bCondition, const char* szErrorText, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  ezTestFramework::s_iAssertCounter++;

  if (!bCondition)
  {
    // if the test breaks here, go one up in the callstack to see where it exactly failed
    OUTPUT_TEST_ERROR
  }

  return EZ_SUCCESS;
}

ezResult ezTestDouble(double f1, double f2, double fEps, const char* szF1, const char* szF2, const char* szFile, ezInt32 iLine,
  const char* szFunction, const char* szMsg, ...)
{
  ezTestFramework::s_iAssertCounter++;

  const double fD = f1 - f2;

  if (fD < -fEps || fD > +fEps)
  {
    char szErrorText[256];
    safeprintf(szErrorText, 256, "Failure: '%s' (%.8f) does not equal '%s' (%.8f) within an epsilon of %.8f", szF1, f1, szF2, f2, fEps);

    OUTPUT_TEST_ERROR
  }

  return EZ_SUCCESS;
}

ezResult ezTestInt(ezInt64 i1, ezInt64 i2, const char* szI1, const char* szI2, const char* szFile, ezInt32 iLine, const char* szFunction,
  const char* szMsg, ...)
{
  ezTestFramework::s_iAssertCounter++;

  if (i1 != i2)
  {
    char szErrorText[256];
    safeprintf(szErrorText, 256, "Failure: '%s' (%i) does not equal '%s' (%i)", szI1, i1, szI2, i2);

    OUTPUT_TEST_ERROR
  }

  return EZ_SUCCESS;
}

ezResult ezTestString(std::string s1, std::string s2, const char* szString1, const char* szString2, const char* szFile, ezInt32 iLine,
  const char* szFunction, const char* szMsg, ...)
{
  ezTestFramework::s_iAssertCounter++;

  if (s1 != s2)
  {
    char szErrorText[2048];
    safeprintf(szErrorText, 2048, "Failure: '%s' (%s) does not equal '%s' (%s)", szString1, s1.c_str(), szString2, s2.c_str());

    OUTPUT_TEST_ERROR
  }

  return EZ_SUCCESS;
}

ezResult ezTestVector(ezVec4d v1, ezVec4d v2, double fEps, const char* szCondition, const char* szFile, ezInt32 iLine,
  const char* szFunction, const char* szMsg, ...)
{
  ezTestFramework::s_iAssertCounter++;

  char szErrorText[256];

  if (!ezMath::IsEqual(v1.x, v2.x, fEps))
  {
    safeprintf(
      szErrorText, 256, "Failure: '%s' - v1.x (%.8f) does not equal v2.x (%.8f) within an epsilon of %.8f", szCondition, v1.x, v2.x, fEps);

    OUTPUT_TEST_ERROR
  }

  if (!ezMath::IsEqual(v1.y, v2.y, fEps))
  {
    safeprintf(
      szErrorText, 256, "Failure: '%s' - v1.y (%.8f) does not equal v2.y (%.8f) within an epsilon of %.8f", szCondition, v1.y, v2.y, fEps);

    OUTPUT_TEST_ERROR
  }

  if (!ezMath::IsEqual(v1.z, v2.z, fEps))
  {
    safeprintf(
      szErrorText, 256, "Failure: '%s' - v1.z (%.8f) does not equal v2.z (%.8f) within an epsilon of %.8f", szCondition, v1.z, v2.z, fEps);

    OUTPUT_TEST_ERROR
  }

  if (!ezMath::IsEqual(v1.w, v2.w, fEps))
  {
    safeprintf(
      szErrorText, 256, "Failure: '%s' - v1.w (%.8f) does not equal v2.w (%.8f) within an epsilon of %.8f", szCondition, v1.w, v2.w, fEps);

    OUTPUT_TEST_ERROR
  }

  return EZ_SUCCESS;
}

ezResult ezTestFiles(
  const char* szFile1, const char* szFile2, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  ezTestFramework::s_iAssertCounter++;

  char szErrorText[s_iMaxErrorMessageLength];

  ezFileReader ReadFile1;
  ezFileReader ReadFile2;

  if (ReadFile1.Open(szFile1) == EZ_FAILURE)
  {
    safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: File '%s' could not be read.", szFile1);

    OUTPUT_TEST_ERROR
  }
  else if (ReadFile2.Open(szFile2) == EZ_FAILURE)
  {
    safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: File '%s' could not be read.", szFile2);

    OUTPUT_TEST_ERROR
  }

  else if (ReadFile1.GetFileSize() != ReadFile2.GetFileSize())
  {
    safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: File sizes do not match: '%s' (%llu Bytes) and '%s' (%llu Bytes)", szFile1,
      ReadFile1.GetFileSize(), szFile2, ReadFile2.GetFileSize());

    OUTPUT_TEST_ERROR
  }
  else
  {
    while (true)
    {
      ezUInt8 uiTemp1[512];
      ezUInt8 uiTemp2[512];
      const ezUInt64 uiRead1 = ReadFile1.ReadBytes(uiTemp1, 512);
      const ezUInt64 uiRead2 = ReadFile2.ReadBytes(uiTemp2, 512);

      if (uiRead1 != uiRead2)
      {
        safeprintf(
          szErrorText, s_iMaxErrorMessageLength, "Failure: Files could not read same amount of data: '%s' and '%s'", szFile1, szFile2);

        OUTPUT_TEST_ERROR
      }
      else
      {
        if (uiRead1 == 0)
          break;

        if (memcmp(uiTemp1, uiTemp2, (size_t)uiRead1) != 0)
        {
          safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: Files contents do not match: '%s' and '%s'", szFile1, szFile2);

          OUTPUT_TEST_ERROR
        }
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTestImage(
  ezUInt32 uiImageNumber, ezUInt32 uiMaxError, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  char szErrorText[s_iMaxErrorMessageLength] = "";

  if (!ezTestFramework::GetInstance()->CompareImages(uiImageNumber, uiMaxError, szErrorText))
  {
    OUTPUT_TEST_ERROR
  }

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(TestFramework, TestFramework_Framework_TestFramework);
