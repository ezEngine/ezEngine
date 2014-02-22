#include <PCH.h>
#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestSetup.h>

int main(int argc, char **argv)
{
  ezTestSetup::InitTestFramework("CoreTest", "Core Tests", argc, (const char**) argv);
  
  // *** Add additional output handlers and configurations here. ***

  ezInt32 iFailedTests = ezTestSetup::RunTests(argc, argv);
  
  ezTestSetup::DeInitTestFramework();
  return iFailedTests;
}