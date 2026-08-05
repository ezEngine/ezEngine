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
bool ezQtExportProjectDlg::s_bCreateLaunchScripts = true;
bool ezQtExportProjectDlg::s_bOpenOutputFolder = true;

ezQtExportProjectDlg::ezQtExportProjectDlg(QWidget* pParent)
  : ezQtDialog(pParent)
{
  setupUi(this);

  ezProjectPreferencesUser* pPref = ezPreferences::QueryPreferences<ezProjectPreferencesUser>();

  Destination->setText(pPref->m_sExportFolder.GetData());
}

void ezQtExportProjectDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  TransformAll->setChecked(s_bTransformAll);
  CreateLaunchScripts->setChecked(s_bCreateLaunchScripts);
  OpenOutputFolder->setChecked(s_bOpenOutputFolder);

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

  s_bTransformAll = TransformAll->isChecked();
  s_bCreateLaunchScripts = CreateLaunchScripts->isChecked();
  s_bOpenOutputFolder = OpenOutputFolder->isChecked();

  ezProjectExportOptions options;
  options.m_bCompileCppPlugin = CompileCpp->isChecked();
  options.m_bTransformAssets = s_bTransformAll;
  options.m_bCreateLaunchScripts = s_bCreateLaunchScripts;

  const ezString sDstFolder = Destination->text().toUtf8().data();

  ezStringBuilder sLog;
  const ezStatus res = ezProjectExport::ExportProjectComplete(sDstFolder, options, &sLog);

  ExportLog->setPlainText(sLog.GetData());

  if (res.Failed())
  {
    ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Project export failed. See log for details.");
  }
  else
  {
    ezQtUiServices::GetSingleton()->MessageBoxInformation("Project export successful.", "project-export-success");

    if (s_bOpenOutputFolder)
    {
      ezQtUiServices::GetSingleton()->OpenInExplorer(sDstFolder, false);
    }
  }
}
