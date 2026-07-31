#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_ExportProjectDlg.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>

class ezQtExportProjectDlg : public ezQtDialog, public Ui_ExportProjectDlg
{
  Q_OBJECT

public:
  ezQtExportProjectDlg(QWidget* pParent);

  static bool s_bTransformAll;
  static bool s_bCreateLaunchScripts;
  static bool s_bOpenOutputFolder;

private Q_SLOTS:
  void on_ExportProjectButton_clicked();
  void on_BrowseDestination_clicked();


protected:
  virtual void showEvent(QShowEvent* e) override;
};
