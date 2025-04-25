#include <GameEngineTest/GameEngineTestPCH.h>

#ifdef BUILDSYSTEM_ENABLE_ANGELSCRIPT_SUPPORT

#  include <AngelScript/include/angelscript.h>
#  include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>
#  include <Core/Messages/CommonMessages.h>
#  include <Core/WorldSerializer/WorldReader.h>
#  include <Foundation/CodeUtils/Preprocessor.h>
#  include <Foundation/IO/FileSystem/FileReader.h>

#  include "AngelScriptTest.h"

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
  AddSubTest("Arrays", SubTests::Arrays);
  AddSubTest("EntryPoints", SubTests::EntryPoints);
  AddSubTest("World", SubTests::World);
  AddSubTest("Messaging", SubTests::Messaging);
  AddSubTest("GameObject", SubTests::GameObject);
  AddSubTest("Physics", SubTests::Physics);
  AddSubTest("Misc", SubTests::Misc);
}

ezResult ezGameEngineTestAngelScript::InitializeSubTest(ezInt32 iIdentifier)
{
  m_pOwnApplication->SubTestBasicsSetup();
  return EZ_SUCCESS;
}

ezTestAppRun ezGameEngineTestAngelScript::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::Types:
      m_pOwnApplication->RunTestScript("Tests/Types/TypesTest.as");
      return ezTestAppRun::Quit;
    case SubTests::Strings:
      m_pOwnApplication->RunTestScript("Tests/Types/StringsTest.as");
      return ezTestAppRun::Quit;
    case SubTests::Arrays:
      m_pOwnApplication->RunTestScript("Tests/Types/ArraysTest.as");
      return ezTestAppRun::Quit;
    default:
      return m_pOwnApplication->SubTestBasisExec(GetSubTestName(iIdentifier));
  }
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

void ezGameEngineTestApplication_AngelScript::RunTestScript(ezStringView sScriptPath)
{
  // Load and process test script
  {
    ezStringBuilder sTestCode;
    ezStringBuilder sProcessedCode;
    {
      ezFileReader read;
      if (read.Open(sScriptPath).Failed())
      {
        ezLog::Error("Failed to open file '{}'.", sScriptPath);
        return;
      }
      sTestCode.ReadAll(read);
    }
    if (ezAngelScriptEngineSingleton::PreprocessCode(sScriptPath, sTestCode, &sProcessedCode, nullptr).Failed())
    {
      ezLog::Error("Failed to preprocess code '{}'.", sScriptPath);
      return;
    }
    m_sCode = sProcessedCode;
    m_sCode.Split(true, m_Lines, "\n");
  }

  EZ_LOCK(m_pWorld->GetWriteMarker());

  auto pAsEngine = ezAngelScriptEngineSingleton::GetSingleton();
  asIScriptModule* pModule = pAsEngine->SetModuleCode("ScriptTest", m_sCode, true);
  if (pModule == nullptr)
  {
    ezLog::Error("Failed to create AngelScript module.");
    return;
  }
  asIScriptContext* m_pContext = pAsEngine->GetEngine()->CreateContext();
  EZ_SCOPE_EXIT(m_pContext->Release(););
  AS_CHECK(m_pContext->SetExceptionCallback(asMETHOD(ezGameEngineTestApplication_AngelScript, TestScriptExceptionCallback), this, asCALL_THISCALL));

  asIScriptFunction* func = pModule->GetFunctionByName("ExecuteTests");
  m_pContext->Prepare(func);

  EZ_TEST_INT(m_pContext->Execute(), asEXECUTION_FINISHED);
}

void ezGameEngineTestApplication_AngelScript::TestScriptExceptionCallback(asIScriptContext* pContext)
{
  ezLog::Error("AS Exception '{}'", pContext->GetExceptionString());

  const ezUInt32 uiNumLevels = pContext->GetCallstackSize();
  for (ezUInt32 i = 0; i < uiNumLevels; ++i)
  {
    const char* szSection = nullptr;
    const ezInt32 iOriginalLine = pContext->GetLineNumber(i, nullptr, &szSection);

    ezInt32 iLine = iOriginalLine;
    ezStringView sSection = szSection;
    ezAngelScriptEngineSingleton::FindCorrectSectionAndLine(m_Lines, iLine, sSection);

    ezStringBuilder line("  ");
    if (asIScriptFunction* pFunc = pContext->GetFunction(i))
    {
      if (!ezStringUtils::IsNullOrEmpty(pFunc->GetNamespace()))
      {
        line.Append(pFunc->GetNamespace(), "::");
      }

      if (!ezStringUtils::IsNullOrEmpty(pFunc->GetObjectName()))
      {
        line.Append(pFunc->GetObjectName(), "::");
      }

      line.AppendFormat("{}() [{}: {}] -> {}", pFunc->GetName(), sSection, iLine, m_Lines[iOriginalLine - 1]);

      ezLog::Error(line);
    }
    else
    {
      line.AppendFormat("<nested call> [{}: {}] -> {}", sSection, iLine, m_Lines[iOriginalLine - 1]);
    }
  }
}

#endif
