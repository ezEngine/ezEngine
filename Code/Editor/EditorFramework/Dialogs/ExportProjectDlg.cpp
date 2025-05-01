#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/Dialogs/ExportProjectDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <EditorFramework/Project/ProjectExport.h>
#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QFileDialog>
#include <ToolsFoundation/Utilities/PathPatternFilter.h>


bool ezQtExportProjectDlg::s_bTransformAll = true;

ezQtExportProjectDlg::ezQtExportProjectDlg(QWidget* pParent)
  : QDialog(pParent)
{
  setupUi(this);

  ezProjectPreferencesUser* pPref = ezPreferences::QueryPreferences<ezProjectPreferencesUser>();

  Destination->setText(pPref->m_sExportFolder.GetData());
}

void ezQtExportProjectDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  TransformAll->setChecked(s_bTransformAll);

  if (!ezCppProject::ExistsProjectCMakeListsTxt())
  {
    CompileCpp->setEnabled(false);
    CompileCpp->setToolTip("This project doesn't have a C++ plugin.");
    CompileCpp->setChecked(false);
  }
  else
  {
    CompileCpp->setChecked(true);
  }
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
  // select asset profile for export
  // copy inputs into resource: RML files

  if (CompileCpp->isChecked())
  {
    if (ezCppProject::EnsureCppPluginReady().Failed())
      return;
  }

  if (TransformAll->isChecked())
  {
    ezStatus stat = ezAssetCurator::GetSingleton()->TransformAllAssets();

    if (stat.Failed())
    {
      ezQtUiServices::GetSingleton()->MessageBoxStatus(stat, "Asset transform failed");
      return;
    }
  }

  const ezString szDstFolder = Destination->text().toUtf8().data();

  ezLogSystemToBuffer logFile;

  auto WriteLogFile = [&]()
  {
    ezStringBuilder sTemp;
    sTemp.Set(szDstFolder, "/ExportLog.txt");

    ExportLog->setPlainText(logFile.m_sBuffer.GetData());

    ezOSFile file;

    if (file.Open(sTemp, ezFileOpenMode::Write).Failed())
    {
      ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to write export log '{0}'", sTemp));
      return;
    }

    file.Write(logFile.m_sBuffer.GetData(), logFile.m_sBuffer.GetElementCount()).AssertSuccess();
  };

  ezLogSystemScope logScope(&logFile);
  EZ_SCOPE_EXIT(WriteLogFile());

  if (ezProjectExport::ExportProject(szDstFolder, ezAssetCurator::GetSingleton()->GetActiveAssetProfile(), ezQtEditorApp::GetSingleton()->GetFileSystemConfig()).Failed())
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning("Project export failed. See log for details.");
  }
  else
  {
    ezQtUiServices::GetSingleton()->MessageBoxInformation("Project export successful.");
    ezQtUiServices::GetSingleton()->OpenInExplorer(szDstFolder, false);
  }
}
