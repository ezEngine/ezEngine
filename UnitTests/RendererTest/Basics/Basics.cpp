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
  };

  virtual void SetupSubTests() override
  {
    AddSubTest("Clear Screen", SubTests::ST_ClearScreen);
  }

  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override
  {
    if (ezGraphicsTest::InitializeSubTest(iIdentifier).Failed())
      return EZ_FAILURE;

    if (iIdentifier == SubTests::ST_ClearScreen)
    {
      m_uiFrames = 0;

      return SetupRenderer();
    }

    return EZ_SUCCESS;
  }

  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override
  {
    if (iIdentifier == SubTests::ST_ClearScreen)
    {
      ShutdownRenderer();
    }

    if (ezGraphicsTest::DeInitializeSubTest(iIdentifier).Failed())
      return EZ_FAILURE;

    return EZ_SUCCESS;
  }

  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier) override
  {
    if (iIdentifier == SubTests::ST_ClearScreen)
    {
      BeginFrame();

      ClearScreen(ezColor::GetCornflowerBlue()); // The original!



      EndFrame(m_uiFrames > 5);

      ++m_uiFrames;
      return m_uiFrames < 10 ? ezTestAppRun::Continue : ezTestAppRun::Quit;
    }

    return ezTestAppRun::Quit;
  }

  ezUInt32 m_uiFrames;
};

static ezRendererTestBasics g_Test;

