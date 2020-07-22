#include <TestFrameworkPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <TestFramework/Framework/TestFramework.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezRegisterSimpleTestHelper);

void ezSimpleTestGroup::AddSimpleTest(const char* szName, SimpleTestFunc TestFunc)
{
  SimpleTestEntry e;
  e.m_szName = szName;
  e.m_Func = TestFunc;

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
  {
    AddSubTest(m_SimpleTests[i].m_szName, i);
  }
}

ezTestAppRun ezSimpleTestGroup::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  // until the block name is properly set, use the test name instead
  ezTestFramework::s_szTestBlockName = m_SimpleTests[iIdentifier].m_szName;

  EZ_PROFILE_SCOPE(m_SimpleTests[iIdentifier].m_szName);
  m_SimpleTests[iIdentifier].m_Func();

  ezTestFramework::s_szTestBlockName = "";
  return ezTestAppRun::Quit;
}

ezResult ezSimpleTestGroup::InitializeSubTest(ezInt32 iIdentifier)
{
  // initialize everything up to 'core'
  ezStartup::StartupCoreSystems();
  return EZ_SUCCESS;
}

ezResult ezSimpleTestGroup::DeInitializeSubTest(ezInt32 iIdentifier)
{
  // shut down completely
  ezStartup::ShutdownCoreSystems();
  ezMemoryTracker::DumpMemoryLeaks();
  return EZ_SUCCESS;
}


EZ_STATICLINK_FILE(TestFramework, TestFramework_Framework_SimpleTest);
