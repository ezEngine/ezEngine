#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <EditorPluginScene/Dialogs/ExportAndRunDlg.moc.h>
#include <Foundation/IO/OSFile.h>
#include <QFileDialog>

bool ezQtExportAndRunDlg::s_bTransformAll = true;
bool ezQtExportAndRunDlg::s_bUpdateThumbnail = false;

static int s_iLastPlayerApp = 0;

ezQtExportAndRunDlg::ezQtExportAndRunDlg(QWidget* pParent)
  : QDialog(pParent)
{
  setupUi(this);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  ToolCombo->addItem("ezPlayer", "Player.exe");
#else
  ToolCombo->addItem("ezPlayer", "Player");
#endif

  ezProjectPreferencesUser* pPref = ezPreferences::QueryPreferences<ezProjectPreferencesUser>();

  for (const auto& app : pPref->m_PlayerApps)
  {
    ezStringBuilder name = ezPathUtils::GetFileName(app);

    ToolCombo->addItem(name.GetData(), QString::fromUtf8(app.GetData()));
  }

  ToolCombo->setCurrentIndex(s_iLastPlayerApp);
}

void ezQtExportAndRunDlg::PullFromUI()
{
  s_bTransformAll = TransformAll->isChecked();
  s_bUpdateThumbnail = UpdateThumbnail->isChecked();
  s_iLastPlayerApp = ToolCombo->currentIndex();

  ezProjectPreferencesUser* pPref = ezPreferences::QueryPreferences<ezProjectPreferencesUser>();
  pPref->m_PlayerApps.Clear();

  for (int i = 1; i < ToolCombo->count(); ++i)
  {
    ezStringBuilder path = ToolCombo->itemData(i).toString().toUtf8().data();
    path.MakeCleanPath();

    pPref->m_PlayerApps.PushBack(path);
  }
}

void ezQtExportAndRunDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  UpdateThumbnail->setVisible(m_bShowThumbnailCheckbox);
  TransformAll->setChecked(s_bTransformAll);
  UpdateThumbnail->setChecked(s_bUpdateThumbnail);
  PlayerCmdLine->setPlainText(m_sCmdLine.GetData());
}

void ezQtExportAndRunDlg::on_ExportOnly_clicked()
{
  PullFromUI();
  m_bRunAfterExport = false;
  accept();
}

void ezQtExportAndRunDlg::on_ExportAndRun_clicked()
{
  PullFromUI();
  m_bRunAfterExport = true;
  m_sApplication = ToolCombo->currentData().toString().toUtf8().data();
  accept();
}

void ezQtExportAndRunDlg::on_AddToolButton_clicked()
{
  ezStringBuilder appDir = ezOSFile::GetApplicationDirectory();
  appDir.MakeCleanPath();
  static QString sLastPath = appDir.GetData();

  const QString sFile = QFileDialog::getOpenFileName(this, "Select Program", sLastPath, "Applicaation (*.exe)", nullptr, QFileDialog::Option::DontResolveSymlinks);

  if (sFile.isEmpty())
    return;

  sLastPath = sFile;

  ezStringBuilder path = sFile.toUtf8().data();
  path.MakeCleanPath();
  path.TrimWordStart(appDir, "/");

  ezStringBuilder tmp;
  ToolCombo->addItem(QString::fromUtf8(path.GetFileName().GetData(tmp)), QString::fromUtf8(path.GetData()));
  ToolCombo->setCurrentIndex(ToolCombo->count() - 1);
}

void ezQtExportAndRunDlg::on_RemoveToolButton_clicked()
{
  ToolCombo->removeItem(ToolCombo->currentIndex());
}

void ezQtExportAndRunDlg::on_ToolCombo_currentIndexChanged(int idx)
{
  RemoveToolButton->setEnabled(idx != 0);
}
