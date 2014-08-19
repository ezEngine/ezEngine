#include <PCH.h>
#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestSetup.h>

ezInt32 ezConstructionCounter::s_iConstructions = 0;
ezInt32 ezConstructionCounter::s_iDestructions = 0;
ezInt32 ezConstructionCounter::s_iConstructionsLast = 0;
ezInt32 ezConstructionCounter::s_iDestructionsLast = 0;

int main(int argc, char **argv)
{
  ezTestSetup::InitTestFramework("ToolsFoundationTest", "Tools Foundation Tests", argc, (const char**) argv);
  
  // *** Add additional output handlers and configurations here. ***

  ezInt32 iFailedTests = ezTestSetup::RunTests(argc, argv);
  
  ezTestSetup::DeInitTestFramework();
  return iFailedTests;
}