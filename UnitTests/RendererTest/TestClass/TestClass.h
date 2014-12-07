#pragma once

#include <TestFramework/Framework/TestBaseClass.h>
#include <RendererFoundation/Device/Device.h>
#include <System/Window/Window.h>

class ezImage;

class ezGraphicsTest : public ezTestBaseClass
{
public:
  ezGraphicsTest();

protected:
  virtual void SetupSubTests() override { }
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier) override { return ezTestAppRun::Quit; }

  virtual ezResult InitializeTest() override { return EZ_SUCCESS; }
  virtual ezResult DeInitializeTest() override { return EZ_SUCCESS; }
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;

protected:
  ezResult SetupRenderer(ezUInt32 uiResolutionX = 640, ezUInt32 uiResolutionY = 480);
  void ShutdownRenderer();
  void ClearScreen(const ezColor& color = ezColor::GetBlack());

  void BeginFrame();
  void EndFrame(bool bImageComparison = true);
  void GetScreenshot(ezImage& img);

  ezWindow* m_pWindow;
  ezGALDevice* m_pDevice;
  ezUInt32 m_uiFrameCounter;

  ezGALRenderTargetConfigHandle m_hBBRT;
  ezGALRasterizerStateHandle m_hRasterizerState;
  ezGALDepthStencilStateHandle m_hDepthStencilState;
};