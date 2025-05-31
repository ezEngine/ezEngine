#include <EditorTest/EditorTestPCH.h>

#include "EditorProcessorTest.h"
#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/Process.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <TestFramework/Framework/TestFramework.h>

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
  switch (iIdentifier)
  {
    case SubTests::CompileOnly:
      return TestCompileOnly();
    case SubTests::CompileAndTransform:
      return TestCompileAndTransform();
  }

  return ezTestAppRun::Quit;
}

ezString ezEditorTestEditorProcessor::GetEditorProcessorPath() const
{
  ezStringBuilder path;
  // Get the directory where EditorTest.exe is located
  path = ezOSFile::GetApplicationDirectory();
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  path.AppendPath("ezEditorProcessor.exe");
#else
  path.AppendPath("ezEditorProcessor");
#endif;
  return path;
}

ezString ezEditorTestEditorProcessor::GetPacManProjectPath() const
{
  ezStringBuilder path;
  ezStringView sdkRoot = ezFileSystem::GetSdkRootDirectory();
  if (sdkRoot.IsEmpty())
  {
    EZ_TEST_FAILURE("Failed to resolve SDK root directory", "Cannot locate PacMan project");
    return "";
  }
  path = sdkRoot;
  path.AppendPath("Data/Samples/PacMan");
  return path;
}

ezResult ezEditorTestEditorProcessor::RunEditorProcessor(const ezDynamicArray<ezString>& arguments)
{
  ezProcessOptions processOptions;
  processOptions.m_sProcess = GetEditorProcessorPath();
  processOptions.m_Arguments = arguments;
  processOptions.m_bHideConsoleWindow = true;

  ezInt32 exitCode = 0;
  ezResult result = ezProcess::Execute(processOptions, &exitCode);

  if (result.Failed())
  {
    EZ_TEST_FAILURE("Failed to execute ezEditorProcessor", "Process execution failed");
    return EZ_FAILURE;
  }

  if (exitCode != 0)
  {
    EZ_TEST_FAILURE("ezEditorProcessor returned non-zero exit code", "Exit code: {0}", exitCode);
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezEditorTestEditorProcessor::PrepareCompile(ezStringBuilder& dllPath, ezStringBuilder& bundlePath)
{
  // Delete any existing build artifacts
  dllPath = ezOSFile::GetApplicationDirectory();
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  dllPath.AppendPath("PacManPlugin.dll");
#else
  dllPath.AppendPath("PacManPlugin.so");
#endif

  bundlePath = ezOSFile::GetApplicationDirectory();
  bundlePath.AppendPath("PacManPlugin.ezPluginBundle");

  ezOSFile::DeleteFile(dllPath).IgnoreResult();
  ezOSFile::DeleteFile(bundlePath).IgnoreResult();

  if (!EZ_TEST_BOOL(!ezOSFile::ExistsFile(dllPath)) ||
      !EZ_TEST_BOOL(!ezOSFile::ExistsFile(bundlePath)))
  {
    EZ_TEST_FAILURE("Failed to delete existing build artifacts", "DLL or bundle file still exists");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezTestAppRun ezEditorTestEditorProcessor::TestCompileOnly()
{
  const ezString projectPath = GetPacManProjectPath();
  if (projectPath.IsEmpty())
    return ezTestAppRun::Quit;

  ezStringBuilder dllPath, bundlePath;
  if (PrepareCompile(dllPath, bundlePath).Failed())
    return ezTestAppRun::Quit;


  // Run EditorProcessor with -compile flag
  ezDynamicArray<ezString> arguments;
  arguments.PushBack("-project");
  arguments.PushBack(projectPath);
  arguments.PushBack("-compile");

  if (RunEditorProcessor(arguments).Failed())
    return ezTestAppRun::Quit;

  // Verify that the DLL and bundle files have been created
  EZ_TEST_BOOL(ezOSFile::ExistsFile(dllPath));
  EZ_TEST_BOOL(ezOSFile::ExistsFile(bundlePath));

  return ezTestAppRun::Quit;
}

ezTestAppRun ezEditorTestEditorProcessor::TestCompileAndTransform()
{
  const ezString projectPath = GetPacManProjectPath();
  if (projectPath.IsEmpty())
    return ezTestAppRun::Quit;

  // Delete AssetCache folder before the test
  ezStringBuilder assetCachePath = projectPath;
  assetCachePath.AppendPath("AssetCache");

  if (ezOSFile::ExistsDirectory(assetCachePath))
  {
    if (!EZ_TEST_BOOL_MSG(ezOSFile::DeleteFolder(assetCachePath).Succeeded(), "Failed to delete AssetCache folder"))
    {
      return ezTestAppRun::Quit;
    }
  }

  ezStringBuilder dllPath, bundlePath;
  if (PrepareCompile(dllPath, bundlePath).Failed())
    return ezTestAppRun::Quit;


  // Run EditorProcessor with -compile and -transform flags
  ezDynamicArray<ezString> arguments;
  arguments.PushBack("-project");
  arguments.PushBack(projectPath);
  arguments.PushBack("-compile");
  arguments.PushBack("-transform");
  arguments.PushBack("Default");

  if (RunEditorProcessor(arguments).Failed())
    return ezTestAppRun::Quit;

  // Verify that the AssetCache folder has been created
  EZ_TEST_BOOL(ezOSFile::ExistsDirectory(assetCachePath));

  // Verify that Default.ezAidlt exists in AssetCache
  ezStringBuilder aidltPath = assetCachePath;
  aidltPath.AppendPath("Default.ezAidlt");
  EZ_TEST_BOOL(ezOSFile::ExistsFile(aidltPath));

  // Verify that the DLL and bundle files have been created
  EZ_TEST_BOOL(ezOSFile::ExistsFile(dllPath));
  EZ_TEST_BOOL(ezOSFile::ExistsFile(bundlePath));

  return ezTestAppRun::Quit;
}
