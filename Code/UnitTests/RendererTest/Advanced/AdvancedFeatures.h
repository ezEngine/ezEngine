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

  void RenderToScreen(ezUInt32 uiRenderTargetClearMask, ezRectFloat viewport, ezDelegate<void(ezGALRenderCommandEncoder*)> func);
  void RenderCube(ezRectFloat viewport, ezMat4 mMVP, ezUInt32 uiRenderTargetClearMask, ezGALResourceViewHandle hSRV);

  void ReadRenderTarget();
  void VertexShaderRenderTargetArrayIndex();

private:
  ezInt32 m_iFrame = 0;
  bool m_bCaptureImage = false;
  ezHybridArray<ezUInt32, 8> m_ImgCompFrames;

  ezShaderResourceHandle m_hShader2;

  ezGALTextureHandle m_hTexture2D;
  ezGALResourceViewHandle m_hTexture2DMips[4];
  ezGALTextureHandle m_hTexture2DArray;

  ezMeshBufferResourceHandle m_hCubeUV;


};
