#include <TestFramework/PCH.h>
#include <TestFramework/Framework/TestFramework.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/MemoryTracker.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezRegisterSimpleTestHelper);

void ezSimpleTestGroup::AddSimpleTest(const char* szName, SimpleTestFunc TestFunc)
{
  SimpleTestEntry e;
  e.m_szName = szName;
  e.m_Func = TestFunc;
  e.m_ProfilingId = ezProfilingSystem::CreateId(szName);

  for (ezUInt32 i = 0; i < m_SimpleTests.size(); ++i)
  {
    if ((strcmp(m_SimpleTests[i].m_szName, e.m_szName) == 0) && (m_SimpleTests[i].m_Func == e.m_Func))
      return;
  }

  m_SimpleTests.push_back(e);
}

void ezSimpleTestGroup::SetupSubTests()
{
  for (ezUInt32 i = 0; i < m_SimpleTests.size(); ++i)
    AddSubTest(m_SimpleTests[i].m_szName, i);
}

ezTestAppRun ezSimpleTestGroup::RunSubTest(ezInt32 iIdentifier)
{
  EZ_PROFILE(m_SimpleTests[iIdentifier].m_ProfilingId);
  m_SimpleTests[iIdentifier].m_Func();
  return ezTestAppRun::Quit;
}

ezResult ezSimpleTestGroup::InitializeSubTest(ezInt32 iIdentifier)
{
  // initialize everything up to 'core'
  ezStartup::StartupCore();
  return EZ_SUCCESS;
}

ezResult ezSimpleTestGroup::DeInitializeSubTest(ezInt32 iIdentifier)
{
  // shut down completely
  ezStartup::ShutdownCore();
  ezMemoryTracker::DumpMemoryLeaks();
  return EZ_SUCCESS;
}


EZ_STATICLINK_FILE(TestFramework, TestFramework_Framework_SimpleTest);

