#include <GameEngineTest/GameEngineTestPCH.h>

#ifdef BUILDSYSTEM_ENABLE_ANGELSCRIPT_SUPPORT

#  include "AngelScriptTest.h"
#  include <Core/Messages/CommonMessages.h>
#  include <Core/Messages/EventMessage.h>
#  include <Core/Scripting/DuktapeFunction.h>
#  include <Core/Scripting/DuktapeHelper.h>
#  include <Core/WorldSerializer/WorldReader.h>
#  include <Foundation/IO/FileSystem/FileReader.h>

static ezGameEngineTestAngelScript s_GameEngineTestAngelScript;

const char* ezGameEngineTestAngelScript::GetTestName() const
{
  return "AngelScript Tests";
}

ezGameEngineTestApplication* ezGameEngineTestAngelScript::CreateApplication()
{
  m_pOwnApplication = EZ_DEFAULT_NEW(ezGameEngineTestApplication_AngelScript);
  return m_pOwnApplication;
}

void ezGameEngineTestAngelScript::SetupSubTests()
{
  AddSubTest("Types", SubTests::Types);
  AddSubTest("Strings", SubTests::Strings);
  AddSubTest("EntryPoints", SubTests::EntryPoints);
  AddSubTest("World", SubTests::World);
  AddSubTest("Messaging", SubTests::Messaging);
  AddSubTest("GameObject", SubTests::GameObject);
}

ezResult ezGameEngineTestAngelScript::InitializeSubTest(ezInt32 iIdentifier)
{
  m_pOwnApplication->SubTestBasicsSetup();
  return EZ_SUCCESS;
}

ezTestAppRun ezGameEngineTestAngelScript::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  return m_pOwnApplication->SubTestBasisExec(GetSubTestName(iIdentifier));
}

//////////////////////////////////////////////////////////////////////////

ezGameEngineTestApplication_AngelScript::ezGameEngineTestApplication_AngelScript()
  : ezGameEngineTestApplication("AngelScript")
{
}

void ezGameEngineTestApplication_AngelScript::SubTestBasicsSetup()
{
  LoadScene("AngelScript/AssetCache/Common/Scenes/Main.ezBinScene").IgnoreResult();
}

ezTestAppRun ezGameEngineTestApplication_AngelScript::SubTestBasisExec(const char* szSubTestName)
{
  Run();
  if (ShouldApplicationQuit())
    return ezTestAppRun::Quit;

  EZ_LOCK(m_pWorld->GetWriteMarker());

  ezGameObject* pTests = nullptr;
  if (m_pWorld->TryGetObjectWithGlobalKey("Tests", pTests) == false)
  {
    EZ_TEST_FAILURE("Failed to retrieve AngelScript Tests-Object", "");
    return ezTestAppRun::Quit;
  }

  const ezStringBuilder sMsg(szSubTestName, "Test");

  ezMsgGenericEvent msg;
  msg.m_sMessage.Assign(sMsg);
  pTests->SendMessageRecursive(msg);

  if (msg.m_sMessage == ezTempHashedString("repeat"))
    return ezTestAppRun::Continue;

  EZ_TEST_STRING(msg.m_sMessage, "done");

  return ezTestAppRun::Quit;
}

#endif
