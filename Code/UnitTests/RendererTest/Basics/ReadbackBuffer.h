#pragma once

#include "../TestClass/TestClass.h"
#include <RendererCore/Textures/Texture2DResource.h>

/// \brief Tests texture readback from the GPU
///
/// Only renderable textures are tested.
/// The test first renders a pattern into the 8x8 texture.
/// Then the texture is read back and uploaded again to another texture.
/// The readback result is converted to RGBA32float and compared to a reference image. As these only change depending on channel count the MapImageNumberToString function is overwritten to always point to the same images.
/// Finally the original an re-uploaded texture is rendered to the screen and the result is again compared to a reference image.
///
/// The subtest list is dynamic, only formats that support render and sample are tested.
class ezRendererTestReadbackBuffer : public ezGraphicsTest
{
  using SUPER = ezGraphicsTest;

public:
  virtual const char* GetTestName() const override { return "Readback Buffer"; }

private:
  enum SubTests
  {
    ST_VertexBuffer,
    ST_IndexBuffer,
    ST_TexelBuffer,
    ST_StructuredBuffer,
    ST_ByteAddressBuffer,
  };

  virtual void SetupSubTests() override;

  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;
  ezTestAppRun ReadbackBuffer(ezUInt32 uiInvocationCount);

private:
  ezShaderResourceHandle m_hComputeShader;

  ezDynamicArray<ezUInt8> m_BufferData;
  ezGALBufferHandle m_hBufferReadback;
  bool m_bReadbackInProgress = true;
  ezGALReadbackBufferHelper m_Readback;
};
