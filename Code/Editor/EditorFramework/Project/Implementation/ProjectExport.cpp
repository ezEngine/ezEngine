#include <Core/Configuration/PlatformProfile.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Project/ProjectExport.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <ToolsFoundation/Utilities/PathPatternFilter.h>

ezResult ezProjectExport::ClearTargetFolder(const char* szAbsFolderPath)
{
  if (ezOSFile::DeleteFolder(szAbsFolderPath).Failed())
  {
    ezLog::Error("Target folder could not be removed:\n'{}'", szAbsFolderPath);
    return EZ_FAILURE;
  }

  if (ezOSFile::CreateDirectoryStructure(szAbsFolderPath).Failed())
  {
    ezLog::Error("Target folder could not be created:\n'{}'", szAbsFolderPath);
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezProjectExport::ScanFolder(ezSet<ezString>& out_Files, const char* szFolder, const ezPathPatternFilter& filter, ezAssetCurator* pCurator, ezDynamicArray<ezString>* pSceneFiles, const ezPlatformProfile* pPlatformProfile)
{
  ezStringBuilder sRootFolder = szFolder;
  sRootFolder.Trim("/\\");

  const ezUInt32 uiRootFolderLength = sRootFolder.GetElementCount();

  ezStringBuilder sAbsFilePath, sRelFilePath;

  ezFileSystemIterator it;
  for (it.StartSearch(sRootFolder, ezFileSystemIteratorFlags::ReportFilesAndFoldersRecursive); it.IsValid();)
  {
    if (ezProgress::GetGlobalProgressbar()->WasCanceled())
    {
      ezLog::Warning("Folder scanning canceled by user");
      return EZ_FAILURE;
    }

    it.GetStats().GetFullPath(sAbsFilePath);

    sRelFilePath = sAbsFilePath;
    sRelFilePath.Shrink(uiRootFolderLength, 0); // keep the slash at the front -> useful for the pattern filter

    ezStringBuilder filterRule;

    if (!filter.PassesFilters(sRelFilePath, &filterRule))
    {
      if (it.GetStats().m_bIsDirectory)
      {
        ezLog::Info(" Skipping folder '{}' - doesn't pass filter rule '{}'.", sRelFilePath, filterRule);
        it.SkipFolder();
      }
      else
      {
        ezLog::Info(" Skipping file '{}' - doesn't pass filter rule '{}'.", sRelFilePath, filterRule);
        it.Next();
      }

      continue;
    }

    if (it.GetStats().m_bIsDirectory)
    {
      it.Next();
      continue;
    }

    if (pCurator)
    {
      auto asset = pCurator->FindSubAsset(sAbsFilePath);

      if (asset.isValid() && asset->m_bMainAsset)
      {
        // redirect to asset output
        ezAssetDocumentManager* pAssetMan = ezStaticCast<ezAssetDocumentManager*>(asset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_pManager);

        sRelFilePath = pAssetMan->GetRelativeOutputFileName(asset->m_pAssetInfo->m_pDocumentTypeDescriptor, sRootFolder, asset->m_pAssetInfo->m_Path, nullptr, pPlatformProfile);

        sRelFilePath.Prepend("AssetCache/");
        out_Files.Insert(sRelFilePath);

        if (pSceneFiles && asset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sDocumentTypeName == "Scene")
        {
          pSceneFiles->PushBack(sRelFilePath);
        }

        for (const ezString& outputTag : asset->m_pAssetInfo->m_Info->m_Outputs)
        {
          sRelFilePath = pAssetMan->GetRelativeOutputFileName(asset->m_pAssetInfo->m_pDocumentTypeDescriptor, sRootFolder, asset->m_pAssetInfo->m_Path, outputTag, pPlatformProfile);

          sRelFilePath.Prepend("AssetCache/");
          out_Files.Insert(sRelFilePath);
        }

        it.Next();
        continue;
      }
    }

    out_Files.Insert(sRelFilePath);
    it.Next();
  }

  return EZ_SUCCESS;
}

ezResult ezProjectExport::CopyFiles(const char* szSrcFolder, const char* szDstFolder, const ezSet<ezString>& files, ezProgressRange* pProgressRange)
{
  ezLog::Info("Source folder: {}", szSrcFolder);
  ezLog::Info("Destination folder: {}", szDstFolder);

  ezStringBuilder sSrc, sDst;

  for (auto itFile = files.GetIterator(); itFile.IsValid(); ++itFile)
  {
    if (ezProgress::GetGlobalProgressbar()->WasCanceled())
    {
      ezLog::Info("File copy operation canceled by user.");
      return EZ_FAILURE;
    }

    if (pProgressRange)
    {
      pProgressRange->BeginNextStep(itFile.Key());
    }

    sSrc.Set(szSrcFolder, "/", itFile.Key());
    sDst.Set(szDstFolder, "/", itFile.Key());

    if (ezOSFile::CopyFile(sSrc, sDst).Succeeded())
    {
      ezLog::Info(" Copied: {}", itFile.Key());
    }
    else
    {
      ezLog::Error(" Copy failed: {}", itFile.Key());
    }
  }

  ezLog::Success("Finished copying files to destination '{}'", szDstFolder);
  return EZ_SUCCESS;
}

ezResult ezProjectExport::GatherGeneratedAssetManagerFiles(ezSet<ezString>& out_Files)
{
  ezTempHybridArray<ezString, 4> addFiles;

  for (auto pMan : ezDocumentManager::GetAllDocumentManagers())
  {
    if (auto pAssMan = ezDynamicCast<ezAssetDocumentManager*>(pMan))
    {
      pAssMan->GetAdditionalOutputs(addFiles).AssertSuccess();

      for (const auto& file : addFiles)
      {
        out_Files.Insert(file);
      }

      addFiles.Clear();
    }
  }

  return EZ_SUCCESS;
}

ezResult ezProjectExport::CreateExportFilterFile(const char* szExpectedFile, const char* szFallbackFile)
{
  if (ezFileSystem::ExistsFile(szExpectedFile))
    return EZ_SUCCESS;

  ezStringBuilder src;
  src.Set("#include <", szFallbackFile, ">\n\n\n[EXCLUDE]\n\n// TODO: add exclude patterns\n\n\n[INCLUDE]\n\n//TODO: add include patterns\n\n\n");

  ezFileWriter file;
  if (file.Open(szExpectedFile).Failed())
  {
    ezLog::Error("Failed to open '{}' for writing.", szExpectedFile);
    return EZ_FAILURE;
  }

  file.WriteBytes(src.GetData(), src.GetElementCount()).AssertSuccess();
  return EZ_SUCCESS;
}

ezResult ezProjectExport::ReadExportFilters(ezPathPatternFilter& out_DataFilter, ezPathPatternFilter& out_BinariesFilter, const ezPlatformProfile* pPlatformProfile)
{
  ezStringBuilder sDefine;
  sDefine.SetFormat("PLATFORM_PROFILE_{} 1", pPlatformProfile->GetConfigName());
  sDefine.ToUpper();

  ezTempHybridArray<ezString, 1> ppDefines;
  ppDefines.PushBack(sDefine);

  if (ezProjectExport::CreateExportFilterFile(":project/ProjectData.ezExportFilter", "CommonData.ezExportFilter").Failed())
  {
    ezLog::Error("The file 'ProjectData.ezExportFilter' could not be created.");
    return EZ_FAILURE;
  }

  if (ezProjectExport::CreateExportFilterFile(":project/ProjectBinaries.ezExportFilter", "CommonBinaries.ezExportFilter").Failed())
  {
    ezLog::Error("The file 'ProjectBinaries.ezExportFilter' could not be created.");
    return EZ_FAILURE;
  }

  if (out_DataFilter.ReadConfigFile("ProjectData.ezExportFilter", ppDefines).Failed())
  {
    ezLog::Error("The file 'ProjectData.ezExportFilter' could not be read.");
    return EZ_FAILURE;
  }

  if (out_BinariesFilter.ReadConfigFile("ProjectBinaries.ezExportFilter", ppDefines).Failed())
  {
    ezLog::Error("The file 'ProjectBinaries.ezExportFilter' could not be read.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezProjectExport::CreateDataDirectoryDDL(const DirectoryMapping& mapping, const char* szTargetDirectory)
{
  ezApplicationFileSystemConfig cfg;

  ezStringBuilder sPath;

  for (auto itDir = mapping.GetIterator(); itDir.IsValid(); ++itDir)
  {
    const auto& info = itDir.Value();

    if (info.m_sTargetDirRootName == "-")
      continue;

    sPath.Set(">sdk/", info.m_sTargetDirPath);

    auto& ddc = cfg.m_DataDirs.ExpandAndGetRef();
    ddc.m_sDataDirSpecialPath = sPath;
    ddc.m_sRootName = info.m_sTargetDirRootName;
  }

  sPath.Set(szTargetDirectory, "/Data/project/RuntimeConfigs/DataDirectories.ddl");

  if (cfg.Save(sPath).Failed())
  {
    ezLog::Error("Failed to write DataDirectories.ddl file.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezProjectExport::GatherAssetLookupTableFiles(DirectoryMapping& mapping, const ezApplicationFileSystemConfig& dirConfig, const ezPlatformProfile* pPlatformProfile)
{
  ezStringBuilder sDataDirPath;

  for (const auto& dataDir : dirConfig.m_DataDirs)
  {
    if (ezFileSystem::ResolveSpecialDirectory(dataDir.m_sDataDirSpecialPath, sDataDirPath).Failed())
    {
      ezLog::Error("Failed to resolve data directory path '{}'", dataDir.m_sDataDirSpecialPath);
      return EZ_FAILURE;
    }

    sDataDirPath.Trim("/\\");

    ezStringBuilder sAidltPath("AssetCache/", pPlatformProfile->GetConfigName(), ".ezAidlt");

    mapping[sDataDirPath].m_Files.Insert(sAidltPath);
  }

  return EZ_SUCCESS;
}

ezResult ezProjectExport::ScanDataDirectories(DirectoryMapping& mapping, const ezApplicationFileSystemConfig& dirConfig, const ezPathPatternFilter& dataFilter, ezDynamicArray<ezString>* pSceneFiles, const ezPlatformProfile* pPlatformProfile)
{
  ezProgressRange progress("Scanning data directories", dirConfig.m_DataDirs.GetCount(), true);

  ezUInt32 uiDataDirNumber = 1;

  ezStringBuilder sDataDirPath, sDstPath;

  for (const auto& dataDir : dirConfig.m_DataDirs)
  {
    progress.BeginNextStep(dataDir.m_sDataDirSpecialPath);

    if (ezFileSystem::ResolveSpecialDirectory(dataDir.m_sDataDirSpecialPath, sDataDirPath).Failed())
    {
      ezLog::Error("Failed to get special directory '{0}'", dataDir.m_sDataDirSpecialPath);
      return EZ_FAILURE;
    }

    sDataDirPath.Trim("/\\");

    ezProjectExport::DataDirectory& ddInfo = mapping[sDataDirPath];

    if (!dataDir.m_sRootName.IsEmpty())
    {
      sDstPath.Set("Data/", dataDir.m_sRootName);

      ddInfo.m_sTargetDirRootName = dataDir.m_sRootName;
      ddInfo.m_sTargetDirPath = sDstPath;
    }
    else
    {
      sDstPath.SetFormat("Data/Extra{}", uiDataDirNumber);
      ++uiDataDirNumber;

      ddInfo.m_sTargetDirPath = sDstPath;
    }

    EZ_SUCCEED_OR_RETURN(ezProjectExport::ScanFolder(ddInfo.m_Files, sDataDirPath, dataFilter, ezAssetCurator::GetSingleton(), pSceneFiles, pPlatformProfile));
  }

  return EZ_SUCCESS;
}

ezResult ezProjectExport::CopyAllFiles(DirectoryMapping& mapping, const char* szTargetDirectory)
{
  ezUInt32 uiTotalFiles = 0;
  for (auto itDir = mapping.GetIterator(); itDir.IsValid(); ++itDir)
    uiTotalFiles += itDir.Value().m_Files.GetCount();

  ezProgressRange range("Copying files", uiTotalFiles, true);

  ezLog::Info("Copying files to target directory '{}'", szTargetDirectory);

  ezStringBuilder sTargetFolder;

  for (auto itDir = mapping.GetIterator(); itDir.IsValid(); ++itDir)
  {
    sTargetFolder.Set(szTargetDirectory, "/", itDir.Value().m_sTargetDirPath);

    if (ezProjectExport::CopyFiles(itDir.Key(), sTargetFolder, itDir.Value().m_Files, &range).Failed())
      return EZ_FAILURE;
  }

  ezLog::Success("Finished copying all files.");
  return EZ_SUCCESS;
}

ezResult ezProjectExport::GatherBinaries(DirectoryMapping& mapping, const ezPathPatternFilter& filter)
{
  ezStringBuilder sAppDir;
  sAppDir = ezOSFile::GetApplicationDirectory();
  sAppDir.MakeCleanPath();
  sAppDir.Trim("/\\");

  ezProjectExport::DataDirectory& ddInfo = mapping[sAppDir];
  ddInfo.m_sTargetDirPath = "Bin";
  ddInfo.m_sTargetDirRootName = "-"; // don't add to data dir config

  if (ezProjectExport::ScanFolder(ddInfo.m_Files, sAppDir, filter, nullptr, nullptr, nullptr).Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezString ezProjectExport::FindCustomGameExecutable()
{
  if (!ezFileSystem::ExistsFile(":project/Editor/CppProject.ddl"))
    return {};

  ezCppSettings cppSettings;
  if (cppSettings.Load().Failed() || cppSettings.m_sPluginName.IsEmpty())
    return {};

  // this has to match the name of the application target that the C++ project generation creates
  ezStringBuilder sExeName(cppSettings.m_sPluginName, "Game");

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  sExeName.Append(".exe");
#endif

  ezStringBuilder sExePath = ezOSFile::GetApplicationDirectory();
  sExePath.AppendPath(sExeName);
  sExePath.MakeCleanPath();

  if (!ezOSFile::ExistsFile(sExePath))
  {
    ezLog::Info("The project's game executable '{}' hasn't been built, exporting ezPlayer instead.", sExeName);
    return {};
  }

  return sExeName;
}

ezResult ezProjectExport::CreateLaunchConfig(const ezDynamicArray<ezString>& sceneFiles, const char* szTargetDirectory, ezStringView sCustomGameExecutable)
{
  auto WriteScript = [szTargetDirectory](ezStringView sScriptName, ezStringView sCommand) -> ezResult
  {
    ezStringBuilder sScriptPath;
    sScriptPath.SetFormat("{}/{}.bat", szTargetDirectory, sScriptName);

    ezOSFile file;
    if (file.Open(sScriptPath, ezFileOpenMode::Write).Failed())
    {
      ezLog::Error("Couldn't create '{}'", sScriptPath);
      return EZ_FAILURE;
    }

    return file.Write(sCommand.GetStartPointer(), sCommand.GetElementCount());
  };

  if (!sCustomGameExecutable.IsEmpty())
  {
    // the game executable decides itself which scene to load (see ezGameState::GetStartupOptions()),
    // so a script per scene would be pointless here
    ezStringBuilder cmd;
    cmd.SetFormat("start Bin/{} -project \"Data/project\"", sCustomGameExecutable);

    ezStringBuilder sScriptName("Launch ", ezPathUtils::GetFileName(sCustomGameExecutable));
    return WriteScript(sScriptName, cmd);
  }

  for (const auto& sf : sceneFiles)
  {
    ezStringBuilder cmd;
    cmd.SetFormat("start Bin/ezPlayer.exe -project \"Data/project\" -scene \"{}\"", sf);

    ezStringBuilder sScriptName("Launch ", ezPathUtils::GetFileName(sf));
    EZ_SUCCEED_OR_RETURN(WriteScript(sScriptName, cmd));
  }

  return EZ_SUCCESS;
}

ezResult ezProjectExport::GatherGeneratedAssetFiles(ezSet<ezString>& out_Files, const char* szProjectDirectory)
{
  ezStringBuilder sRoot(szProjectDirectory, "/AssetCache/Generated");

  ezPathPatternFilter filter;
  ezSet<ezString> files;
  EZ_SUCCEED_OR_RETURN(ScanFolder(files, sRoot, filter, nullptr, nullptr, nullptr));

  ezStringBuilder sFilePath;

  for (const auto& file : files)
  {
    sFilePath.Set("/AssetCache/Generated", file);
    out_Files.Insert(sFilePath);
  }

  return EZ_SUCCESS;
}

void ezProjectExport::AddPackageDependenciesToFileList(DirectoryMapping& ref_fileList)
{
  auto knownAssets = ezAssetCurator::GetSingleton()->GetKnownAssets();

  ezStringBuilder sAbsPath, sRelPath;

  for (auto it = knownAssets->GetIterator(); it.IsValid(); ++it)
  {
    const ezAssetInfo* pAssetInfo = it.Value();
    if (pAssetInfo->m_Info == nullptr)
      continue;

    for (const ezString& dep : pAssetInfo->m_Info->m_PackageDependencies)
    {
      // GUIDs reference other assets which ScanFolder already handles via the asset curator
      if (ezConversionUtils::IsStringUuid(dep))
        continue;

      if (ezFileSystem::ResolvePath(dep, &sAbsPath, nullptr).Failed())
        continue;

      sAbsPath.MakeCleanPath();

      // Find which data directory this file belongs to and add it directly to that directory's file list
      for (auto itDir = ref_fileList.GetIterator(); itDir.IsValid(); ++itDir)
      {
        if (sAbsPath.StartsWith_NoCase(itDir.Key()))
        {
          sRelPath = sAbsPath;
          sRelPath.Shrink(itDir.Key().GetElementCount(), 0); // keeps the leading slash
          itDir.Value().m_Files.Insert(sRelPath);
          break;
        }
      }
    }
  }
}

ezStatus ezProjectExport::ExportProjectComplete(ezStringView sTargetDirectory, const ezProjectExportOptions& options, ezStringBuilder* out_pLog)
{
  if (sTargetDirectory.IsEmpty())
    return ezStatus("No target directory given.");

  if (!ezPathUtils::IsAbsolutePath(sTargetDirectory))
    return ezStatus(ezFmt("The target directory '{}' is not an absolute path.", sTargetDirectory));

  const ezString sTargetDir = sTargetDirectory;

  ezLogSystemToBuffer logBuffer;
  ezStatus result = ezStatus(EZ_SUCCESS);

  {
    ezLogSystemScope logScope(&logBuffer);

    if (options.m_bCompileCppPlugin)
    {
      // Does nothing if the project has no C++ code at all. Failing here would leave the export with
      // binaries that don't match the code, so it is not something to continue past.
      if (ezCppProject::EnsureCppPluginReady().Failed())
      {
        result = ezStatus("Building the project's C++ plugin failed.");
      }
    }

    if (result.Succeeded() && options.m_bTransformAssets)
    {
      const ezStatus stat = ezAssetCurator::GetSingleton()->TransformAllAssets();

      if (stat.Failed())
      {
        result = ezStatus(ezFmt("Transforming all assets failed: {}", stat.GetMessageString()));
      }
    }

    if (result.Succeeded())
    {
      if (ezProjectExport::ExportProject(sTargetDir, ezAssetCurator::GetSingleton()->GetActiveAssetProfile(), ezQtEditorApp::GetSingleton()->GetFileSystemConfig(), options.m_bCreateLaunchScripts).Failed())
      {
        result = ezStatus("Project export failed.");
      }
    }
  }

  if (out_pLog != nullptr)
  {
    *out_pLog = logBuffer.m_sBuffer;
  }

  // Written last, and only when the directory is there: a failure early on means ExportProject() never
  // got as far as creating it, and an otherwise empty folder containing just a log would look like a
  // half finished export.
  if (ezOSFile::ExistsDirectory(sTargetDir))
  {
    ezStringBuilder sLogFile(sTargetDir, "/ExportLog.txt");

    ezOSFile file;
    if (file.Open(sLogFile, ezFileOpenMode::Write).Succeeded())
    {
      file.Write(logBuffer.m_sBuffer.GetData(), logBuffer.m_sBuffer.GetElementCount()).IgnoreResult();
    }
    else
    {
      ezLog::Warning("Failed to write the export log '{}'.", sLogFile);
    }
  }

  return result;
}

ezResult ezProjectExport::ExportProject(const char* szTargetDirectory, const ezPlatformProfile* pPlatformProfile, const ezApplicationFileSystemConfig& dataDirs, bool bCreateLaunchScripts)
{
  ezProgressRange mainProgress("Export Project", 7, true);
  mainProgress.SetStepWeighting(0, 0.05f); // Preparing output folder
  mainProgress.SetStepWeighting(1, 0.05f); // Generating special files
  mainProgress.SetStepWeighting(2, 0.10f); // Scanning data directories
  mainProgress.SetStepWeighting(3, 0.05f); // Gathering binaries
  mainProgress.SetStepWeighting(4, 1.0f);  // Copying files
  mainProgress.SetStepWeighting(5, 0.01f); // Writing data directory config
  mainProgress.SetStepWeighting(6, 0.01f); // Finish up

  ezStringBuilder sProjectRootDir;
  ezTempHybridArray<ezString, 16> sceneFiles;
  ezProjectExport::DirectoryMapping fileList;

  ezPathPatternFilter dataFilter;
  ezPathPatternFilter binariesFilter;

  const ezString sCustomGameExecutable = ezProjectExport::FindCustomGameExecutable();

  // 0
  {
    mainProgress.BeginNextStep("Preparing output folder");
    EZ_SUCCEED_OR_RETURN(ezProjectExport::ClearTargetFolder(szTargetDirectory));
  }

  // 0
  {
    ezFileSystem::ResolveSpecialDirectory(">project", sProjectRootDir).AssertSuccess();
    sProjectRootDir.Trim("/\\");

    EZ_SUCCEED_OR_RETURN(ezProjectExport::GatherAssetLookupTableFiles(fileList, dataDirs, pPlatformProfile));
    EZ_SUCCEED_OR_RETURN(ezProjectExport::ReadExportFilters(dataFilter, binariesFilter, pPlatformProfile));
    EZ_SUCCEED_OR_RETURN(ezProjectExport::GatherGeneratedAssetFiles(fileList[sProjectRootDir].m_Files, sProjectRootDir));
  }

  // 1
  {
    mainProgress.BeginNextStep("Generating special files");
    EZ_SUCCEED_OR_RETURN(ezProjectExport::GatherGeneratedAssetManagerFiles(fileList[sProjectRootDir].m_Files));
  }

  // 2
  {
    mainProgress.BeginNextStep("Scanning data directories");
    EZ_SUCCEED_OR_RETURN(ezProjectExport::ScanDataDirectories(fileList, dataDirs, dataFilter, &sceneFiles, pPlatformProfile));
    ezProjectExport::AddPackageDependenciesToFileList(fileList);
  }

  // 3
  {
    // by default all DLLs are excluded by CommonBinaries.ezExportFilter
    // we want to override this for all the runtime DLLs and indirect DLL dependencies
    // so we add those to the 'include filter'

    for (auto it : ezQtEditorApp::GetSingleton()->GetPluginBundles().m_Plugins)
    {
      if (!it.Value().m_bSelected)
        continue;

      for (const auto& dep : it.Value().m_PackageDependencies)
      {
        binariesFilter.AddFilter(dep, true);
      }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
      for (const auto& dep : it.Value().m_PackageDependenciesDebug)
      {
        binariesFilter.AddFilter(dep, true);
      }
#elif EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
      for (const auto& dep : it.Value().m_PackageDependenciesDev)
      {
        binariesFilter.AddFilter(dep, true);
      }
#else
      for (const auto& dep : it.Value().m_PackageDependenciesShipping)
      {
        binariesFilter.AddFilter(dep, true);
      }
#endif
      for (const auto& dep : it.Value().m_RuntimePlugins)
      {
        ezStringBuilder tmp = dep;

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
        tmp.Append(".dll");
#elif EZ_ENABLED(EZ_PLATFORM_LINUX)
        tmp.Append(".so");
#else
#  error "Platform not implemented"
#endif

        binariesFilter.AddFilter(tmp, true);
      }
    }

    if (!sCustomGameExecutable.IsEmpty())
    {
      // the export filters exclude all executables by default, so the project's own game executable
      // has to be added explicitly, and ezPlayer isn't needed anymore
      ezStringBuilder sPattern("/", sCustomGameExecutable);
      binariesFilter.AddFilter(sPattern, true);

      for (ezUInt32 i = binariesFilter.m_IncludePatterns.GetCount(); i > 0; --i)
      {
        if (binariesFilter.m_IncludePatterns[i - 1].m_sString.IsEqual_NoCase("/ezPlayer.exe"))
        {
          binariesFilter.m_IncludePatterns.RemoveAtAndCopy(i - 1);
        }
      }

      ezLog::Info("Exporting '{}' as the game executable, ezPlayer is not exported.", sCustomGameExecutable);
    }

    mainProgress.BeginNextStep("Gathering binaries");
    EZ_SUCCEED_OR_RETURN(ezProjectExport::GatherBinaries(fileList, binariesFilter));
  }

  // 4
  {
    mainProgress.BeginNextStep("Copying files");
    EZ_SUCCEED_OR_RETURN(ezProjectExport::CopyAllFiles(fileList, szTargetDirectory));
  }

  // 5
  {
    mainProgress.BeginNextStep("Writing data directory config");
    EZ_SUCCEED_OR_RETURN(ezProjectExport::CreateDataDirectoryDDL(fileList, szTargetDirectory));
  }

  // 6
  {
    mainProgress.BeginNextStep("Finishing up");

    if (bCreateLaunchScripts)
    {
      EZ_SUCCEED_OR_RETURN(ezProjectExport::CreateLaunchConfig(sceneFiles, szTargetDirectory, sCustomGameExecutable));
    }
  }

  return EZ_SUCCESS;
}
