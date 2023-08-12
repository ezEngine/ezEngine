#pragma once

#include "../TestClass/TestClass.h"
#include <RendererCore/Textures/Texture2DResource.h>

class ezRendererTestAdvancedFeatures : public ezGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "AdvancedFeatures"; }

private:
  enum SubTests
  {
    ST_ReadRenderTarget,
    ST_VertexShaderRenderTargetArrayIndex,
    //ST_BackgroundResourceLoading,
  };

  enum ImageCaptureFrames
  {
    DefaultCapture = 5,

  };

  virtual void SetupSubTests() override
  {
    AddSubTest("01 - ReadRenderTarget", SubTests::ST_ReadRenderTarget);
    AddSubTest("02 - VertexShaderRenderTargetArrayIndex", SubTests::ST_VertexShaderRenderTargetArrayIndex);
  }

  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  void ReadRenderTarget();
  void VertexShaderRenderTargetArrayIndex();

private:
  ezShaderResourceHandle m_hShader2;

  ezGALTextureHandle m_hTexture2D;
  ezGALResourceViewHandle m_hTexture2DMips[4];
  ezGALTextureHandle m_hTexture2DArray;
};
