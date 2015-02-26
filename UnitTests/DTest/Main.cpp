#include <PCH.h>
#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestSetup.h>

ezInt32 ezConstructionCounter::s_iConstructions = 0;
ezInt32 ezConstructionCounter::s_iDestructions = 0;
ezInt32 ezConstructionCounter::s_iConstructionsLast = 0;
ezInt32 ezConstructionCounter::s_iDestructionsLast = 0;

ezSimpleTestGroup* ezCreateSimpleTestGroup(const char* szName)
{
  return EZ_DEFAULT_NEW(ezSimpleTestGroup)(szName);
}

void ezDestroySimpleTestGroup(ezSimpleTestGroup* pTestGroup)
{
  EZ_DEFAULT_DELETE(pTestGroup);
}

ezRegisterSimpleTestHelper* ezCreateRegisterSimpleTestHelper(ezSimpleTestGroup* pTestGroup, const char* szTestName, ezSimpleTestGroup::SimpleTestFunc Func)
{
  return EZ_DEFAULT_NEW(ezRegisterSimpleTestHelper)(pTestGroup, szTestName, Func);
}

void ezDestroyRegisterSimpleTestHelper(ezRegisterSimpleTestHelper* pHelper)
{
  EZ_DEFAULT_DELETE(pHelper);
}

// Defined on D side
void ezInitDTests();
void ezDeinitDTests(); 

int main(int argc, char **argv)
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