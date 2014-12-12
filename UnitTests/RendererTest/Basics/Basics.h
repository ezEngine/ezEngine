#include <PCH.h>
#include "../TestClass/TestClass.h"

class ezRendererTestBasics : public ezGraphicsTest
{
public:

  virtual const char* GetTestName() const override { return "Basics"; }

private:
  enum SubTests
  {
    ST_ClearScreen,
    ST_SimpleMesh,
    ST_RasterizerStates,
    ST_BlendStates,
  };

  virtual void SetupSubTests() override
  {
    AddSubTest("Clear Screen", SubTests::ST_ClearScreen);
    AddSubTest("Simple Mesh", SubTests::ST_SimpleMesh);
    AddSubTest("Rasterizer States", SubTests::ST_RasterizerStates);
    //AddSubTest("Blend States", SubTests::ST_BlendStates);
  }


  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;

  ezTestAppRun SubtestClearScreen();
  ezTestAppRun SubtestSimpleMesh();
  ezTestAppRun SubtestRasterizerStates();
  ezTestAppRun SubtestBlendStates();

  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier) override
  {
    ++m_uiFrame;

    if (iIdentifier == SubTests::ST_ClearScreen)
      return SubtestClearScreen();

    if (iIdentifier == SubTests::ST_SimpleMesh)
      return SubtestSimpleMesh();

    if (iIdentifier == SubTests::ST_RasterizerStates)
      return SubtestRasterizerStates();

    if (iIdentifier == SubTests::ST_BlendStates)
      return SubtestBlendStates();

    return ezTestAppRun::Quit;
  }

  ezUInt32 m_uiFrame;
  ezMeshBufferResourceHandle m_hSphere;
};


