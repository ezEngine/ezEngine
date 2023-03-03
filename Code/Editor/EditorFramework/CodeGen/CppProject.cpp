#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/Process.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Project/ToolsProject.h>

#include <Shlobj.h>

ezString ezCppProject::GetTargetSourceDir()
{
  ezStringBuilder sTargetDir = ezToolsProject::GetSingleton()->GetProjectDirectory();
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

bool ezCppProject::ExistsSolution(const ezCppSettings& cfg)
{
  return ezOSFile::ExistsFile(GetSolutionPath(cfg));
}

bool ezCppProject::ExistsProjectCMakeListsTxt()
{
  ezStringBuilder sPath = GetTargetSourceDir();
  sPath.AppendPath("CMakeLists.txt");
  return ezOSFile::ExistsFile(sPath);
}

ezResult ezCppProject::PopulateWithDefaultSources(const ezCppSettings& cfg)
{
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

  return EZ_SUCCESS;
}

ezResult ezCppProject::CleanBuildDir(const ezCppSettings& cfg)
{
  const ezString sBuildDir = GetBuildDir(cfg);

  if (!ezOSFile::ExistsDirectory(sBuildDir))
    return EZ_SUCCESS;

  return ezOSFile::DeleteFolder(sBuildDir);
}

ezResult ezCppProject::RunCMake(const ezCppSettings& cfg)
{
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

  ezLog::Success("Solution generated.\n\n{}\n", log.m_sBuffer);
  return EZ_SUCCESS;
}

ezResult ezCppProject::RunCMakeIfNecessary(const ezCppSettings& cfg)
{
  if (!ezCppProject::ExistsProjectCMakeListsTxt())
    return EZ_SUCCESS;

  if (ezCppProject::ExistsSolution(cfg))
    return EZ_SUCCESS;

  return ezCppProject::RunCMake(cfg);
}

ezResult ezCppProject::CompileSolution(const ezCppSettings& cfg)
{
  EZ_LOG_BLOCK("Compile Solution");

  if (cfg.m_sMsBuildPath.IsEmpty())
  {
    ezLog::Error("MSBuild path is not available.");
    return EZ_FAILURE;
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

  if (!ezCppProject::ExistsSolution(cfg))
  {
    EZ_SUCCEED_OR_RETURN(ezCppProject::RunCMake(cfg));
  }

  return CompileSolution(cfg);
}

ezResult ezCppProject::FindMsBuild(const ezCppSettings& cfg)
{
  if (!cfg.m_sMsBuildPath.IsEmpty())
    return EZ_SUCCESS;

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
}
