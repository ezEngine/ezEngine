#include <EditorTest/EditorTestPCH.h>

#include "Samples.h"
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/CodeGen/CppSettings.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/StringConversion.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <TestFramework/Framework/TestFramework.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
// only show/enable this test on Windows, since we can only compile game plugins there
static ezEditorTestSamples s_EditorTestSamples;
#endif

const char* ezEditorTestSamples::GetTestName() const
{
  return "Samples";
}

void ezEditorTestSamples::SetupSubTests()
{
  AddSubTest("Asteroids", SubTests::ST_Asteroids);
  AddSubTest("PacMan", SubTests::ST_PacMan);
  AddSubTest("RTS", SubTests::ST_RTS);
}

ezResult ezEditorTestSamples::InitializeTest()
{
  return EZ_SUCCESS;
}

ezResult ezEditorTestSamples::DeInitializeTest()
{
  return EZ_SUCCESS;
}

ezResult ezEditorTestSamples::InitializeSubTest(ezInt32 iIdentifier)
{
  if (SUPER::InitializeTest().Failed())
    return EZ_FAILURE;

  switch (iIdentifier)
  {
    case SubTests::ST_PacMan:
      m_sProjectName = "PacMan";
      break;
    case SubTests::ST_Asteroids:
      m_sProjectName = "Asteroids";
      break;
    case SubTests::ST_RTS:
      m_sProjectName = "RTS";
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  ezStringBuilder sPath("Data/Samples/", m_sProjectName);

  if (SUPER::OpenProject(sPath).Failed())
    return EZ_FAILURE;

  if (ezCppProject::ForceSdkCompatibleCompiler().Failed())
  {
    ezLog::Error("Failed to autodetect SDK compatible compiler for testing");
    return EZ_FAILURE;
  }

  ezCppSettings cpp;
  cpp.Load().AssertSuccess("Failed to load C++ project settings");

  // update sources, in case there was a version change
  ezUInt32 uiNumFilesCopied = 0;
  ezCppProject::PopulateWithDefaultSources(cpp, &uiNumFilesCopied).AssertSuccess();


  EZ_TEST_INT_MSG(uiNumFilesCopied, 0, "You need to open the '%s' sample and regenerate the C++ solution.", m_sProjectName.GetData());

  ezPreferences::SaveApplicationPreferences();

  return EZ_SUCCESS;
}

ezResult ezEditorTestSamples::DeInitializeSubTest(ezInt32 iIdentifier)
{
  if (!m_sProjectPath.IsEmpty())
  {
    // The build results take up vast amounts of memory and prolong test result upload.
    ezStringBuilder buildOutput = m_sProjectPath;
    buildOutput.AppendPath("CppSource", "Build");
    ezOSFile::DeleteFolder(buildOutput).IgnoreResult();
    m_sProjectPath.Clear();
  }

  if (SUPER::DeInitializeTest().Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}


ezTestAppRun ezEditorTestSamples::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  ProcessEvents();

  EZ_TEST_STATUS(GenerateAndCompile());

  ProcessEvents();
  return ezTestAppRun::Quit;
}
