#include <TestFramework/PCH.h>
#include <TestFramework/Framework/TestFramework.h>
#include <Foundation/Configuration/Startup.h>

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

void ezSimpleTestGroup::RunSubTest(ezInt32 iIdentifier)
{
  // initialize everything up to 'core'
  ezStartup::StartupCore();

  {
    EZ_PROFILE(m_SimpleTests[iIdentifier].m_ProfilingId);
    m_SimpleTests[iIdentifier].m_Func();
  }

  // shut down completely
  ezStartup::ShutdownBase();
}

