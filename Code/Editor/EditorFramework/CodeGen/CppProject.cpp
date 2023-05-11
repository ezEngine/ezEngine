#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/Process.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Project/ToolsProject.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#  include <Shlobj.h>
#endif

ezEvent<const ezCppSettings&> ezCppProject::s_ChangeEvents;

ezString ezCppProject::GetTargetSourceDir(ezStringView sProjectDirectory /*= {}*/)
{
  ezStringBuilder sTargetDir = sProjectDirectory;

  if (sTargetDir.IsEmpty())
  {
    sTargetDir = ezToolsProject::GetSingleton()->GetProjectDirectory();
  }

  sTargetDir.AppendPath("CppSource");
  return sTargetDir;
}

ezString ezCppProject::GetGeneratorFolderName(const ezCppSettings& cfg)
{
  switch (cfg.m_Compiler)
  {
    case ezCppSettings::Compiler::None:
      return "";

    case ezCppSettings::Compiler::Vs2019:
      return "Vs2019x64";

    case ezCppSettings::Compiler::Vs2022:
      return "Vs2022x64";

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return {};
}

ezString ezCppProject::GetCMakeGeneratorName(const ezCppSettings& cfg)
{
  switch (cfg.m_Compiler)
  {
    case ezCppSettings::Compiler::None:
      return "";

    case ezCppSettings::Compiler::Vs2019:
      return "Visual Studio 16 2019";

    case ezCppSettings::Compiler::Vs2022:
      return "Visual Studio 17 2022";

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return {};
}

ezString ezCppProject::GetPluginSourceDir(const ezCppSettings& cfg, ezStringView sProjectDirectory /*= {}*/)
{
  ezStringBuilder sDir = GetTargetSourceDir(sProjectDirectory);
  sDir.AppendPath(cfg.m_sPluginName);
  sDir.Append("Plugin");
  return sDir;
}

ezString ezCppProject::GetBuildDir(const ezCppSettings& cfg)
{
  ezStringBuilder sBuildDir;
  sBuildDir.Format("{}/Build/{}", GetTargetSourceDir(), GetGeneratorFolderName(cfg));
  return sBuildDir;
}

ezString ezCppProject::GetSolutionPath(const ezCppSettings& cfg)
{
  ezStringBuilder sSolutionFile;
  sSolutionFile = GetBuildDir(cfg);
  sSolutionFile.AppendPath(cfg.m_sPluginName);
  sSolutionFile.Append(".sln");
  return sSolutionFile;
}

ezResult ezCppProject::CheckCMakeCache(const ezCppSettings& cfg)
{
  ezStringBuilder sCacheFile;
  sCacheFile = GetBuildDir(cfg);
  sCacheFile.AppendPath("CMakeCache.txt");

  ezFileReader file;
  EZ_SUCCEED_OR_RETURN(file.Open(sCacheFile));

  ezStringBuilder content;
  content.ReadAll(file);

  const ezStringView sSearchFor = "CMAKE_CONFIGURATION_TYPES:STRING="_ezsv;

  const char* pConfig = content.FindSubString(sSearchFor);
  if (pConfig == nullptr)
    return EZ_FAILURE;

  pConfig += sSearchFor.GetElementCount();

  const char* pEndConfig = content.FindSubString("\n", pConfig);
  if (pEndConfig == nullptr)
    return EZ_FAILURE;

  ezStringBuilder sUsedCfg;
  sUsedCfg.SetSubString_FromTo(pConfig, pEndConfig);
  sUsedCfg.Trim("\t\n\r ");

  if (sUsedCfg != BUILDSYSTEM_BUILDTYPE)
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

bool ezCppProject::ExistsSolution(const ezCppSettings& cfg)
{
  return ezOSFile::ExistsFile(GetSolutionPath(cfg));
}

bool ezCppProject::ExistsProjectCMakeListsTxt()
{
  if (!ezToolsProject::IsProjectOpen())
    return false;

  ezStringBuilder sPath = GetTargetSourceDir();
  sPath.AppendPath("CMakeLists.txt");
  return ezOSFile::ExistsFile(sPath);
}

ezResult ezCppProject::PopulateWithDefaultSources(const ezCppSettings& cfg)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  EZ_SCOPE_EXIT(QApplication::restoreOverrideCursor());

  const ezString sProjectName = cfg.m_sPluginName;

  ezStringBuilder sProjectNameUpper = cfg.m_sPluginName;
  sProjectNameUpper.ToUpper();

  const ezStringBuilder sTargetDir = ezToolsProject::GetSingleton()->GetProjectDirectory();

  ezStringBuilder sSourceDir = ezApplicationServices::GetSingleton()->GetApplicationDataFolder();
  sSourceDir.AppendPath("CppProject");

  ezDynamicArray<ezFileStats> items;
  ezOSFile::GatherAllItemsInFolder(items, sSourceDir, ezFileSystemIteratorFlags::ReportFilesRecursive);

  struct FileToCopy
  {
    ezString m_sSource;
    ezString m_sDestination;
  };

  ezHybridArray<FileToCopy, 32> filesCopied;

  // gather files
  {
    for (const auto& item : items)
    {
      ezStringBuilder srcPath, dstPath;
      item.GetFullPath(srcPath);

      dstPath = srcPath;
      dstPath.MakeRelativeTo(sSourceDir).IgnoreResult();

      dstPath.ReplaceAll("CppProject", sProjectName);
      dstPath.Prepend(sTargetDir, "/");
      dstPath.MakeCleanPath();

      // don't copy files over that already exist (and may have edits)
      if (ezOSFile::ExistsFile(dstPath))
      {
        // if any file already exists, don't copy non-existing (user might have deleted unwanted sample files)
        filesCopied.Clear();
        break;
      }

      auto& ftc = filesCopied.ExpandAndGetRef();
      ftc.m_sSource = srcPath;
      ftc.m_sDestination = dstPath;
    }
  }

  // Copy files
  {
    for (const auto& ftc : filesCopied)
    {
      if (ezOSFile::CopyFile(ftc.m_sSource, ftc.m_sDestination).Failed())
      {
        ezLog::Error("Failed to copy a file.\nSource: '{}'\nDestination: '{}'\n", ftc.m_sSource, ftc.m_sDestination);
        return EZ_FAILURE;
      }
    }
  }

  // Modify sources
  {
    for (const auto& filePath : filesCopied)
    {
      ezStringBuilder content;

      {
        ezFileReader file;
        if (file.Open(filePath.m_sDestination).Failed())
        {
          ezLog::Error("Failed to open C++ project file for reading.\nSource: '{}'\n", filePath.m_sDestination);
          return EZ_FAILURE;
        }

        content.ReadAll(file);
      }

      content.ReplaceAll("CppProject", sProjectName);
      content.ReplaceAll("CPPPROJECT", sProjectNameUpper);

      {
        ezFileWriter file;
        if (file.Open(filePath.m_sDestination).Failed())
        {
          ezLog::Error("Failed to open C++ project file for writing.\nSource: '{}'\n", filePath.m_sDestination);
          return EZ_FAILURE;
        }

        file.WriteBytes(content.GetData(), content.GetElementCount()).IgnoreResult();
      }
    }
  }

  s_ChangeEvents.Broadcast(cfg);
  return EZ_SUCCESS;
}

ezResult ezCppProject::CleanBuildDir(const ezCppSettings& cfg)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  EZ_SCOPE_EXIT(QApplication::restoreOverrideCursor());

  const ezString sBuildDir = GetBuildDir(cfg);

  if (!ezOSFile::ExistsDirectory(sBuildDir))
    return EZ_SUCCESS;

  return ezOSFile::DeleteFolder(sBuildDir);
}

ezResult ezCppProject::RunCMake(const ezCppSettings& cfg)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  EZ_SCOPE_EXIT(QApplication::restoreOverrideCursor());

  if (!ExistsProjectCMakeListsTxt())
  {
    ezLog::Error("No CMakeLists.txt exists in target source directory '{}'", GetTargetSourceDir());
    return EZ_FAILURE;
  }

  const ezString sSdkDir = ezFileSystem::GetSdkRootDirectory();
  const ezString sBuildDir = ezCppProject::GetBuildDir(cfg);
  const ezString sSolutionFile = ezCppProject::GetSolutionPath(cfg);

  ezStringBuilder tmp;

  QStringList args;
  args << "-S";
  args << ezCppProject::GetTargetSourceDir().GetData();

  tmp.Format("-DEZ_SDK_DIR:PATH={}", sSdkDir);
  args << tmp.GetData();

  tmp.Format("-DEZ_BUILDTYPE_ONLY:STRING={}", BUILDSYSTEM_BUILDTYPE);
  args << tmp.GetData();

  args << "-G";
  args << ezCppProject::GetCMakeGeneratorName(cfg).GetData();

  args << "-B";
  args << sBuildDir.GetData();

  args << "-A";
  args << "x64";

  ezLogSystemToBuffer log;

  ezStatus res = ezQtEditorApp::GetSingleton()->ExecuteTool("cmake/bin/cmake", args, 120, &log, ezLogMsgType::InfoMsg);

  if (res.Failed())
  {
    ezLog::Error("Solution generation failed:\n\n{}\n{}\n", log.m_sBuffer, res.m_sMessage);
    return EZ_FAILURE;
  }

  if (!ExistsSolution(cfg))
  {
    ezLog::Error("CMake did not generate the expected solution. Did you attempt to rename it? If so, you may need to delete the top-level CMakeLists.txt file and set up the C++ project again.");
    return EZ_FAILURE;
  }

  ezLog::Success("Solution generated.\n\n{}\n", log.m_sBuffer);
  s_ChangeEvents.Broadcast(cfg);
  return EZ_SUCCESS;
}

ezResult ezCppProject::RunCMakeIfNecessary(const ezCppSettings& cfg)
{
  if (!ezCppProject::ExistsProjectCMakeListsTxt())
    return EZ_SUCCESS;

  if (ezCppProject::ExistsSolution(cfg) && ezCppProject::CheckCMakeCache(cfg))
    return EZ_SUCCESS;

  return ezCppProject::RunCMake(cfg);
}

ezResult ezCppProject::CompileSolution(const ezCppSettings& cfg)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  EZ_SCOPE_EXIT(QApplication::restoreOverrideCursor());

  EZ_LOG_BLOCK("Compile Solution");

  if (cfg.m_sMsBuildPath.IsEmpty())
  {
    ezLog::Error("MSBuild path is not available.");
    return EZ_FAILURE;
  }

  if (ezSystemInformation::IsDebuggerAttached())
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning("When a debugger is attached, MSBuild usually fails to compile the project.\n\nDetach the debugger now, then press OK to continue.");
  }

  ezHybridArray<ezString, 32> errors;

  ezProcessOptions po;
  po.m_sProcess = cfg.m_sMsBuildPath;
  po.m_bHideConsoleWindow = true;
  po.m_onStdOut = [&](ezStringView res)
  {
    if (res.FindSubString_NoCase("error") != nullptr)
      errors.PushBack(res);
  };

  po.AddArgument(ezCppProject::GetSolutionPath(cfg));
  po.AddArgument("/m"); // multi-threaded compilation
  po.AddArgument("/nr:false");
  po.AddArgument("/t:Build");
  po.AddArgument("/p:Configuration={}", BUILDSYSTEM_BUILDTYPE);
  po.AddArgument("/p:Platform=x64");

  ezStringBuilder sMsBuildCmd;
  po.BuildCommandLineString(sMsBuildCmd);
  ezLog::Dev("Running MSBuild: {}", sMsBuildCmd);

  ezInt32 iReturnCode = 0;
  if (ezProcess::Execute(po, &iReturnCode).Failed())
  {
    ezLog::Error("MSBuild failed to run.");
    return EZ_FAILURE;
  }

  if (iReturnCode == 0)
  {
    ezLog::Success("Compiled C++ solution.");
    return EZ_SUCCESS;
  }

  ezLog::Error("MSBuild failed with return code {}", iReturnCode);

  for (const auto& err : errors)
  {
    ezLog::Error(err);
  }

  return EZ_FAILURE;
}

ezResult ezCppProject::BuildCodeIfNecessary(const ezCppSettings& cfg)
{
  EZ_SUCCEED_OR_RETURN(ezCppProject::FindMsBuild(cfg));

  if (!ezCppProject::ExistsProjectCMakeListsTxt())
    return EZ_SUCCESS;

  if (!ezCppProject::ExistsSolution(cfg) || !ezCppProject::CheckCMakeCache(cfg))
  {
    EZ_SUCCEED_OR_RETURN(ezCppProject::RunCMake(cfg));
  }

  return CompileSolution(cfg);
}

ezResult ezCppProject::FindMsBuild(const ezCppSettings& cfg)
{
  if (!cfg.m_sMsBuildPath.IsEmpty())
    return EZ_SUCCESS;

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  ezStringBuilder sVsWhere;

  wchar_t* pPath = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramFilesX86, KF_FLAG_DEFAULT, nullptr, &pPath)))
  {
    sVsWhere = ezStringWChar(pPath);
    sVsWhere.AppendPath("Microsoft Visual Studio/Installer/vswhere.exe");

    CoTaskMemFree(pPath);
  }
  else
  {
    ezLog::Error("Could not find the 'Program Files (x86)' folder.");
    return EZ_FAILURE;
  }

  ezStringBuilder sStdOut;
  ezProcessOptions po;
  po.m_sProcess = sVsWhere;
  po.m_bHideConsoleWindow = true;
  po.m_onStdOut = [&](ezStringView res)
  { sStdOut.Append(res); };

  // TODO: search for VS2022 or VS2019 depending on cfg
  po.AddCommandLine("-latest -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe");

  if (ezProcess::Execute(po).Failed())
  {
    ezLog::Error("Executing vsWhere.exe failed. Do you have the correct version of Visual Studio installed?");
    return EZ_FAILURE;
  }

  sStdOut.Trim("\n\r");
  sStdOut.MakeCleanPath();

  cfg.m_sMsBuildPath = sStdOut;
  return EZ_SUCCESS;
#else
  return EZ_FAILURE;
#endif
}

void ezCppProject::UpdatePluginConfig(const ezCppSettings& cfg)
{
  const ezStringBuilder sPluginName(cfg.m_sPluginName, "Plugin");

  ezPluginBundleSet& bundles = ezQtEditorApp::GetSingleton()->GetPluginBundles();

  ezStringBuilder txt;
  bundles.m_Plugins.Remove(sPluginName);
  ezPluginBundle& plugin = bundles.m_Plugins[sPluginName];
  plugin.m_bLoadCopy = true;
  plugin.m_bSelected = true;
  plugin.m_bMissing = true;
  plugin.m_LastModificationTime.Invalidate();
  plugin.m_ExclusiveFeatures.PushBack("ProjectPlugin");
  txt.Set("'", cfg.m_sPluginName, "' project plugin");
  plugin.m_sDisplayName = txt;
  txt.Set("C++ code for the '", cfg.m_sPluginName, "' project.");
  plugin.m_sDescription = txt;
  plugin.m_RuntimePlugins.PushBack(sPluginName);

  ezQtEditorApp::GetSingleton()->WritePluginSelectionStateDDL();
}

ezResult ezCppProject::EnsureCppPluginReady()
{
  if (!ExistsProjectCMakeListsTxt())
    return EZ_SUCCESS;

  ezCppSettings cppSettings;
  if (cppSettings.Load().Failed())
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to load the C++ plugin settings."));
    return EZ_FAILURE;
  }

  if (ezCppProject::BuildCodeIfNecessary(cppSettings).Failed())
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to build the C++ code. See log for details."));
    return EZ_FAILURE;
  }

  ezQtEditorApp::GetSingleton()->RestartEngineProcessIfPluginsChanged(true);
  return EZ_SUCCESS;
}

bool ezCppProject::IsBuildRequired()
{
  if (!ExistsProjectCMakeListsTxt())
    return false;

  ezCppSettings cfg;
  if (cfg.Load().Failed())
    return false;

  if (!ezCppProject::ExistsSolution(cfg))
    return true;

  if (!ezCppProject::CheckCMakeCache(cfg))
    return true;

  ezStringBuilder sPath = ezOSFile::GetApplicationDirectory();
  sPath.AppendPath(cfg.m_sPluginName);
  sPath.Append("Plugin");

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  sPath.Append(".dll");
#else
  sPath.Append(".so");
#endif

  if (!ezOSFile::ExistsFile(sPath))
    return true;

  return false;
}
