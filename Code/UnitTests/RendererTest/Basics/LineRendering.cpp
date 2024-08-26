#include <RendererTest/RendererTestPCH.h>

#include "Basics.h"

ezTestAppRun ezRendererTestBasics::SubtestLineRendering()
{
  BeginFrame();
  BeginCommands("RendererTest");

  ezColor clear(0, 0, 0, 0);
  BeginRendering(clear);

  RenderLineObjects(ezShaderBindFlags::Default);

  EZ_TEST_LINE_IMAGE(0, 150);
  EndRendering();
  EndCommands();
  EndFrame();

  return m_iFrame < 0 ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}
