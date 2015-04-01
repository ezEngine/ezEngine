#pragma once

#include <Foundation/Basics.h>
#include <QDialog>
#include <Tools/EditorFramework/ui_PluginDlg.h>

class PluginDlg : public QDialog, public Ui_PluginDlg
{
public:
  Q_OBJECT

public:
  PluginDlg(QWidget* parent);


private slots:
  void on_ButtonOK_clicked();
  void on_ButtonCancel_clicked();

private:
  void FillPluginList();
};


