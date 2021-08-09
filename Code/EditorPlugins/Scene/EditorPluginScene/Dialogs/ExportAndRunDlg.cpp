#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Dialogs/ExportAndRunDlg.moc.h>

bool ezQtExportAndRunDlg::s_bTransformAll = true;
bool ezQtExportAndRunDlg::s_bUpdateThumbnail = false;

ezQtExportAndRunDlg::ezQtExportAndRunDlg(QWidget* parent)
  : QDialog(parent)
{
  setupUi(this);
}

void ezQtExportAndRunDlg::PullFromUI()
{
  s_bTransformAll = TransformAll->isChecked();
  s_bUpdateThumbnail = UpdateThumbnail->isChecked();
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
  accept();
}
