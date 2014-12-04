#include <PCH.h>
#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestSetup.h>

int main(int argc, char **argv)
{
  ezTestSetup::InitTestFramework("CoreTest", "Core Tests", argc, (const char**) argv);
  
  // *** Add additional output handlers and configurations here. ***

  while (ezTestSetup::RunTests() == ezTestAppRun::Continue)
  {
  }

  const ezInt32 iFailedTests = ezTestSetup::GetFailedTestCount();
  
  ezTestSetup::DeInitTestFramework();
  return iFailedTests;
}