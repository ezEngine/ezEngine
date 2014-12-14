#include <TestFramework/PCH.h>
#include <TestFramework/Framework/TestFramework.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Time.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Configuration/Startup.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezTestBaseClass);

void ezTestBaseClass::UpdateConfiguration(ezTestConfiguration& config) const
{
  // If the configuration hasn't been set yet this is the first instance of ezTestBaseClass being called
  // to fill in the configuration and we thus have to do so.
  // Derived classes can have more information (e.g.GPU info) and there is no way to know which instance
  // of ezTestBaseClass may have additional information so we ask all of them and each one early outs
  // if the information it knows about is already present.
  if (config.m_uiInstalledMainMemory == 0)
  {
    ezStartup::StartupBase();

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

void ezTestBaseClass::ClearSubTests()
{
  m_Entries.clear();
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

ezTestAppRun ezTestBaseClass::DoSubTestRun(ezInt32 iIdentifier, double& fDuration)
{
  fDuration = 0.0;

  ezTestAppRun ret = ezTestAppRun::Quit;

  try
  {
    ezTime StartTime = ezTime::Now();

    ret = RunSubTest(iIdentifier);

    fDuration = (ezTime::Now() - StartTime).GetMilliseconds();

  }
  catch(...)
  {
    ezInt32 iEntry = -1;

    for (ezInt32 i = 0; i < (ezInt32) m_Entries.size(); ++i)
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

  return ret;
}


EZ_STATICLINK_FILE(TestFramework, TestFramework_Framework_TestBaseClass);

