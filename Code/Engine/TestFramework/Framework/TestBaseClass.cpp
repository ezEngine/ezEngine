#include <TestFramework/PCH.h>
#include <TestFramework/Framework/TestFramework.h>
#include <Foundation/Logging/Log.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezTestBaseClass);

void ezTestBaseClass::ClearSubTests()
{
  m_Entries.clear();
}

void ezTestBaseClass::SetSubTestDuration(ezInt32 iIndex, double fDuration)
{
  m_Entries[iIndex].m_fDuration = fDuration;
}

void ezTestBaseClass::AddSubTest(const char* szName, ezInt32 iIdentifier)
{
  TestEntry e;
  e.m_szName = szName;
  e.m_iIdentifier = iIdentifier;

  m_Entries.push_back(e);
}

ezResult ezTestBaseClass::DoTestInitialization()
{
  try
  {
    if (InitializeTest() == EZ_FAILURE)
    {
      ezTestFramework::Output(ezTestOutput::Error, "Test Initialization failed.");
      return EZ_FAILURE;
    }
  }
  catch(...)
  {
    ezTestFramework::Output(ezTestOutput::Error, "Exception during test initialization.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezTestBaseClass::DoTestDeInitialization()
{
  try
  {
    if (DeInitializeTest() == EZ_FAILURE)
      ezTestFramework::Output(ezTestOutput::Error, "Test DeInitialization failed.");
  }
  catch(...)
  {
    ezTestFramework::Output(ezTestOutput::Error, "Exception during test de-initialization.");
  }
}

ezResult ezTestBaseClass::DoSubTestInitialization(ezInt32 iIdentifier)
{
  try
  {
    if (InitializeSubTest(iIdentifier) == EZ_FAILURE)
    {
      ezTestFramework::Output(ezTestOutput::Error, "Sub-Test Initialization failed, skipping Test.");
      return EZ_FAILURE;
    }
  }
  catch(...)
  {
    ezTestFramework::Output(ezTestOutput::Error, "Exception during sub-test initialization.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezTestBaseClass::DoSubTestDeInitialization(ezInt32 iIdentifier)
{
  try
  {
    if (DeInitializeSubTest(iIdentifier) == EZ_FAILURE)
      ezTestFramework::Output(ezTestOutput::Error, "Sub-Test De-Initialization failed.");
  }
  catch(...)
  {
    ezTestFramework::Output(ezTestOutput::Error, "Exception during sub-test de-initialization.");
  }
}

double ezTestBaseClass::DoSubTestRun(ezInt32 iIdentifier, const char* szSubTestName)
{
  double fDuration = 0.0;

  try
  {
    #if EZ_PLATFORM_WINDOWS
      ezUInt64 uiStartTime, uiEndTime, uiFrequency;
      QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&uiStartTime));
    #endif

    RunSubTest(iIdentifier);

    #if EZ_PLATFORM_WINDOWS
      QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&uiEndTime));
      QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&uiFrequency));

      fDuration = ((double) (uiEndTime - uiStartTime) / (double) uiFrequency) * 1000.0;
    #endif

    //SetSubTestDuration(iIndex, fDuration);
  }
  catch(...)
  {
    ezTestFramework::Output(ezTestOutput::Error, "Exception during sub-test '%s'.", szSubTestName);
  }

  return fDuration;
}

static void LogWriter(const ezLog::LoggingEvent& e, void* ptr)
{
  switch (e.m_EventType)
  {
  case ezLog::EventType::FatalErrorMsg:
    ezTestFramework::Output(ezTestOutput::Error, "ezLog Fatal Error: %s", e.m_szText);
    break;
  case ezLog::EventType::ErrorMsg:
    ezTestFramework::Output(ezTestOutput::Error, "ezLog Error: %s", e.m_szText);
    break;
  case ezLog::EventType::SeriousWarningMsg:
    ezTestFramework::Output(ezTestOutput::Error, "ezLog Serious Warning: %s", e.m_szText);
    break;
  case ezLog::EventType::WarningMsg:
    ezTestFramework::Output(ezTestOutput::Info, "ezLog Warning: %s", e.m_szText);
    break;
  default:
    return;
  }
}

ezResult ezTestBaseClass::ExecuteTest(std::deque<ezSubTestEntry>& SubTestsToExecute, double& out_fTotalDuration)
{
  out_fTotalDuration = 0.0;
  #if EZ_PLATFORM_WINDOWS
    ezUInt64 uiStartTime, uiEndTime, uiFrequency;
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&uiStartTime));
  #endif

  ezLog::AddLogWriter(LogWriter);

  const ezInt32 iTestErrorCount = ezTestFramework::GetErrorCount();

  ezTestFramework::Output(ezTestOutput::BeginBlock, "Executing Test: '%s'", GetTestName());
  
  if (DoTestInitialization() == EZ_FAILURE)
  {
    ezTestFramework::Output(ezTestOutput::EndBlock, "");
    return EZ_FAILURE;
  }

  for (ezUInt32 st = 0; st < SubTestsToExecute.size(); ++st)
  {
    SubTestsToExecute[st].m_fTestDuration = 0.0;

    if (!SubTestsToExecute[st].m_bEnableTest)
    {
      //ezTestFramework::Output(ezTestOutput::Message, "Skipping deactivated Sub-Test: '%s'", SubTestsToExecute[st].m_szSubTestName);
      continue;
    }

    const ezInt32 iSubTestErrorCount = ezTestFramework::GetErrorCount();

    ezTestFramework::Output(ezTestOutput::BeginBlock, "Executing Sub-Test: '%s'", SubTestsToExecute[st].m_szSubTestName);
    
    if (DoSubTestInitialization(SubTestsToExecute[st].m_iSubTestIdentifier) == EZ_SUCCESS)
    {
      const double fDuration = DoSubTestRun(SubTestsToExecute[st].m_iSubTestIdentifier, SubTestsToExecute[st].m_szSubTestName);
      DoSubTestDeInitialization(SubTestsToExecute[st].m_iSubTestIdentifier);

      SubTestsToExecute[st].m_fTestDuration = fDuration;

      const ezUInt32 uiMin = (ezUInt32) (fDuration / 1000.0 / 60.0);
      const ezUInt32 uiSec = (ezUInt32) (fDuration / 1000.0 - uiMin * 60.0);
      const ezUInt32 uiMS  = (ezUInt32) (fDuration - uiSec * 1000.0);

      ezTestFramework::Output(ezTestOutput::Duration, "%i:%02i:%03i", uiMin, uiSec, uiMS);

      if (iSubTestErrorCount == ezTestFramework::GetErrorCount())
        ezTestFramework::Output(ezTestOutput::Success, "Sub-Test '%s' succeeded.", SubTestsToExecute[st].m_szSubTestName);
    }

    ezTestFramework::Output(ezTestOutput::EndBlock, "");
  }

  ezLog::RemoveLogWriter(LogWriter);

  DoTestDeInitialization();

  #if EZ_PLATFORM_WINDOWS
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&uiEndTime));
    QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&uiFrequency));

    out_fTotalDuration = ((double) (uiEndTime - uiStartTime) / (double) uiFrequency) * 1000.0;
  #endif

  const ezUInt32 uiMin = (ezUInt32) (out_fTotalDuration / 1000.0 / 60.0);
  const ezUInt32 uiSec = (ezUInt32) (out_fTotalDuration / 1000.0 - uiMin * 60.0);
  const ezUInt32 uiMS  = (ezUInt32) (out_fTotalDuration - uiSec * 1000.0);

  ezTestFramework::Output(ezTestOutput::Duration, "%i:%02i:%03i", uiMin, uiSec, uiMS);

  ezResult Result = EZ_SUCCESS;

  if (iTestErrorCount == ezTestFramework::GetErrorCount())
  {
    Result = EZ_SUCCESS;
    ezTestFramework::Output(ezTestOutput::Success, "Test '%s' succeeded", GetTestName());
  }
  else
  {
    Result = EZ_FAILURE;
    ezTestFramework::Output(ezTestOutput::Error, "Test '%s' failed: %i Errors.", GetTestName(), ezTestFramework::GetErrorCount() - iTestErrorCount);
  }

  ezTestFramework::Output(ezTestOutput::EndBlock, "");
  return Result;
}
