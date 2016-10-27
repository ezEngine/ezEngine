#pragma once

#include <EditorPluginAssets/ui_SceneImportDlg.h>
#include <QDialog>

class ezQtSceneImportDlg : public QDialog, public Ui_SceneImportDlg
{
  Q_OBJECT

public:
  ezQtSceneImportDlg(QWidget *parent = nullptr);

public slots:
  virtual void on_accepted();
  virtual void on_InputBrowse_clicked();
  virtual void on_OutputBrowse_clicked();

private:

};

