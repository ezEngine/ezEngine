#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_LaunchFileserveDlg.h>
#include <Foundation/Strings/String.h>
#include <QDialog>

class EZ_EDITORFRAMEWORK_DLL ezQtLaunchFileserveDlg : public QDialog, public Ui_ezQtLaunchFileserveDlg
{
public:
  Q_OBJECT

public:
  ezQtLaunchFileserveDlg(QWidget* parent);
  ~ezQtLaunchFileserveDlg();

  ezString m_sFileserveCmdLine;

private Q_SLOTS:
  void on_ButtonLaunch_clicked();

private:
  virtual void showEvent(QShowEvent* event) override;
};
