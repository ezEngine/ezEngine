#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Project/ProjectExport.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Utilities/PathPatternFilter.h>

ezStatus ezProjectExport::ClearTargetFolder(const char* szAbsFolderPath)
{
  if (ezOSFile::DeleteFolder(szAbsFolderPath).Failed())
  {
    return ezStatus(ezFmt("Target folder could not be removed:\n'{}'", szAbsFolderPath));
  }

  if (ezOSFile::CreateDirectoryStructure(szAbsFolderPath).Failed())
  {
    return ezStatus(ezFmt("Target folder could not be created:\n'{}'", szAbsFolderPath));
  }

  return ezStatus(EZ_SUCCESS);
}

ezResult ezProjectExport::ScanFolder(ezSet<ezString>& out_Files, const char* szFolder, const ezPathPatternFilter& filter, ezProgress* pProgress, ezAssetCurator* pCurator)
{
  ezStringBuilder sRootFolder = szFolder;
  sRootFolder.Trim("/\\");

  const ezUInt32 uiRootFolderLength = sRootFolder.GetElementCount();

  ezStringBuilder sAbsFilePath, sRelFilePath;

  ezFileSystemIterator it;
  for (it.StartSearch(sRootFolder, ezFileSystemIteratorFlags::ReportFilesAndFoldersRecursive); it.IsValid();)
  {
    if (pProgress && pProgress->WasCanceled())
      return EZ_FAILURE;

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

ezResult ezProjectExport::CopyFiles(const char* szSrcFolder, const char* szDstFolder, const ezSet<ezString>& files, ezProgress* pProgress, ezProgressRange* pProgressRange, ezLogInterface* pLog)
{
  ezLog::Info(pLog, "Source folder: ", szSrcFolder);
  ezLog::Info(pLog, "Destination folder: ", szDstFolder);

  ezStringBuilder sSrc, sDst;

  for (auto itFile = files.GetIterator(); itFile.IsValid(); ++itFile)
  {
    if (pProgress && pProgress->WasCanceled())
    {
      ezLog::Info(pLog, "File copy operation canceled by user.");
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
      ezLog::Info(pLog, " Copied: {}", itFile.Key());
    }
    else
    {
      ezLog::Error(pLog, " Copy failed: {}", itFile.Key());
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

  // ezStringBuilder sTarget;
  // sTarget.Set(">project/", szExpectedFile);

  ezFileWriter file;
  EZ_SUCCEED_OR_RETURN(file.Open(szExpectedFile));
  EZ_SUCCEED_OR_RETURN(file.WriteBytes(src.GetData(), src.GetElementCount()));

  return EZ_SUCCESS;
}

ezStatus ezProjectExport::ReadExportFilters(ezPathPatternFilter& out_DataFilter, ezPathPatternFilter& out_BinariesFilter, const char* szPlatformProfileName)
{
  ezStringBuilder sDefine;
  sDefine.Format("PLATFORM_PROFILE_{} 1", szPlatformProfileName);
  sDefine.ToUpper();

  ezHybridArray<ezString, 1> ppDefines;
  ppDefines.PushBack(sDefine);

  if (ezProjectExport::CreateExportFilterFile(":project/ProjectData.ezExportFilter", "CommonData.ezExportFilter").Failed())
  {
    return ezStatus(ezFmt("The file 'ProjectData.ezExportFilter' could not be created."));
  }

  if (ezProjectExport::CreateExportFilterFile(":project/ProjectBinaries.ezExportFilter", "CommonBinaries.ezExportFilter").Failed())
  {
    return ezStatus(ezFmt("The file 'ProjectBinaries.ezExportFilter' could not be created."));
  }

  if (out_DataFilter.ReadConfigFile("ProjectData.ezExportFilter", ppDefines).Failed())
  {
    return ezStatus(ezFmt("The file 'ProjectData.ezExportFilter' could not be read."));
  }

  if (out_BinariesFilter.ReadConfigFile("ProjectBinaries.ezExportFilter", ppDefines).Failed())
  {
    return ezStatus(ezFmt("The file 'ProjectBinaries.ezExportFilter' could not be read."));
  }

  return ezStatus(EZ_SUCCESS);
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
