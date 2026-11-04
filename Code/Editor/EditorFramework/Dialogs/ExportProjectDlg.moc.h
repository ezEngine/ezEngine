#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Project/ProjectExport.h>
#include <EditorFramework/ui_ExportProjectDlg.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>

class ezQtExportProjectDlg : public ezQtDialog, public Ui_ExportProjectDlg
{
  Q_OBJECT

public:
  ezQtExportProjectDlg(QWidget* pParent);

  static bool s_bTransformAll;
  static bool s_bCompileCpp;
  static bool s_bRunPostExportScript;

private Q_SLOTS:
  void on_ExportProjectButton_clicked();
  void on_BrowseDestination_clicked();
  void on_OpenFolderButton_clicked();
  void on_SaveLogButton_clicked();
  void on_Destination_textChanged(const QString& sText);
  void on_TransformAll_clicked(bool bChecked);
  void on_CompileCpp_clicked(bool bChecked);
  void on_RunPostExportScript_clicked(bool bChecked);
  void on_HelpButton_clicked();
  void on_ProblemsOnly_toggled(bool bChecked);

protected:
  virtual void showEvent(QShowEvent* e) override;

private:
  void UpdateOpenFolderButton();
  void UpdateLogView();

  ezProjectExportReport m_Report;
};
