#include <TestFrameworkPCH.h>

#include <TestFramework/Framework/TestFramework.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezTestBaseClass);

const char* ezTestBaseClass::GetSubTestName(ezInt32 iIdentifier) const
{
  if (iIdentifier < 0 || static_cast<std::size_t>(iIdentifier) > m_Entries.size())
  {
    ezLog::Error("Tried to access retrieve sub-test name using invalid identifier.");
    return "";
  }

  return m_Entries[iIdentifier].m_szName;
}

void ezTestBaseClass::UpdateConfiguration(ezTestConfiguration& config) const
{
  // If the configuration hasn't been set yet this is the first instance of ezTestBaseClass being called
  // to fill in the configuration and we thus have to do so.
  // Derived classes can have more information (e.g.GPU info) and there is no way to know which instance
  // of ezTestBaseClass may have additional information so we ask all of them and each one early outs
  // if the information it knows about is already present.
  if (config.m_uiInstalledMainMemory == 0)
  {
    const ezSystemInformation& pSysInfo = ezSystemInformation::Get();
    config.m_uiInstalledMainMemory = pSysInfo.GetInstalledMainMemory();
    config.m_uiMemoryPageSize = pSysInfo.GetMemoryPageSize();
    config.m_uiCPUCoreCount = pSysInfo.GetCPUCoreCount();
    config.m_sPlatformName = pSysInfo.GetPlatformName();
    config.m_b64BitOS = pSysInfo.Is64BitOS();
    config.m_b64BitApplication = EZ_ENABLED(EZ_PLATFORM_64BIT);
    config.m_sBuildConfiguration = pSysInfo.GetBuildConfiguration();
    config.m_iDateTime = ezTimestamp::CurrentTimestamp().GetInt64(ezSIUnitOfTime::Second);
    config.m_iRCSRevision = ezTestFramework::GetInstance()->GetSettings().m_iRevision;
    config.m_sHostName = pSysInfo.GetHostName();
  }
}

void ezTestBaseClass::MapImageNumberToString(const char* szTestName, const char* szSubTestName, ezUInt32 uiImageNumber, ezStringBuilder& out_String) const
{
  out_String.Format("{0}_{1}_{2}", szTestName, szSubTestName, ezArgI(uiImageNumber, 3, true));
  out_String.ReplaceAll(" ", "_");
}

void ezTestBaseClass::ClearSubTests()
{
  m_Entries.clear();
}

void ezTestBaseClass::AddSubTest(const char* szName, ezInt32 iIdentifier)
{
  EZ_ASSERT_DEV(szName != nullptr, "Sub test name must not be nullptr");

  TestEntry e;
  e.m_szName = szName;
  e.m_iIdentifier = iIdentifier;

  m_Entries.push_back(e);
}

ezResult ezTestBaseClass::DoTestInitialization()
{
#if EZ_ENABLED(EZ_TESTFRAMEWORK_SUPPORT_EXCEPTIONS)
  try
#endif
  {
    if (InitializeTest() == EZ_FAILURE)
    {
      ezTestFramework::Output(ezTestOutput::Error, "Test Initialization failed.");
      return EZ_FAILURE;
    }
  }
#if EZ_ENABLED(EZ_TESTFRAMEWORK_SUPPORT_EXCEPTIONS)
  catch (...)
  {
    ezTestFramework::Output(ezTestOutput::Error, "Exception during test initialization.");
    return EZ_FAILURE;
  }
#endif

  return EZ_SUCCESS;
}

void ezTestBaseClass::DoTestDeInitialization()
{
#if EZ_ENABLED(EZ_TESTFRAMEWORK_SUPPORT_EXCEPTIONS)
  try
#endif

  {
    if (DeInitializeTest() == EZ_FAILURE)
      ezTestFramework::Output(ezTestOutput::Error, "Test DeInitialization failed.");
  }
#if EZ_ENABLED(EZ_TESTFRAMEWORK_SUPPORT_EXCEPTIONS)
  catch (...)
  {
    ezTestFramework::Output(ezTestOutput::Error, "Exception during test de-initialization.");
  }
#endif
}

ezResult ezTestBaseClass::DoSubTestInitialization(ezInt32 iIdentifier)
{
#if EZ_ENABLED(EZ_TESTFRAMEWORK_SUPPORT_EXCEPTIONS)
  try
#endif
  {
    if (InitializeSubTest(iIdentifier) == EZ_FAILURE)
    {
      ezTestFramework::Output(ezTestOutput::Error, "Sub-Test Initialization failed, skipping Test.");
      return EZ_FAILURE;
    }
  }
#if EZ_ENABLED(EZ_TESTFRAMEWORK_SUPPORT_EXCEPTIONS)
  catch (...)
  {
    ezTestFramework::Output(ezTestOutput::Error, "Exception during sub-test initialization.");
    return EZ_FAILURE;
  }
#endif

  return EZ_SUCCESS;
}

void ezTestBaseClass::DoSubTestDeInitialization(ezInt32 iIdentifier)
{
#if EZ_ENABLED(EZ_TESTFRAMEWORK_SUPPORT_EXCEPTIONS)
  try
#endif
  {
    if (DeInitializeSubTest(iIdentifier) == EZ_FAILURE)
      ezTestFramework::Output(ezTestOutput::Error, "Sub-Test De-Initialization failed.");
  }
#if EZ_ENABLED(EZ_TESTFRAMEWORK_SUPPORT_EXCEPTIONS)
  catch (...)
  {
    ezTestFramework::Output(ezTestOutput::Error, "Exception during sub-test de-initialization.");
  }
#endif
}

ezTestAppRun ezTestBaseClass::DoSubTestRun(ezInt32 iIdentifier, double& fDuration, ezUInt32 uiInvocationCount)
{
  fDuration = 0.0;

  ezTestAppRun ret = ezTestAppRun::Quit;

#if EZ_ENABLED(EZ_TESTFRAMEWORK_SUPPORT_EXCEPTIONS)
  try
#endif
  {
    ezTime StartTime = ezTime::Now();

    ret = RunSubTest(iIdentifier, uiInvocationCount);

    fDuration = (ezTime::Now() - StartTime).GetMilliseconds();
  }
#if EZ_ENABLED(EZ_TESTFRAMEWORK_SUPPORT_EXCEPTIONS)
  catch (...)
  {
    ezInt32 iEntry = -1;

    for (ezInt32 i = 0; i < (ezInt32)m_Entries.size(); ++i)
    {
      if (m_Entries[i].m_iIdentifier == iIdentifier)
      {
        iEntry = i;
        break;
      }
    }

    if (iEntry >= 0)
      ezTestFramework::Output(ezTestOutput::Error, "Exception during sub-test '%s'.", m_Entries[iEntry].m_szName);
    else
      ezTestFramework::Output(ezTestOutput::Error, "Exception during unknown sub-test.");
  }
#endif

  return ret;
}


EZ_STATICLINK_FILE(TestFramework, TestFramework_Framework_TestBaseClass);

