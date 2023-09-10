#include <RendererTest/RendererTestPCH.h>

#include <Foundation/Utilities/CommandLineUtils.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestSetup.h>

#include <RendererTest/Advanced/OffscreenRenderer.h>

EZ_TESTFRAMEWORK_ENTRY_POINT_BEGIN("RendererTest", "Renderer Tests")
{
  ezTextureUtils::s_bForceFullQualityAlways = true; // never allow to use low-res textures

  ezCommandLineUtils cmd;
  cmd.SetCommandLine(argc, (const char**)argv, ezCommandLineUtils::PreferOsArgs);

  if (cmd.GetBoolOption("-offscreen"))
  {
    ezOffscreenRendererTest offScreenTest;
    offScreenTest.SetCommandLineArguments(argc, (const char**)argv);
    ezRun(&offScreenTest); // Life cycle & run method calling
    const int iReturnCode = offScreenTest.GetReturnCode();
    // shutdown with exit code
    ezTestSetup::DeInitTestFramework(true);
    return iReturnCode;
  }
}
EZ_TESTFRAMEWORK_ENTRY_POINT_END()
