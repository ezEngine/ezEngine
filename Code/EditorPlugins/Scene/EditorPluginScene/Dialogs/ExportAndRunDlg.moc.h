#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <EditorPluginScene/ui_ExportAndRunDlg.h>
#include <QDialog>

class ezQtExportAndRunDlg : public QDialog, public Ui_ExportAndRunDlg
{
  Q_OBJECT

public:
  ezQtExportAndRunDlg(QWidget* parent);

  static bool s_bTransformAll;
  static bool s_bUpdateThumbnail;
  bool m_bRunAfterExport = false;
  ezString m_sCmdLine;

private Q_SLOTS:
  void on_ExportOnly_clicked();
  void on_ExportAndRun_clicked();

private:
  void PullFromUI();

protected:
  virtual void showEvent(QShowEvent* e) override;
};
