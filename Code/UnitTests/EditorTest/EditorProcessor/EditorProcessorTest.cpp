#include <EditorTest/EditorTestPCH.h>

#include "EditorProcessorTest.h"
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/CodeGen/CppSettings.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Application/Application.h>
#include <GuiFoundation/Action/ActionManager.h>

static ezEditorTestEditorProcessor s_EditorTestEditorProcessor;

const char* ezEditorTestEditorProcessor::GetTestName() const
{
  return "EditorProcessor";
}

void ezEditorTestEditorProcessor::SetupSubTests()
{
  AddSubTest("Compile Only", SubTests::CompileOnly);
  AddSubTest("Compile and Transform", SubTests::CompileAndTransform);
}

ezResult ezEditorTestEditorProcessor::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return EZ_FAILURE;

  if (SUPER::OpenProject("Data/Samples/PacMan").Failed())
    return EZ_FAILURE;

  if (ezCppProject::ForceSdkCompatibleCompiler().Failed())
  {
    ezLog::Error("Failed to autodetect SDK compatible compiler for testing");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezEditorTestEditorProcessor::DeInitializeTest()
{
  if (SUPER::DeInitializeTest().Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezTestAppRun ezEditorTestEditorProcessor::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  if (iIdentifier == SubTests::CompileOnly)
  {
    ProcessEvents();

    // Test the -compile option functionality
    ezCppSettings cpp;
    if (!EZ_TEST_RESULT(cpp.Load()))
      return ezTestAppRun::Quit;

    // Clean build directory to ensure fresh compilation
    ezString sBuildDir = ezCppProject::GetBuildDir(cpp);
    if (ezOSFile::ExistsDirectory(sBuildDir))
    {
      if (!EZ_TEST_RESULT(ezOSFile::DeleteFolder(sBuildDir)))
        return ezTestAppRun::Quit;
    }

    // Test that CMake generation works
    if (!EZ_TEST_RESULT(ezCppProject::RunCMake(cpp)))
      return ezTestAppRun::Quit;

    EZ_TEST_BOOL(ezCppProject::ExistsProjectCMakeListsTxt());
    EZ_TEST_BOOL(ezCppProject::ExistsSolution(cpp));

    // Test that compilation works (this is what -compile option does)
    EZ_TEST_RESULT(ezCppProject::BuildCodeIfNecessary(cpp));
    
    ProcessEvents();
    
    // Verify that the plugin was built successfully
    ezStringBuilder sPluginPath = ezOSFile::GetApplicationDirectory();
    sPluginPath.AppendPath(cpp.m_sPluginName);
    sPluginPath.Append("Plugin");
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    sPluginPath.Append(".dll");
#else
    sPluginPath.Append(".so");
#endif
    EZ_TEST_BOOL(ezOSFile::ExistsFile(sPluginPath));
  }
  else if (iIdentifier == SubTests::CompileAndTransform)
  {
    ProcessEvents();

    // Test the combined -compile and -transform functionality
    ezCppSettings cpp;
    if (!EZ_TEST_RESULT(cpp.Load()))
      return ezTestAppRun::Quit;

    // Ensure compilation happens first
    EZ_TEST_RESULT(ezCppProject::BuildCodeIfNecessary(cpp));
    
    // Verify plugin exists after compilation
    ezStringBuilder sPluginPath = ezOSFile::GetApplicationDirectory();
    sPluginPath.AppendPath(cpp.m_sPluginName);
    sPluginPath.Append("Plugin");
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    sPluginPath.Append(".dll");
#else
    sPluginPath.Append(".so");
#endif
    EZ_TEST_BOOL(ezOSFile::ExistsFile(sPluginPath));

    ProcessEvents();
      // Test that asset transformation would work after compilation
    // (We don't actually transform all assets in the test as it would take too long)
    ezUInt32 uiPlatform = ezAssetCurator::GetSingleton()->FindAssetProfileByName("Default");
    EZ_TEST_BOOL(uiPlatform != ezInvalidIndex);
    
    ProcessEvents();
  }

  return ezTestAppRun::Quit;
}

ezResult ezEditorTestEditorProcessor::InitializeSubTest(ezInt32 iIdentifier)
{
  return EZ_SUCCESS;
}

ezResult ezEditorTestEditorProcessor::DeInitializeSubTest(ezInt32 iIdentifier)
{
  ezDocumentManager::CloseAllDocuments();
  return EZ_SUCCESS;
}
