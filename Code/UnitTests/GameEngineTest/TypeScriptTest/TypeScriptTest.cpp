#include <GameEngineTestPCH.h>

#include "TypeScriptTest.h"
#include <Core/Messages/EventMessage.h>
#include <Core/Scripting/DuktapeFunction.h>
#include <Core/Scripting/DuktapeHelper.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <TypeScriptPlugin/Components/TypeScriptComponent.h>

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

static int Duk_TestFailure(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  const char* szFile = duk.GetStringValue(0);
  const ezInt32 iLine = duk.GetIntValue(1);
  const char* szFunction = duk.GetStringValue(2);
  const char* szMsg = duk.GetStringValue(3);

  ezTestBool(false, "TypeScript Test Failed", szFile, iLine, szFunction, szMsg);

  return duk.ReturnVoid();
}

void ezGameEngineTestApplication_TypeScript::SubTestBasicsSetup()
{
  LoadScene("TypeScript/AssetCache/Common/Scenes/TypeScripting.ezObjectGraph");

  EZ_LOCK(m_pWorld->GetWriteMarker());
  ezTypeScriptComponentManager* pMan = m_pWorld->GetOrCreateComponentManager<ezTypeScriptComponentManager>();

  pMan->GetTsBinding().GetDukTapeContext().RegisterGlobalFunction("ezTestFailure", Duk_TestFailure, 4);
}

ezTestAppRun ezGameEngineTestApplication_TypeScript::SubTestBasisExec(ezInt32 iCurFrame)
{
  if (Run() == ezApplication::Quit)
    return ezTestAppRun::Quit;

  EZ_LOCK(m_pWorld->GetWriteMarker());

  ezGameObject* pTests = nullptr;
  if (m_pWorld->TryGetObjectWithGlobalKey("Tests", pTests) == false)
  {
    EZ_TEST_FAILURE("Failed to retrieve TypeScript Tests-Object", "");
    return ezTestAppRun::Quit;
  }

  switch (iCurFrame)
  {
    case 0:
    {
      ezMsgGenericEvent msg;
      msg.m_sMessage = "TestVec3";
      pTests->SendMessageRecursive(msg);

      EZ_TEST_STRING(msg.m_sMessage, "done");
    }
    break;

      //case 15:
      //  EZ_TEST_IMAGE(0, 50);
      //  break;

    case 10:
      return ezTestAppRun::Quit;
  }

  return ezTestAppRun::Continue;
}
