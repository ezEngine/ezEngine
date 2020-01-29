#include <GameEngineTestPCH.h>

#include "TypeScriptTest.h"
#include <Core/Messages/CommonMessages.h>
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
  AddSubTest("Vec2", SubTests::Vec2);
  AddSubTest("Vec3", SubTests::Vec3);
  AddSubTest("Quat", SubTests::Quat);
  AddSubTest("Mat3", SubTests::Mat3);
  AddSubTest("Mat4", SubTests::Mat4);
  AddSubTest("Transform", SubTests::Transform);
  AddSubTest("Color", SubTests::Color);
  AddSubTest("GameObject", SubTests::GameObject);
  AddSubTest("Component", SubTests::Component);
}

ezResult ezGameEngineTestTypeScript::InitializeSubTest(ezInt32 iIdentifier)
{
  if (iIdentifier >= SubTests::Vec2 && iIdentifier <= SubTests::Component)
  {
    m_pOwnApplication->SubTestBasicsSetup();
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezTestAppRun ezGameEngineTestTypeScript::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  if (iIdentifier >= SubTests::Vec2 && iIdentifier <= SubTests::Component)
    return m_pOwnApplication->SubTestBasisExec(iIdentifier);

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

ezTestAppRun ezGameEngineTestApplication_TypeScript::SubTestBasisExec(ezInt32 iIdentifier)
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

  const char* szMsg[] =
    {
      "TestVec2",
      "TestVec3",
      "TestQuat",
      "TestMat3",
      "TestMat4",
      "TestTransform",
      "TestColor",
      "TestGameObject",
      "TestComponent",
    };

  ezMsgGenericEvent msg;
  msg.m_sMessage = szMsg[iIdentifier];
  pTests->SendMessageRecursive(msg);

  EZ_TEST_STRING(msg.m_sMessage, "done");

  return ezTestAppRun::Quit;
}
