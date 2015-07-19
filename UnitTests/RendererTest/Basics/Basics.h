#include <PCH.h>
#include "../TestClass/TestClass.h"
#include <RendererCore/Textures/TextureResource.h>

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
    ST_TexturesCube,
  };

  virtual void SetupSubTests() override
  {
    AddSubTest("Clear Screen", SubTests::ST_ClearScreen);
    AddSubTest("Rasterizer States", SubTests::ST_RasterizerStates);
    //AddSubTest("Blend States", SubTests::ST_BlendStates);
    AddSubTest("2D Textures", SubTests::ST_Textures2D);
    //AddSubTest("3D Textures", SubTests::ST_Textures3D);
    AddSubTest("Cube Textures", SubTests::ST_TexturesCube);
  }


  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;

  ezTestAppRun SubtestClearScreen();
  ezTestAppRun SubtestRasterizerStates();
  ezTestAppRun SubtestBlendStates();
  ezTestAppRun SubtestTextures2D();
  ezTestAppRun SubtestTextures3D();
  ezTestAppRun SubtestTexturesCube();

  void RenderObjects(ezBitflags<ezShaderBindFlags> ShaderBindFlags);

  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier) override
  {
    ++m_iFrame;

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

    if (iIdentifier == SubTests::ST_TexturesCube)
      return SubtestTexturesCube();

    return ezTestAppRun::Quit;
  }

  ezInt32 m_iFrame;
  ezMeshBufferResourceHandle m_hSphere;
  ezMeshBufferResourceHandle m_hSphere2;
  ezMeshBufferResourceHandle m_hTorus;
  ezMeshBufferResourceHandle m_hLongBox;
  ezTextureResourceHandle m_hTexture;
};


