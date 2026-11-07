#pragma once

#include "../TestClass/TestClass.h"
#include <RendererCore/Textures/Texture2DResource.h>

class ezRendererTestBasics : public ezGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "Basics"; }

private:
  enum SubTests
  {
    ST_ClearScreen,
    ST_RasterizerStates,
    ST_BlendStates,
    ST_Textures2D,
    ST_Textures3D,
    ST_BlockCompressedMipChain2D,
    ST_BlockCompressedMipChain3D,
    ST_TexturesCube,
    ST_LineRendering,
  };

  virtual void SetupSubTests() override
  {
    const ezGALDeviceCapabilities& caps = GetDeviceCapabilities();

    AddSubTest("Clear Screen", SubTests::ST_ClearScreen);
    AddSubTest("Rasterizer States", SubTests::ST_RasterizerStates);
    AddSubTest("Blend States", SubTests::ST_BlendStates);
    AddSubTest("2D Textures", SubTests::ST_Textures2D);
    // AddSubTest("3D Textures", SubTests::ST_Textures3D); /// \todo 3D Texture support is currently not implemented
    const auto bc4Support = caps.m_FormatSupport[ezGALResourceFormat::BC4UNormalized];
    if (bc4Support.IsSet(ezGALResourceFormatSupport::Texture))
    {
      AddSubTest("Block-Compressed Mip Chain 2D", SubTests::ST_BlockCompressedMipChain2D);
      AddSubTest("Block-Compressed Mip Chain 3D", SubTests::ST_BlockCompressedMipChain3D);
    }
    AddSubTest("Cube Textures", SubTests::ST_TexturesCube);
    AddSubTest("Line Rendering", SubTests::ST_LineRendering);
  }

  virtual ezResult InitializeTest() override { return EZ_SUCCESS; }
  virtual ezResult DeInitializeTest() override { return EZ_SUCCESS; }
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;

  ezTestAppRun SubtestClearScreen();
  ezTestAppRun SubtestRasterizerStates();
  ezTestAppRun SubtestBlendStates();
  ezTestAppRun SubtestTextures2D();
  ezTestAppRun SubtestTextures3D();
  ezTestAppRun SubtestBlockCompressedMipChain2D();
  ezTestAppRun SubtestBlockCompressedMipChain3D();
  ezTestAppRun SubtestTexturesCube();
  ezTestAppRun SubtestLineRendering();

  void RenderObjects(ezBitflags<ezShaderBindFlags> ShaderBindFlags);
  void RenderLineObjects(ezBitflags<ezShaderBindFlags> ShaderBindFlags);

  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override
  {
    m_iFrame = uiInvocationCount;

    if (iIdentifier == SubTests::ST_ClearScreen)
      return SubtestClearScreen();

    if (iIdentifier == SubTests::ST_RasterizerStates)
      return SubtestRasterizerStates();

    if (iIdentifier == SubTests::ST_BlendStates)
      return SubtestBlendStates();

    if (iIdentifier == SubTests::ST_Textures2D)
      return SubtestTextures2D();

    if (iIdentifier == SubTests::ST_Textures3D)
      return SubtestTextures3D();

    if (iIdentifier == SubTests::ST_BlockCompressedMipChain2D)
      return SubtestBlockCompressedMipChain2D();

    if (iIdentifier == SubTests::ST_BlockCompressedMipChain3D)
      return SubtestBlockCompressedMipChain3D();

    if (iIdentifier == SubTests::ST_TexturesCube)
      return SubtestTexturesCube();

    if (iIdentifier == SubTests::ST_LineRendering)
      return SubtestLineRendering();

    return ezTestAppRun::Quit;
  }

  ezMeshBufferResourceHandle m_hSphere;
  ezMeshBufferResourceHandle m_hSphere2;
  ezMeshBufferResourceHandle m_hTorus;
  ezMeshBufferResourceHandle m_hLongBox;
  ezMeshBufferResourceHandle m_hLineBox;
  ezTexture2DResourceHandle m_hTexture2D;
  ezTextureCubeResourceHandle m_hTextureCube;
  ezGALTextureHandle m_hBlockCompressedTexture;
};
