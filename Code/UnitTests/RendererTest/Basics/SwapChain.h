#pragma once

#include "../TestClass/TestClass.h"
#include <RendererCore/Textures/Texture2DResource.h>

class ezRendererTestSwapChain : public ezGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "SwapChain"; }

private:
  enum SubTests
  {
    ST_ColorOnly,
    ST_D16,
    ST_D24S8,
    ST_D32,
    ST_NoVSync,
    ST_ResizeWindow,
  };

  virtual void SetupSubTests() override
  {
    AddSubTest("Color Only", SubTests::ST_ColorOnly);
    AddSubTest("Depth D16", SubTests::ST_D16);
    AddSubTest("Depth D24S8", SubTests::ST_D24S8);
    AddSubTest("Depth D32", SubTests::ST_D32);
    AddSubTest("No VSync", SubTests::ST_NoVSync);
    AddSubTest("Resize Window", SubTests::ST_ResizeWindow);
  }

  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;

  void ResizeTest(ezUInt32 uiInvocationCount);
  ezTestAppRun BasicRenderLoop(ezInt32 iIdentifier, ezUInt32 uiInvocationCount);

  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override
  {
    ++m_iFrame;

    switch (iIdentifier)
    {
      case SubTests::ST_ResizeWindow:
        ResizeTest(uiInvocationCount);
       [[fallthrough]];
      case SubTests::ST_ColorOnly:
      case SubTests::ST_D16:
      case SubTests::ST_D24S8:
      case SubTests::ST_D32:
      case SubTests::ST_NoVSync:
        return BasicRenderLoop(iIdentifier, uiInvocationCount);
      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
        break;
    }
    return ezTestAppRun::Quit;
  }

  ezSizeU32 m_CurrentWindowSize = ezSizeU32(320, 240);
  ezInt32 m_iFrame;
};
