#include <EditorTest/EditorTestPCH.h>

#include "GenerateCompile.h"
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/CodeGen/CppSettings.h>
#include <GuiFoundation/Action/ActionManager.h>

static ezEditorTestGenerateCompile s_EditorTestGenerateAndCompile;

const char* ezEditorTestGenerateCompile::GetTestName() const
{
  return "Generate and Compile";
}

void ezEditorTestGenerateCompile::SetupSubTests()
{
  AddSubTest("Generate and Compile", SubTests::GenerateAndCompile);
}

ezResult ezEditorTestGenerateCompile::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return EZ_FAILURE;

  if (SUPER::OpenProject("Data/Samples/PacMan").Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezResult ezEditorTestGenerateCompile::DeInitializeTest()
{
  if (SUPER::DeInitializeTest().Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezTestAppRun ezEditorTestGenerateCompile::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  if (iIdentifier == SubTests::GenerateAndCompile)
  {
    ProcessEvents();

    ezCppSettings cpp;
    if (!EZ_TEST_RESULT(cpp.Load()))
      return ezTestAppRun::Quit;

    ezString sBuildDir = ezCppProject::GetBuildDir(cpp);
    if (!EZ_TEST_RESULT(ezOSFile::DeleteFolder(sBuildDir)))
      return ezTestAppRun::Quit;

#if _MSC_VER >= 1930
    cpp.m_Compiler = ezCppSettings::Compiler::Vs2022;
#elif
    cpp.m_Compiler = ezCppSettings::Compiler::Vs2019;
#endif

    if (!EZ_TEST_RESULT(ezCppProject::RunCMake(cpp)))
      return ezTestAppRun::Quit;

    EZ_TEST_BOOL(ezCppProject::ExistsProjectCMakeListsTxt());
    EZ_TEST_BOOL(ezCppProject::ExistsSolution(cpp));

    EZ_TEST_RESULT(ezCppProject::BuildCodeIfNecessary(cpp));
    ProcessEvents();
  }

  return ezTestAppRun::Quit;
}

ezResult ezEditorTestGenerateCompile::InitializeSubTest(ezInt32 iIdentifier)
{
  return EZ_SUCCESS;
}

ezResult ezEditorTestGenerateCompile::DeInitializeSubTest(ezInt32 iIdentifier)
{
  ezDocumentManager::CloseAllDocuments();
  return EZ_SUCCESS;
}
