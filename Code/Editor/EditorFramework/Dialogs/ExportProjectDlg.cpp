#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/ExportProjectDlg.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <Foundation/IO/OSFile.h>
#include <QFileDialog>

bool ezQtExportProjectDlg::s_bTransformAll = true;

ezQtExportProjectDlg::ezQtExportProjectDlg(QWidget* parent)
  : QDialog(parent)
{
  setupUi(this);

  // ezProjectPreferencesUser* pPref = ezPreferences::QueryPreferences<ezProjectPreferencesUser>();
}

void ezQtExportProjectDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  TransformAll->setChecked(s_bTransformAll);
}

void ezQtExportProjectDlg::on_ExportProjectButton_clicked()
{
  accept();
}
