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
class ezRendererTestReadback : public ezGraphicsTest
{
  using SUPER = ezGraphicsTest;

public:
  virtual const char* GetTestName() const override { return "Readback"; }

private:
  virtual void SetupSubTests() override;

  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult GetImage(ezImage& ref_img, const ezSubTestEntry& subTest, ezUInt32 uiImageNumber) override;
  virtual void MapImageNumberToString(const char* szTestName, const ezSubTestEntry& subTest, ezUInt32 uiImageNumber, ezStringBuilder& out_sString) const override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;
  ezTestAppRun Readback(ezUInt32 uiInvocationCount);

  void CompareReadbackImage(ezImage&& image);
  void CompareUploadImage();

private:
  ezDynamicArray<ezEnum<ezGALResourceFormat>> m_TestableFormats;
  ezDynamicArray<ezString> m_TestableFormatStrings;

  ezGALResourceFormat::Enum m_Format = ezGALResourceFormat::Invalid;
  ezShaderResourceHandle m_hUVColorShader;
  ezShaderResourceHandle m_hUVColorIntShader;
  ezShaderResourceHandle m_hUVColorUIntShader;
  ezShaderResourceHandle m_hUVColorDepthShader;
  ezShaderResourceHandle m_hTexture2DShader;
  ezShaderResourceHandle m_hTexture2DIntShader;
  ezShaderResourceHandle m_hTexture2DUIntShader;
  ezGALTextureHandle m_hTexture2DReadback;
  ezGALTextureHandle m_hTexture2DUpload;
  mutable ezImage m_ReadBackResult;
  mutable ezString m_sReadBackReferenceImage;
  bool m_bReadbackInProgress = true;
};
