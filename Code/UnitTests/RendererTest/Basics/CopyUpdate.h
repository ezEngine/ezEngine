#pragma once

#include "../TestClass/TestClass.h"

/// Tests buffer and texture copy and update operations on the GAL command encoder.
///
/// Each sub-test creates source and/or destination resources, issues the operation
/// under test, reads back the destination and binary-compares the readback bytes
/// against the expected CPU data. No image comparison is needed since the result
/// is validated directly on the bytes.
class ezRendererTestCopyUpdate : public ezGraphicsTest
{
  using SUPER = ezGraphicsTest;

public:
  virtual const char* GetTestName() const override { return "CopyUpdate"; }

private:
  enum SubTests
  {
    ST_CopyBuffer,
    ST_CopyTexture,
    ST_CopyTextureArray,
    ST_CopyTextureCube,
    ST_CopyTextureCubeArray,
    ST_CopyTextureNpot,
    ST_CopyTextureBC1,
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeTest() override;
  virtual ezResult DeInitializeTest() override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  void RunCopyBuffer();
  void RunCopyTextureRegion(ezInt32 iIdentifier);
  void RunCopyTexture();
  void RunUpdateTexture(ezInt32 iIdentifier);
  void RunUpdateTextureForNextFrame(ezInt32 iIdentifier);

  /// Initializes a texture source + destination pair. Texture2D is treated as a one-layer texture.
  void SetupTextureCopyUpdatePair(
    ezGALTextureType::Enum type, ezUInt32 uiArraySize, ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiMipLevels, ezUInt32 uiPaddingBytes, ezGALResourceFormat::Enum format = ezGALResourceFormat::BGRAUByteNormalizedsRGB);

  void VerifyBufferReadback(ezGALBufferHandle hBuffer, ezArrayPtr<const ezUInt8> expected);
  /// Reads back all slices and mips of a texture and compares each against expectedLayers[slice].
  void VerifyTextureSliceReadback(ezGALTextureHandle hTexture, ezArrayPtr<const ezImage> expectedLayers);

  /// Width and height for the power-of-two texture sub-tests. 8x8 keeps the data small and easy to inspect.
  static constexpr ezUInt32 s_uiTextureSize = 8;
  static constexpr ezUInt32 s_uiNpotTextureWidth = 10;
  static constexpr ezUInt32 s_uiNpotTextureHeight = 6;
  /// Number of mip levels for the texture sub-tests so we exercise the multi-mip branch of CopyTexture.
  static constexpr ezUInt32 s_uiTextureMips = 3;
  /// Buffer size in bytes for the buffer sub-tests.
  static constexpr ezUInt32 s_uiBufferSize = 256;

  ezGALBufferHandle m_hBufferSource;
  ezGALBufferHandle m_hBufferDest;
  ezGALTextureHandle m_hTextureSource;
  ezGALTextureHandle m_hTextureDest;

  ezDynamicArray<ezUInt8> m_BufferSourceData;
  /// Per-slice CPU copy of the source texture.
  ezDynamicArray<ezImage> m_TextureSourceImages;
  /// Per-slice CPU copy of the destination texture. Each texture test updates this alongside the GPU texture.
  ezDynamicArray<ezImage> m_TextureDestImages;

  ezGALReadbackBufferHelper m_BufferReadback;
  ezGALReadbackTextureHelper m_TextureReadback;
};
