#pragma once

#include <EditorPlugins/Assets/EditorPluginAssets/ui_SceneImportDlg.h>
#include <QDialog>

class ezQtSceneImportDlg : public QDialog, public Ui_SceneImportDlg
{
  Q_OBJECT

public:
  ezQtSceneImportDlg(QWidget *parent = nullptr);

public Q_SLOTS:
  virtual void on_accepted();
  virtual void on_InputBrowse_clicked();
  virtual void on_OutputBrowse_clicked();

private:
  void validatePaths();
};

