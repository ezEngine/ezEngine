#include <DTest/PCH.h>
#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestSetup.h>

ezSimpleTestGroup* ezCreateSimpleTestGroup(const char* szName)
{
  return EZ_DEFAULT_NEW(ezSimpleTestGroup, szName);
}

void ezDestroySimpleTestGroup(ezSimpleTestGroup* pTestGroup)
{
  EZ_DEFAULT_DELETE(pTestGroup);
}

ezRegisterSimpleTestHelper* ezCreateRegisterSimpleTestHelper(ezSimpleTestGroup* pTestGroup, const char* szTestName, ezSimpleTestGroup::SimpleTestFunc Func)
{
  return EZ_DEFAULT_NEW(ezRegisterSimpleTestHelper, pTestGroup, szTestName, Func);
}

void ezDestroyRegisterSimpleTestHelper(ezRegisterSimpleTestHelper* pHelper)
{
  EZ_DEFAULT_DELETE(pHelper);
}

void ezSetTestBlockName(const char* szTestBlockName)
{
  ezTestFramework::s_szTestBlockName = szTestBlockName;
}

void ezIncreaseAssertCount()
{
  ezTestFramework::s_iAssertCounter++;
}

int ezTestMain(int argc, char **argv)
{
  ezTestSetup::InitTestFramework("FoundationTest", "Foundation Tests", argc, (const char**) argv);
  
  // *** Add additional output handlers and configurations here. ***

  while (ezTestSetup::RunTests() == ezTestAppRun::Continue)
  {
  }

  const ezInt32 iFailedTests = ezTestSetup::GetFailedTestCount();
  
  ezTestSetup::DeInitTestFramework();
  return iFailedTests;
}