#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/ExportProjectDlg.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/String.h>
#include <QFileDialog>
#include <ToolsFoundation/Utilities/PathPatternFilter.h>


bool ezQtExportProjectDlg::s_bTransformAll = true;

ezQtExportProjectDlg::ezQtExportProjectDlg(QWidget* parent)
  : QDialog(parent)
{
  setupUi(this);

  ezProjectPreferencesUser* pPref = ezPreferences::QueryPreferences<ezProjectPreferencesUser>();

  Destination->setText(pPref->m_sExportFolder.GetData());
}

void ezQtExportProjectDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  TransformAll->setChecked(s_bTransformAll);
}

void ezQtExportProjectDlg::on_BrowseDestination_clicked()
{
  QString sPath = QFileDialog::getExistingDirectory(this, QLatin1String("Select output directory"), Destination->text());

  if (!sPath.isEmpty())
  {
    Destination->setText(sPath);
    ezProjectPreferencesUser* pPref = ezPreferences::QueryPreferences<ezProjectPreferencesUser>();
    pPref->m_sExportFolder = sPath.toUtf8().data();
  }
}

void ezQtExportProjectDlg::on_ExportProjectButton_clicked()
{
  // TODO:
  // filter out unused runtime/game plugins
  // output log to UI
  // asset profile
  // copy inputs into resource: RML files
  // code cleanup

  if (TransformAll->isChecked())
  {
    ezStatus stat = ezAssetCurator::GetSingleton()->TransformAllAssets(ezTransformFlags::None);

    if (stat.Failed())
    {
      ezQtUiServices::GetSingleton()->MessageBoxStatus(stat, "Asset transform failed");
      return;
    }
  }

  const ezPlatformProfile* pAssetProfile = ezAssetCurator::GetSingleton()->GetActiveAssetProfile();

  ezProgressRange mainProgress("Export Project", 6, true);
  mainProgress.SetStepWeighting(0, 0.05f); // Preparing output folder
  mainProgress.SetStepWeighting(1, 0.10f); // Scanning data directories
  mainProgress.SetStepWeighting(2, 0.10f); // Filtering files
  mainProgress.SetStepWeighting(3, 0.05f); // Gathering binaries
  mainProgress.SetStepWeighting(4, 0.01f); // Writing data directory config
  // mainProgress.SetStepWeighting(5, 0.0f); // Copying files

  const ezString szDstFolder = Destination->text().toUtf8().data();

  {
    mainProgress.BeginNextStep("Preparing output folder");

    if (ezOSFile::DeleteFolder(szDstFolder).Failed())
    {
      ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to remove destination folder:\n'{}'", szDstFolder));
      return;
    }

    if (ezOSFile::CreateDirectoryStructure(szDstFolder).Failed())
    {
      ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to create destination folder:\n'{}'", szDstFolder));
      return;
    }
  }

  ezStringBuilder sTemp;

  ezOSFile logFile;

  sTemp.Set(szDstFolder, "/ExportLog.txt");
  if (logFile.Open(sTemp, ezFileOpenMode::Write).Failed())
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to create log file '{0}'", sTemp));
    return;
  }

  const auto dataDirs = ezQtEditorApp::GetSingleton()->GetFileSystemConfig();

  ezStringBuilder sPath, sRelPath, sStartPath;

  struct DataDirInfo
  {
    ezString m_sTargetDirPath;
    ezString m_sTargetDirRootName;
    ezSet<ezString> m_Files;
  };

  ezMap<ezString, DataDirInfo> fileList;

  auto logToFile = [&](const char* msg, const char* msg2)
  {
    logFile.Write(msg, ezStringUtils::GetStringElementCount(msg)).AssertSuccess();
    logFile.Write(msg2, ezStringUtils::GetStringElementCount(msg2)).AssertSuccess();
    logFile.Write("\n", 1).AssertSuccess();
  };

  ezPathPatternFilter dataFilter;
  ezPathPatternFilter binariesFilter;

  sTemp.Format("PLATFORM_PROFILE_{} 1", pAssetProfile->GetConfigName());
  sTemp.ToUpper();

  ezHybridArray<ezString, 1> ppDefines;
  ppDefines.PushBack(sTemp);

  if (dataFilter.ReadConfigFile("ProjectData.ezExportFilter", ppDefines).Failed())
  {
    ezQtUiServices::GetSingleton()->MessageBoxInformation(ezFmt("The config file 'ProjectData.ezExportFilter' does not exist or is invalid.\n\nUsing 'CommonData.ezExportFilter' instead."));

    if (dataFilter.ReadConfigFile("CommonData.ezExportFilter", ppDefines).Failed())
    {
      ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("The config file 'CommonData.ezExportFilter' does not exist or is invalid.\n\nCanceling operation.."));

      return;
    }
  }

  if (binariesFilter.ReadConfigFile("ProjectBinaries.ezExportFilter", ppDefines).Failed())
  {
    ezQtUiServices::GetSingleton()->MessageBoxInformation(ezFmt("The config file 'ProjectBinaries.ProjectBinaries' does not exist or is invalid.\n\nUsing 'CommonBinaries.ezExportFilter' instead."));

    if (binariesFilter.ReadConfigFile("CommonBinaries.ezExportFilter", ppDefines).Failed())
    {
      ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("The config file 'CommonBinaries.ezExportFilter' does not exist or is invalid.\n\nCanceling operation.."));

      return;
    }
  }

  // asset document manager generated files
  {
    ezDynamicArray<ezString> addFiles;

    for (auto pMan : ezDocumentManager::GetAllDocumentManagers())
    {
      if (auto pAssMan = ezDynamicCast<ezAssetDocumentManager*>(pMan))
      {
        pAssMan->GetAdditionalOutputs(addFiles);
      }
    }

    if (ezFileSystem::ResolveSpecialDirectory(">project", sStartPath).Succeeded())
    {
      sStartPath.Trim("/\\");

      DataDirInfo& ddInfo = fileList[sStartPath];
      ezSet<ezString>& ddFileList = ddInfo.m_Files;

      for (const auto& file : addFiles)
      {
        ddFileList.Insert(file);
      }
    }
  }

  // ezAidlt files
  {
    for (const auto& dataDir : dataDirs.m_DataDirs)
    {
      if (ezFileSystem::ResolveSpecialDirectory(dataDir.m_sDataDirSpecialPath, sStartPath).Failed())
      {
        ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to get special directory '{0}'", dataDir.m_sDataDirSpecialPath));
        return;
      }

      ezStringBuilder sAidltPath("AssetCache/", pAssetProfile->GetConfigName(), ".ezAidlt");

      sStartPath.Trim("/\\");
      fileList[sStartPath].m_Files.Insert(sAidltPath);
    }
  }

  {
    mainProgress.BeginNextStep("Scanning data directories");

    ezProgressRange progress("Scanning data directories", dataDirs.m_DataDirs.GetCount(), true);

    ezUInt32 uiDataDirNumber = 1;

    for (const auto& dataDir : dataDirs.m_DataDirs)
    {
      progress.BeginNextStep(dataDir.m_sDataDirSpecialPath);

      if (ezFileSystem::ResolveSpecialDirectory(dataDir.m_sDataDirSpecialPath, sStartPath).Failed())
      {
        ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to get special directory '{0}'", dataDir.m_sDataDirSpecialPath));
        return;
      }

      sStartPath.Trim("/\\");
      const ezUInt32 uiStrip = sStartPath.GetElementCount();

      DataDirInfo& ddInfo = fileList[sStartPath];
      ezSet<ezString>& ddFileList = ddInfo.m_Files;

      if (!dataDir.m_sRootName.IsEmpty())
      {
        sTemp.Set("Data/", dataDir.m_sRootName);

        ddInfo.m_sTargetDirRootName = dataDir.m_sRootName;
        ddInfo.m_sTargetDirPath = sTemp;
      }
      else
      {
        sTemp.Format("Data/Extra{}", uiDataDirNumber);

        ddInfo.m_sTargetDirPath = sTemp;
      }

      ezFileSystemIterator it;
      for (it.StartSearch(sStartPath, ezFileSystemIteratorFlags::ReportFilesAndFoldersRecursive); it.IsValid();)
      {
        if (ezProgress::GetGlobalProgressbar()->WasCanceled())
          return;

        it.GetStats().GetFullPath(sPath);

        sRelPath = sPath;
        sRelPath.Shrink(uiStrip, 0);

        if (it.GetStats().m_bIsDirectory)
        {
          if (!dataFilter.PassesFilters(sRelPath))
          {
            it.SkipFolder();
          }
          else
          {
            it.Next();
          }

          continue;
        }

        if (!dataFilter.PassesFilters(sRelPath))
        {
          it.Next();
          continue;
        }

        auto asset = ezAssetCurator::GetSingleton()->FindSubAsset(sPath);
        if (asset.isValid())
        {
          // redirect to asset output
          if (asset->m_bMainAsset)
          {
            ezAssetDocumentManager* pAssetMan = ezStaticCast<ezAssetDocumentManager*>(asset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_pManager);

            ezStringBuilder sRelFile = pAssetMan->GetRelativeOutputFileName(asset->m_pAssetInfo->m_pDocumentTypeDescriptor, sStartPath, asset->m_pAssetInfo->m_sAbsolutePath, nullptr);

            sRelFile.Prepend("AssetCache/");
            ddFileList.Insert(sRelFile);

            for (const ezString& outputTag : asset->m_pAssetInfo->m_Info->m_Outputs)
            {
              sRelFile = pAssetMan->GetRelativeOutputFileName(asset->m_pAssetInfo->m_pDocumentTypeDescriptor, sStartPath, asset->m_pAssetInfo->m_sAbsolutePath, outputTag);

              sRelFile.Prepend("AssetCache/");
              ddFileList.Insert(sRelFile);
            }
          }

          // ignore all asset files
          it.Next();
          continue;
        }

        ddFileList.Insert(sRelPath);
        it.Next();
      }
    }
  }

  // Binaries
  {
    mainProgress.BeginNextStep("Gathering binaries");

    // ezProgressRange range("Gathering binaries", true);

    sStartPath = ezOSFile::GetApplicationDirectory();
    sStartPath.MakeCleanPath();
    sStartPath.Trim("/\\");
    const ezUInt32 uiStrip = sStartPath.GetElementCount();

    DataDirInfo& ddInfo = fileList[sStartPath];
    ezSet<ezString>& ddFileList = ddInfo.m_Files;

    ddInfo.m_sTargetDirPath = "Bin";
    ddInfo.m_sTargetDirRootName = "-"; // don't add to data dir config

    ezFileSystemIterator it;
    for (it.StartSearch(sStartPath, ezFileSystemIteratorFlags::ReportFilesAndFoldersRecursive); it.IsValid();)
    {
      if (ezProgress::GetGlobalProgressbar()->WasCanceled())
        return;

      it.GetStats().GetFullPath(sPath);

      sRelPath = sPath;
      sRelPath.Shrink(uiStrip, 0);

      if (!binariesFilter.PassesFilters(sRelPath))
      {
        if (it.GetStats().m_bIsDirectory)
          it.SkipFolder();
        else
          it.Next();

        continue;
      }

      ddFileList.Insert(sRelPath);
      it.Next();
    }
  }

  // write data dir config file
  {
    mainProgress.BeginNextStep("Writing data directory config");

    ezApplicationFileSystemConfig cfg;

    for (auto itDir = fileList.GetIterator(); itDir.IsValid(); ++itDir)
    {
      const auto& info = itDir.Value();

      if (info.m_sTargetDirRootName == "-")
        continue;

      sPath.Set(">sdk/", info.m_sTargetDirPath);

      auto& ddc = cfg.m_DataDirs.ExpandAndGetRef();
      ddc.m_sDataDirSpecialPath = sPath;
      ddc.m_sRootName = info.m_sTargetDirRootName;
    }

    sPath.Set(szDstFolder, "/Data/project/DataDirectories.ddl");
    if (cfg.Save(sPath).Failed())
    {
      if (cfg.Save(sPath).Failed())
        ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to write data directory config file '{0}'", sPath));
      return;
    }
  }

  ezDynamicArray<ezString> sceneFiles;

  {
    mainProgress.BeginNextStep("Copying files");

    ezUInt32 uiTotalFiles = 0;
    for (auto itDir = fileList.GetIterator(); itDir.IsValid(); ++itDir)
      uiTotalFiles += itDir.Value().m_Files.GetCount();

    ezProgressRange range("Copying files", uiTotalFiles, true);

    logToFile("Output folder: ", szDstFolder);

    for (auto itDir = fileList.GetIterator(); itDir.IsValid(); ++itDir)
    {
      logToFile("Source sub-folder: ", itDir.Key());
      logToFile("Destination sub-folder: ", itDir.Value().m_sTargetDirPath);

      for (auto itFile = itDir.Value().m_Files.GetIterator(); itFile.IsValid(); ++itFile)
      {
        if (ezProgress::GetGlobalProgressbar()->WasCanceled())
          return;

        range.BeginNextStep(itFile.Key());

        sPath.Set(itDir.Key(), "/", itFile.Key());
        sTemp.Set(szDstFolder, "/", itDir.Value().m_sTargetDirPath, "/", itFile.Key());

        if (ezOSFile::CopyFile(sPath, sTemp).Succeeded())
        {
          logToFile(" -> ", itFile.Key());

          if (sTemp.EndsWith_NoCase(".ezObjectGraph") && sTemp.FindSubString_NoCase("scene"))
          {
            sceneFiles.PushBack(sTemp);
          }
        }
        else
        {
          logToFile(" Copy failed:", itFile.Key());
        }
      }
    }
  }

  // write .bat files
  for (const auto& sf : sceneFiles)
  {
    ezStringBuilder cmd;
    cmd.Format("start Bin/Player.exe -scene \"{}", sf);

    ezStringBuilder bat;
    bat.Format("{}/Launch {}.bat", szDstFolder, ezPathUtils::GetFileName(sf));

    ezOSFile file;
    if (file.Open(bat, ezFileOpenMode::Write).Succeeded())
    {
      file.Write(cmd.GetData(), cmd.GetElementCount()).AssertSuccess();
    }
  }

  accept();
}
