#pragma once

#include <Foundation/Basics.h>
#include <QDialog>
#include <Tools/EditorFramework/ui_EditorPluginConfigDlg.h>

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


