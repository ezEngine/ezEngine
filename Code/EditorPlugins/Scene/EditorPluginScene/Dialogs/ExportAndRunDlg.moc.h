#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <EditorPluginScene/ui_ExportAndRunDlg.h>
#include <QDialog>

class ezSceneDocument;

class ezQtExportAndRunDlg : public QDialog, public Ui_ExportAndRunDlg
{
  Q_OBJECT

public:
  ezQtExportAndRunDlg(QWidget* pParent);

  static bool s_bTransformAll;
  static bool s_bUpdateThumbnail;
  bool m_bRunAfterExport = false;
  bool m_bShowThumbnailCheckbox = true;
  ezString m_sCmdLine;
  ezString m_sApplication;

private Q_SLOTS:
  void on_ExportOnly_clicked();
  void on_ExportAndRun_clicked();
  void on_AddToolButton_clicked();
  void on_RemoveToolButton_clicked();
  void on_ToolCombo_currentIndexChanged(int);

private:
  void PullFromUI();

protected:
  virtual void showEvent(QShowEvent* e) override;
};

