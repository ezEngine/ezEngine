#include <TestFramework/PCH.h>
#include <TestFramework/Framework/TestFramework.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Time.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezTestBaseClass);

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

double ezTestBaseClass::DoSubTestRun(ezInt32 iIdentifier)
{
  double fDuration = 0.0;

  if (iIdentifier < 0 || iIdentifier >= (ezInt32)m_Entries.size())
  {
    ezTestFramework::Output(ezTestOutput::Error, "Test with identifier '%d' not found.", iIdentifier);
    return fDuration;
  }

  try
  {
    ezTime StartTime = ezSystemTime::Now();

    RunSubTest(iIdentifier);

    fDuration = (ezSystemTime::Now() - StartTime).GetMilliseconds();

  }
  catch(...)
  {
    ezTestFramework::Output(ezTestOutput::Error, "Exception during sub-test '%s'.", m_Entries[iIdentifier].m_szName);
  }

  return fDuration;
}


EZ_STATICLINK_FILE(TestFramework, TestFramework_Framework_TestBaseClass);

