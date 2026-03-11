#include <RendererTest/RendererTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <RendererTest/TestClass/SimpleRendererTest.h>

void ezSimpleRendererTestGroup::AddSimpleRendererTest(const char* szName, SimpleRendererTestFunc testFunc)
{
  SimpleRendererTestEntry e;
  e.m_szName = szName;
  e.m_Func = testFunc;

  for (ezUInt32 i = 0; i < m_SimpleRendererTests.size(); ++i)
  {
    if ((strcmp(m_SimpleRendererTests[i].m_szName, e.m_szName) == 0) && (m_SimpleRendererTests[i].m_Func == e.m_Func))
      return;
  }

  m_SimpleRendererTests.push_back(e);
}

void ezSimpleRendererTestGroup::SetupSubTests()
{
  for (ezUInt32 i = 0; i < m_SimpleRendererTests.size(); ++i)
  {
    AddSubTest(m_SimpleRendererTests[i].m_szName, i);
  }
}

ezTestAppRun ezSimpleRendererTestGroup::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  // until the block name is properly set, use the test name instead
  ezTestFramework::s_szTestBlockName = m_SimpleRendererTests[iIdentifier].m_szName;

  EZ_PROFILE_SCOPE(m_SimpleRendererTests[iIdentifier].m_szName);
  m_SimpleRendererTests[iIdentifier].m_Func();

  ezTestFramework::s_szTestBlockName = "";
  return ezTestAppRun::Quit;
}
