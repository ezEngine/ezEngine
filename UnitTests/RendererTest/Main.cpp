#include <PCH.h>
#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestSetup.h>
#include <RendererCore/Textures/TextureResource.h>

int main(int argc, char **argv)
{
  ezTestSetup::InitTestFramework("RendererTest", "Renderer Tests", argc, (const char**) argv);

  ezTextureResource::s_bForceFullQualityAlways = true; // never allow to use low-res textures
  
  // *** Add additional output handlers and configurations here. ***

  while (ezTestSetup::RunTests() == ezTestAppRun::Continue)
  {
  }

  const ezInt32 iFailedTests = ezTestSetup::GetFailedTestCount();
  
  ezTestSetup::DeInitTestFramework();
  return iFailedTests;
}