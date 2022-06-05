#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_ExportProjectDlg.h>
#include <QDialog>

class ezQtExportProjectDlg : public QDialog, public Ui_ExportProjectDlg
{
  Q_OBJECT

public:
  ezQtExportProjectDlg(QWidget* parent);

  static bool s_bTransformAll;

private Q_SLOTS:
  void on_ExportProjectButton_clicked();


protected:
  virtual void showEvent(QShowEvent* e) override;
};

