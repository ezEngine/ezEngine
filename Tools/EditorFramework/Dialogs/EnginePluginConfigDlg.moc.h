#pragma once

#include <Foundation/Basics.h>
#include <QDialog>
#include <Tools/EditorFramework/ui_EnginePluginConfigDlg.h>

class ezQtEnginePluginConfigDlg : public QDialog, public Ui_EnginePluginConfigDlg
{
public:
  Q_OBJECT

public:
  ezQtEnginePluginConfigDlg(QWidget* parent);


private slots:
  void on_ButtonOK_clicked();
  void on_ButtonCancel_clicked();

private:
  void FillPluginList();
};


