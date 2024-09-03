#include <TestFramework/TestFrameworkPCH.h>

#include <TestFramework/Framework/TestFramework.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezTestBaseClass);

const char* ezTestBaseClass::GetSubTestName(ezInt32 iIdentifier) const
{
  const ezInt32 entryIndex = FindEntryForIdentifier(iIdentifier);

  if (entryIndex < 0)
  {
    ezLog::Error("Tried to access retrieve sub-test name using invalid identifier.");
    return "";
  }

  return m_Entries[entryIndex].m_szName;
}

void ezTestBaseClass::UpdateConfiguration(ezTestConfiguration& ref_config) const
{
  // If the configuration hasn't been set yet this is the first instance of ezTestBaseClass being called
  // to fill in the configuration and we thus have to do so.
  // Derived classes can have more information (e.g.GPU info) and there is no way to know which instance
  // of ezTestBaseClass may have additional information so we ask all of them and each one early outs
  // if the information it knows about is already present.
  if (ref_config.m_uiInstalledMainMemory == 0)
  {
    const ezSystemInformation& pSysInfo = ezSystemInformation::Get();
    ref_config.m_uiInstalledMainMemory = pSysInfo.GetInstalledMainMemory();
    ref_config.m_uiMemoryPageSize = pSysInfo.GetMemoryPageSize();
    ref_config.m_uiCPUCoreCount = pSysInfo.GetCPUCoreCount();
    ref_config.m_sPlatformName = pSysInfo.GetPlatformName();
    ref_config.m_b64BitOS = pSysInfo.Is64BitOS();
    ref_config.m_b64BitApplication = EZ_ENABLED(EZ_PLATFORM_64BIT);
    ref_config.m_sBuildConfiguration = pSysInfo.GetBuildConfiguration();
    ref_config.m_iDateTime = ezTimestamp::CurrentTimestamp().GetInt64(ezSIUnitOfTime::Second);
    ref_config.m_iRCSRevision = ezTestFramework::GetInstance()->GetSettings().m_iRevision;
    ref_config.m_sHostName = pSysInfo.GetHostName();
  }
}

void ezTestBaseClass::MapImageNumberToString(const char* szTestName, const ezSubTestEntry& subTest, ezUInt32 uiImageNumber, ezStringBuilder& out_sString) const
{
  out_sString.SetFormat("{0}_{1}_{2}", szTestName, subTest.m_szSubTestName, ezArgI(uiImageNumber, 3, true));
  out_sString.ReplaceAll(" ", "_");
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
  try
  {
    if (InitializeTest() == EZ_FAILURE)
    {
      ezTestFramework::Output(ezTestOutput::Error, "Test Initialization failed.");
      return EZ_FAILURE;
    }
  }
  catch (...)
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
  catch (...)
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
  catch (...)
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
  catch (...)
  {
    ezTestFramework::Output(ezTestOutput::Error, "Exception during sub-test de-initialization.");
  }
}

ezTestAppRun ezTestBaseClass::DoSubTestRun(ezInt32 iIdentifier, double& fDuration, ezUInt32 uiInvocationCount)
{
  fDuration = 0.0;

  ezTestAppRun ret = ezTestAppRun::Quit;

  try
  {
    ezTime StartTime = ezTime::Now();

    ret = RunSubTest(iIdentifier, uiInvocationCount);

    fDuration = (ezTime::Now() - StartTime).GetMilliseconds();
  }
  catch (...)
  {
    const ezInt32 iEntry = FindEntryForIdentifier(iIdentifier);

    if (iEntry >= 0)
      ezTestFramework::Output(ezTestOutput::Error, "Exception during sub-test '%s'.", m_Entries[iEntry].m_szName);
    else
      ezTestFramework::Output(ezTestOutput::Error, "Exception during unknown sub-test.");
  }

  return ret;
}

ezInt32 ezTestBaseClass::FindEntryForIdentifier(ezInt32 iIdentifier) const
{
  for (ezInt32 i = 0; i < (ezInt32)m_Entries.size(); ++i)
  {
    if (m_Entries[i].m_iIdentifier == iIdentifier)
    {
      return i;
    }
  }

  return -1;
}


