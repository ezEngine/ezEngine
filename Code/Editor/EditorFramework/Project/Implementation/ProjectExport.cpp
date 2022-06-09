#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Project/ProjectExport.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
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

ezResult ezProjectExport::ScanFolder(ezSet<ezString>& out_Files, const char* szFolder, const ezPathPatternFilter& filter, ezAssetCurator* pCurator)
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

    if (!filter.PassesFilters(sRelFilePath))
    {
      if (it.GetStats().m_bIsDirectory)
        it.SkipFolder();
      else
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

        sRelFilePath = pAssetMan->GetRelativeOutputFileName(asset->m_pAssetInfo->m_pDocumentTypeDescriptor, sRootFolder, asset->m_pAssetInfo->m_sAbsolutePath, nullptr);

        sRelFilePath.Prepend("AssetCache/");
        out_Files.Insert(sRelFilePath);

        // TODO
        // if (asset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sDocumentTypeName == "Scene")
        //{
        //  sTemp.Set(sRootFolder, "/", sRelFilePath);
        //  sceneFiles.PushBack(sTemp);
        //}

        for (const ezString& outputTag : asset->m_pAssetInfo->m_Info->m_Outputs)
        {
          sRelFilePath = pAssetMan->GetRelativeOutputFileName(asset->m_pAssetInfo->m_pDocumentTypeDescriptor, sRootFolder, asset->m_pAssetInfo->m_sAbsolutePath, outputTag);

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
  ezLog::Info("Source folder: ", szSrcFolder);
  ezLog::Info("Destination folder: ", szDstFolder);

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

  return EZ_SUCCESS;
}

void ezProjectExport::GatherGeneratedAssetManagerFiles(ezSet<ezString>& out_Files)
{
  ezHybridArray<ezString, 4> addFiles;

  for (auto pMan : ezDocumentManager::GetAllDocumentManagers())
  {
    if (auto pAssMan = ezDynamicCast<ezAssetDocumentManager*>(pMan))
    {
      pAssMan->GetAdditionalOutputs(addFiles);

      for (const auto& file : addFiles)
      {
        out_Files.Insert(file);
      }

      addFiles.Clear();
    }
  }
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

ezResult ezProjectExport::ReadExportFilters(ezPathPatternFilter& out_DataFilter, ezPathPatternFilter& out_BinariesFilter, const char* szPlatformProfileName)
{
  ezStringBuilder sDefine;
  sDefine.Format("PLATFORM_PROFILE_{} 1", szPlatformProfileName);
  sDefine.ToUpper();

  ezHybridArray<ezString, 1> ppDefines;
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

  sPath.Set(szTargetDirectory, "/Data/project/DataDirectories.ddl");

  if (cfg.Save(sPath).Failed())
  {
    ezLog::Error("Failed to write DataDirectories.ddl file.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezProjectExport::GatherAssetLookupTableFiles(DirectoryMapping& mapping, const ezApplicationFileSystemConfig& dirConfig, const char* szPlatformProfileName)
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

    ezStringBuilder sAidltPath("AssetCache/", szPlatformProfileName, ".ezAidlt");

    mapping[sDataDirPath].m_Files.Insert(sAidltPath);
  }

  return EZ_SUCCESS;
}

ezResult ezProjectExport::ScanDataDirectories(DirectoryMapping& mapping, const ezApplicationFileSystemConfig& dirConfig, const ezPathPatternFilter& dataFilter)
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
      sDstPath.Format("Data/Extra{}", uiDataDirNumber);
      ++uiDataDirNumber;

      ddInfo.m_sTargetDirPath = sDstPath;
    }

    EZ_SUCCEED_OR_RETURN(ezProjectExport::ScanFolder(ddInfo.m_Files, sDataDirPath, dataFilter, ezAssetCurator::GetSingleton()));
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

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

ezLogSystemToFile::ezLogSystemToFile() = default;
ezLogSystemToFile::~ezLogSystemToFile() = default;

ezResult ezLogSystemToFile::Open(const char* szAbsFile)
{
  return m_File.Open(szAbsFile, ezFileOpenMode::Write);
}

void ezLogSystemToFile::HandleLogMessage(const ezLoggingEventData& le)
{
  if (!m_File.IsOpen())
    return;

  ezStringBuilder tmp;

  switch (le.m_EventType)
  {
    case ezLogMsgType::ErrorMsg:
      tmp.Append("Error: ", le.m_szText, "\n");
      break;
    case ezLogMsgType::SeriousWarningMsg:
    case ezLogMsgType::WarningMsg:
      tmp.Append("Warning: ", le.m_szText, "\n");
      break;
    case ezLogMsgType::SuccessMsg:
    case ezLogMsgType::InfoMsg:
    case ezLogMsgType::DevMsg:
    case ezLogMsgType::DebugMsg:
      tmp.Append(le.m_szText, "\n");
      break;

    default:
      return;
  }

  m_File.Write(tmp.GetData(), tmp.GetElementCount()).AssertSuccess();
}

//////////////////////////////////////////////////////////////////////////
