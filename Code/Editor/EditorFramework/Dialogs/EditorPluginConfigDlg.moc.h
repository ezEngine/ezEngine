#pragma once

#include <EditorFramework/ui_EditorPluginConfigDlg.h>
#include <Foundation/Basics.h>
#include <QDialog>

class ezQtEditorPluginConfigDlg : public QDialog, public Ui_ezQtEditorPluginConfigDlg
{
public:
  Q_OBJECT

public:
  ezQtEditorPluginConfigDlg(QWidget* parent);


private Q_SLOTS:
  void on_ButtonOK_clicked();
  void on_ButtonCancel_clicked();

private:
  void FillPluginList();
};

