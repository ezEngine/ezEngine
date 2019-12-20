#include <GameEngineTestPCH.h>

#include "TypeScriptTest.h"
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>

static ezGameEngineTestTypeScript s_GameEngineTestTypeScript;

const char* ezGameEngineTestTypeScript::GetTestName() const
{
  return "TypeScript Tests";
}

ezGameEngineTestApplication* ezGameEngineTestTypeScript::CreateApplication()
{
  m_pOwnApplication = EZ_DEFAULT_NEW(ezGameEngineTestApplication_TypeScript);
  return m_pOwnApplication;
}

void ezGameEngineTestTypeScript::SetupSubTests()
{
  AddSubTest("Basics", SubTests::Basics);
}

ezResult ezGameEngineTestTypeScript::InitializeSubTest(ezInt32 iIdentifier)
{
  m_iFrame = -1;

  if (iIdentifier == SubTests::Basics)
  {
    m_pOwnApplication->SubTestBasicsSetup();
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezTestAppRun ezGameEngineTestTypeScript::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  ++m_iFrame;

  if (iIdentifier == SubTests::Basics)
    return m_pOwnApplication->SubTestBasisExec(m_iFrame);

  EZ_ASSERT_NOT_IMPLEMENTED;
  return ezTestAppRun::Quit;
}

//////////////////////////////////////////////////////////////////////////

ezGameEngineTestApplication_TypeScript::ezGameEngineTestApplication_TypeScript()
  : ezGameEngineTestApplication("TypeScript")
{
}

//////////////////////////////////////////////////////////////////////////

void ezGameEngineTestApplication_TypeScript::SubTestBasicsSetup()
{
  LoadScene("TypeScript/AssetCache/Common/Scenes/TypeScripting.ezObjectGraph");
}

ezTestAppRun ezGameEngineTestApplication_TypeScript::SubTestBasisExec(ezInt32 iCurFrame)
{
  if (Run() == ezApplication::Quit)
    return ezTestAppRun::Quit;

  switch (iCurFrame)
  {
    //case 15:
    //  EZ_TEST_IMAGE(0, 50);
    //  break;
    //case 30:
    //  EZ_TEST_IMAGE(1, 50);
    //  break;

    case 60:
      //EZ_TEST_IMAGE(2, 50);
      return ezTestAppRun::Quit;
  }

  return ezTestAppRun::Continue;
}
