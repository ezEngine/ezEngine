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

  if (TransformAll->isChecked())
  {
    ezStatus stat = ezAssetCurator::GetSingleton()->TransformAllAssets(ezTransformFlags::None);

    if (stat.Failed())
    {
      ezQtUiServices::GetSingleton()->MessageBoxStatus(stat, "Asset transform failed");
      return;
    }
  }

  const ezString sAssetProfile = ezAssetCurator::GetSingleton()->GetActiveAssetProfile()->GetConfigName();
  const ezString szDstFolder = Destination->text().toUtf8().data();

  ezLogSystemToBuffer logFile;

  auto WriteLogFile = [&]()
  {
    ezStringBuilder sTemp;
    sTemp.Set(szDstFolder, "/ExportLog.txt");

    ezOSFile file;

    if (file.Open(sTemp, ezFileOpenMode::Write).Failed())
    {
      ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to write export log '{0}'", sTemp));
      return;
    }

    file.Write(logFile.m_sBuffer.GetData(), logFile.m_sBuffer.GetElementCount()).AssertSuccess();
  };
  EZ_SCOPE_EXIT(WriteLogFile());

  ezLogSystemScope logScope(&logFile);

  const auto dataDirs = ezQtEditorApp::GetSingleton()->GetFileSystemConfig();



  ezProgressRange mainProgress("Export Project", 6, true);
  mainProgress.SetStepWeighting(0, 0.05f); // Preparing output folder
  mainProgress.SetStepWeighting(1, 0.10f); // Scanning data directories
  mainProgress.SetStepWeighting(2, 0.10f); // Filtering files
  mainProgress.SetStepWeighting(3, 0.05f); // Gathering binaries
  mainProgress.SetStepWeighting(4, 0.01f); // Writing data directory config
  // mainProgress.SetStepWeighting(5, 0.0f); // Copying files

  {
    mainProgress.BeginNextStep("Preparing output folder");
    if (ezProjectExport::ClearTargetFolder(szDstFolder).Failed())
      return;
  }
  ezStringBuilder sProjectRootDir;
  ezFileSystem::ResolveSpecialDirectory(">project", sProjectRootDir).AssertSuccess();
  sProjectRootDir.Trim("/\\");


  ezProjectExport::DirectoryMapping fileList;

  ezPathPatternFilter dataFilter;
  ezPathPatternFilter binariesFilter;

  if (ezProjectExport::ReadExportFilters(dataFilter, binariesFilter, sAssetProfile).Failed())
    return;

  ezProjectExport::GatherGeneratedAssetManagerFiles(fileList[sProjectRootDir].m_Files);
  if (ezProjectExport::GatherAssetLookupTableFiles(fileList, dataDirs, sAssetProfile).Failed())
    return;

  ezHybridArray<ezString, 16> sceneFiles;

  {
    mainProgress.BeginNextStep("Scanning data directories");

    if (ezProjectExport::ScanDataDirectories(fileList, dataDirs, dataFilter, &sceneFiles).Failed())
      return;
  }

  {
    mainProgress.BeginNextStep("Gathering binaries");

    if (ezProjectExport::GatherBinaries(fileList, binariesFilter).Failed())
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
