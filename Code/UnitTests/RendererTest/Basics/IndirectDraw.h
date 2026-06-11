#pragma once

#include "../TestClass/TestClass.h"

class ezRendererTestIndirectDraw : public ezGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "IndirectDraw"; }

private:
  enum SubTests
  {
    ST_DrawInstancedIndirect,
    ST_DrawIndexedInstancedIndirect,
    ST_DrawIndexedInstancedIndirectOffset,
    ST_DispatchIndirect,
  };

  enum ImageCaptureFrames
  {
    DefaultCapture = 5,
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  void DrawInstancedIndirect();
  void DrawIndexedInstancedIndirect();
  void DrawIndexedInstancedIndirectOffset();
  void DispatchIndirect();

  void FillIndirectArgsViaCompute(ezUInt32 arg0, ezUInt32 arg1, ezUInt32 arg2, ezUInt32 arg3, ezUInt32 arg4 = 0);
  void CaptureImage();

  // Resources
  static constexpr ezUInt32 s_uiRTSize = 32;

  ezGALBufferHandle m_hIndirectArgsBuffer;
  ezMeshBufferResourceHandle m_hTriangleMesh;
  ezMeshBufferResourceHandle m_hIndexedTriangleMesh;

  // For dispatch test
  ezGALTextureHandle m_hDispatchOutputTexture;

  // Shaders
  ezShaderResourceHandle m_hFillArgsShader;
  ezShaderResourceHandle m_hDrawShader;
  ezShaderResourceHandle m_hInstancedDrawShader;
  ezShaderResourceHandle m_hIndexedInstancedDrawShader;
  ezShaderResourceHandle m_hDispatchWriteShader;
};
