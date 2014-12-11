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
  };

  virtual void SetupSubTests() override
  {
    AddSubTest("Clear Screen", SubTests::ST_ClearScreen);
    AddSubTest("Simple Mesh", SubTests::ST_SimpleMesh);
  }


  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override
  {
    m_uiFrame = 0;

    if (ezGraphicsTest::InitializeSubTest(iIdentifier).Failed())
      return EZ_FAILURE;

    if (iIdentifier == SubTests::ST_ClearScreen)
    {
      return SetupRenderer(320, 240);
    }

    if (iIdentifier == SubTests::ST_SimpleMesh)
    {
      if (SetupRenderer().Failed())
        return EZ_FAILURE;

      m_hSphere = CreateSphere(3);

      return EZ_SUCCESS;
    }

    return EZ_SUCCESS;
  }

  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override
  {
    if (iIdentifier == SubTests::ST_SimpleMesh)
    {
      m_hSphere.Invalidate();
    }

    ShutdownRenderer();

    if (ezGraphicsTest::DeInitializeSubTest(iIdentifier).Failed())
      return EZ_FAILURE;

    return EZ_SUCCESS;
  }

  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier) override
  {
    ++m_uiFrame;

    if (iIdentifier == SubTests::ST_ClearScreen)
    {
      BeginFrame();

      switch (m_uiFrame)
      {
      case 1:
        ClearScreen(ezColor::GetBlack());
        break;
      case 2:
        ClearScreen(ezColor::GetCornflowerBlue()); // The original!
        break;
      case 3:
        ClearScreen(ezColor::GetRed());
        break;
      case 4:
        ClearScreen(ezColor::GetGreen());
        break;
      case 5:
        ClearScreen(ezColor::GetYellow());
        break;
      case 6:
        ClearScreen(ezColor::GetBlue());
        break;
      case 7:
        ClearScreen(ezColor::GetWhite());
        break;
      case 8:
        ClearScreen(ezColor(0.5f, 0.5f, 0.5f, 0.5f));
        break;
      }

      EZ_TEST_IMAGE(0);

      EndFrame();

      return m_uiFrame < 7 ? ezTestAppRun::Continue : ezTestAppRun::Quit;
    }

    if (iIdentifier == SubTests::ST_SimpleMesh)
    {
      BeginFrame();
      ClearScreen(ezColor::GetCornflowerBlue()); // The original!

      ezMat4 mTransform;
      mTransform.SetIdentity();

      RenderObject(m_hSphere, mTransform);

      EZ_TEST_IMAGE(30);

      EndFrame();

      return m_uiFrame < 1 ? ezTestAppRun::Continue : ezTestAppRun::Quit;
    }

    return ezTestAppRun::Quit;
  }

  ezUInt32 m_uiFrame;
  ezMeshBufferResourceHandle m_hSphere;
};

static ezRendererTestBasics g_Test;

