#include <EditorPluginScenePCH.h>

#include <EditorPluginScene/Dialogs/ExportAndRunDlg.moc.h>

bool ezQtExportAndRunDlg::s_bTransformAll = true;

ezQtExportAndRunDlg::ezQtExportAndRunDlg(QWidget* parent)
  : QDialog(parent)
{
  setupUi(this);
}

void ezQtExportAndRunDlg::PullFromUI()
{
  s_bTransformAll = TransformAll->isChecked();
}

void ezQtExportAndRunDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  TransformAll->setChecked(s_bTransformAll);
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
