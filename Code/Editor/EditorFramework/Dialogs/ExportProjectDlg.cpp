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

  ezLogSystemScope logScope(&logFile);



  const auto dataDirs = ezQtEditorApp::GetSingleton()->GetFileSystemConfig();

  ezProjectExport::DirectoryMapping fileList;

  ezPathPatternFilter dataFilter;
  ezPathPatternFilter binariesFilter;

  if (ezProjectExport::ReadExportFilters(dataFilter, binariesFilter, pAssetProfile->GetConfigName()).Failed())
    return;

  ezProjectExport::GatherGeneratedAssetManagerFiles(fileList[sProjectRootDir].m_Files);
  if (ezProjectExport::GatherAssetLookupTableFiles(fileList, dataDirs, pAssetProfile->GetConfigName()).Failed())
    return;

  ezDynamicArray<ezString> sceneFiles;

  {
    mainProgress.BeginNextStep("Scanning data directories");

    if (ezProjectExport::ScanDataDirectories(fileList, dataDirs, dataFilter).Failed())
      return;
  }

  // Binaries
  {
    mainProgress.BeginNextStep("Gathering binaries");

    // ezProgressRange range("Gathering binaries", true);

    ezStringBuilder sAppDir;
    sAppDir = ezOSFile::GetApplicationDirectory();
    sAppDir.MakeCleanPath();
    sAppDir.Trim("/\\");

    ezProjectExport::DataDirectory& ddInfo = fileList[sAppDir];
    ddInfo.m_sTargetDirPath = "Bin";
    ddInfo.m_sTargetDirRootName = "-"; // don't add to data dir config

    if (ezProjectExport::ScanFolder(ddInfo.m_Files, sAppDir, binariesFilter, nullptr).Failed())
      return;
  }

  {
    mainProgress.BeginNextStep("Copying files");

    if (ezProjectExport::CopyAllFiles(fileList, szDstFolder).Failed())
      return;
  }

  {
    mainProgress.BeginNextStep("Writing data directory config");

    if (ezProjectExport::CreateDataDirectoryDDL(fileList, szDstFolder).Failed())
      return;
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
