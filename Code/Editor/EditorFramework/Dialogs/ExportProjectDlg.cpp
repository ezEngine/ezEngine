#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/ExportProjectDlg.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <EditorFramework/Project/ProjectExport.h>
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

  ezStringBuilder sProjectRootDir;
  if (ezFileSystem::ResolveSpecialDirectory(">project", sProjectRootDir).Failed())
    return;
  sProjectRootDir.Trim("/\\");


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
    ezQtUiServices::GetSingleton()->MessageBoxStatus(ezProjectExport::ClearTargetFolder(szDstFolder), "Failed to clear output folder");
  }

  ezStringBuilder sTemp;
  sTemp.Set(szDstFolder, "/ExportLog.txt");

  ezLogSystemToFile logFile;
  if (logFile.Open(sTemp).Failed())
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

  ezPathPatternFilter dataFilter;
  ezPathPatternFilter binariesFilter;

  ezQtUiServices::GetSingleton()->MessageBoxStatus(ezProjectExport::ReadExportFilters(dataFilter, binariesFilter, pAssetProfile->GetConfigName()), "Setting up export configuration failed.");

  ezProjectExport::GatherGeneratedAssetManagerFiles(fileList[sProjectRootDir].m_Files);

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

  ezDynamicArray<ezString> sceneFiles;

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

      if (ezProjectExport::ScanFolder(ddInfo.m_Files, sStartPath, dataFilter, ezProgress::GetGlobalProgressbar(), ezAssetCurator::GetSingleton()).Failed())
        return;
    }
  }

  // Binaries
  {
    mainProgress.BeginNextStep("Gathering binaries");

    // ezProgressRange range("Gathering binaries", true);

    ezStringBuilder sAppDir;
    sAppDir = ezOSFile::GetApplicationDirectory();
    sAppDir.MakeCleanPath();
    sAppDir.Trim("/\\");

    DataDirInfo& ddInfo = fileList[sAppDir];
    ddInfo.m_sTargetDirPath = "Bin";
    ddInfo.m_sTargetDirRootName = "-"; // don't add to data dir config

    if (ezProjectExport::ScanFolder(ddInfo.m_Files, sAppDir, binariesFilter, ezProgress::GetGlobalProgressbar(), nullptr).Failed())
      return;
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



  {
    mainProgress.BeginNextStep("Copying files");

    ezUInt32 uiTotalFiles = 0;
    for (auto itDir = fileList.GetIterator(); itDir.IsValid(); ++itDir)
      uiTotalFiles += itDir.Value().m_Files.GetCount();

    ezProgressRange range("Copying files", uiTotalFiles, true);

    ezLog::Info(&logFile, "Output folder: {}", szDstFolder);

    ezStringBuilder sTargetFolder;

    for (auto itDir = fileList.GetIterator(); itDir.IsValid(); ++itDir)
    {
      sTargetFolder.Set(szDstFolder, "/", itDir.Value().m_sTargetDirPath);

      if (ezProjectExport::CopyFiles(itDir.Key(), sTargetFolder, itDir.Value().m_Files, ezProgress::GetGlobalProgressbar(), &range, &logFile).Failed())
        return;
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
