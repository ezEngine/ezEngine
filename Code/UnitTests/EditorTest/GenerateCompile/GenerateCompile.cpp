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
  AddSubTest("01 - Generate and Compile", SubTests::ST_GenerateAndCompile);
  AddSubTest("02 - EditorProcessor: Compile Only", SubTests::ST_EditorProcessorCompileOnly);
  AddSubTest("03 - EditorProcessor: Compile and Transform", SubTests::ST_EditorProcessorCompileAndTransform);
}

ezResult ezEditorTestGenerateCompile::InitializeTest()
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
  ezPreferences::SaveApplicationPreferences();

  return EZ_SUCCESS;
}

ezResult ezEditorTestGenerateCompile::DeInitializeTest()
{
  if (!m_sProjectPath.IsEmpty())
  {
    // The build results take up vast amounts of memory and prolong test result upload.
    ezStringBuilder buildOutput = m_sProjectPath;
    buildOutput.AppendPath("CppSource", "Build");
    ezOSFile::DeleteFolder(buildOutput).IgnoreResult();
  }

  if (SUPER::DeInitializeTest().Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezStatus ezEditorTestGenerateCompile::PrepareCompile(ezStringBuilder& dllPath, ezStringBuilder& bundlePath)
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
    return ezStatus("Failed to delete existing build artifacts - DLL or bundle file still exists");
  }

  return ezStatus(EZ_SUCCESS);
}

ezString ezEditorTestGenerateCompile::GetEditorProcessorPath() const
{
  ezStringBuilder path;
  // Get the directory where EditorTest.exe is located
  path = ezOSFile::GetApplicationDirectory();
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  path.AppendPath("ezEditorProcessor.exe");
#else
  path.AppendPath("ezEditorProcessor");
#endif
  return path;
}

ezStatus ezEditorTestGenerateCompile::RunEditorProcessor(const ezDynamicArray<ezString>& arguments)
{
  ezProcessOptions processOptions;
  processOptions.m_sProcess = GetEditorProcessorPath();
  processOptions.m_Arguments = arguments;
  processOptions.m_bHideConsoleWindow = true;

  ezInt32 exitCode = 0;
  ezResult result = ezProcess::Execute(processOptions, &exitCode);

  if (result.Failed() || exitCode != 0)
  {
    return ezStatus(ezFmt("ezEditorProcessor failed with exit code: {}", exitCode));
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezEditorTestGenerateCompile::EditorProcessorCompileOnly()
{
  ezStringBuilder dllPath, bundlePath;
  EZ_SUCCEED_OR_RETURN(PrepareCompile(dllPath, bundlePath));

  // Run EditorProcessor with -compile flag
  ezDynamicArray<ezString> arguments;
  arguments.PushBack("-project");
  arguments.PushBack(m_sProjectPath);
  arguments.PushBack("-compile");
  arguments.PushBack("-outputDir");
  ezStringBuilder userDataDir = ezTestFramework::GetInstance()->GetAbsOutputPath();
  userDataDir.AppendPath(GetTestName());
  userDataDir.MakeCleanPath();
  arguments.PushBack(userDataDir);

  EZ_SUCCEED_OR_RETURN(RunEditorProcessor(arguments));

  // Verify that the DLL and bundle files have been created
  EZ_TEST_BOOL(ezOSFile::ExistsFile(dllPath));
  EZ_TEST_BOOL(ezOSFile::ExistsFile(bundlePath));

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezEditorTestGenerateCompile::EditorProcessorCompileAndTransform()
{
  // Delete AssetCache folder before the test
  ezStringBuilder assetCachePath = m_sProjectPath;
  assetCachePath.AppendPath("AssetCache");

  if (ezOSFile::ExistsDirectory(assetCachePath) && ezOSFile::DeleteFolder(assetCachePath).Failed())
  {
    return ezStatus(ezFmt("Failed to delete asset cache folder: {}", assetCachePath));
  }

  ezStringBuilder dllPath, bundlePath;
  EZ_SUCCEED_OR_RETURN(PrepareCompile(dllPath, bundlePath));

  // Run EditorProcessor with -compile and -transform flags
  ezDynamicArray<ezString> arguments;
  arguments.PushBack("-project");
  arguments.PushBack(m_sProjectPath);
  arguments.PushBack("-compile");
  arguments.PushBack("-transform");
  arguments.PushBack("Default");
  arguments.PushBack("-outputDir");
  ezStringBuilder userDataDir = ezTestFramework::GetInstance()->GetAbsOutputPath();
  userDataDir.AppendPath(GetTestName());
  userDataDir.MakeCleanPath();
  arguments.PushBack(userDataDir);

  EZ_SUCCEED_OR_RETURN(RunEditorProcessor(arguments));

  // Verify that the AssetCache folder has been created
  EZ_TEST_BOOL(ezOSFile::ExistsDirectory(assetCachePath));

  // Verify that Default.ezAidlt exists in AssetCache
  ezStringBuilder aidltPath = assetCachePath;
  aidltPath.AppendPath("Default.ezAidlt");
  EZ_TEST_BOOL(ezOSFile::ExistsFile(aidltPath));

  // Verify that the DLL and bundle files have been created
  EZ_TEST_BOOL(ezOSFile::ExistsFile(dllPath));
  EZ_TEST_BOOL(ezOSFile::ExistsFile(bundlePath));

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezEditorTestGenerateCompile::GenerateAndCompile()
{
  ezStringBuilder dllPath, bundlePath;
  EZ_SUCCEED_OR_RETURN(PrepareCompile(dllPath, bundlePath));

  ezCppSettings cpp;
  if (!EZ_TEST_RESULT(cpp.Load()))
    return ezStatus(EZ_FAILURE);

  ezString sBuildDir = ezCppProject::GetBuildDir(cpp);
  if (ezOSFile::ExistsDirectory(sBuildDir))
  {
    if (!EZ_TEST_RESULT(ezOSFile::DeleteFolder(sBuildDir)))
      return ezStatus(EZ_FAILURE);
  }

  if (!EZ_TEST_RESULT(ezCppProject::RunCMake(cpp)))
    return ezStatus(EZ_FAILURE);

  EZ_TEST_BOOL(ezCppProject::ExistsProjectCMakeListsTxt());
  EZ_TEST_BOOL(ezCppProject::ExistsSolution(cpp));

  EZ_TEST_RESULT(ezCppProject::BuildCodeIfNecessary(cpp));
  ProcessEvents();
  return ezStatus(EZ_SUCCESS);
}

ezTestAppRun ezEditorTestGenerateCompile::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  ProcessEvents();
  switch (iIdentifier)
  {
    case SubTests::ST_GenerateAndCompile:
    {
      EZ_TEST_STATUS(GenerateAndCompile());
    }
    break;
    case SubTests::ST_EditorProcessorCompileOnly:
    {
      EZ_TEST_STATUS(EditorProcessorCompileOnly());
    }
    break;
    case SubTests::ST_EditorProcessorCompileAndTransform:
    {
      EZ_TEST_STATUS(EditorProcessorCompileAndTransform());
    }
    break;
  }
  ProcessEvents();
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
