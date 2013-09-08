#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Strings/String.h>
#include <QMainWindow>
#include <Projects/Inspector/ui_mainwindow.h>

class ezMainWindow : public QMainWindow, public Ui_MainWindow
{
public:
  Q_OBJECT

public:
  ezMainWindow();

  static ezMainWindow* s_pWidget;

  void paintEvent(QPaintEvent* event) EZ_OVERRIDE;

  void SaveLayout() const;
  void LoadLayout();

  void Log(const char* szMsg);

public slots:
  void DockWidgetVisibilityChanged(bool bVisible);
  void on_ActionShowWindowLog_triggered();
  void on_ActionShowWindowConfig_triggered();
  void on_ActionShowWindowMemory_triggered();
  void on_ActionShowWindowInput_triggered();
  void on_ActionShowWindowCVar_triggered();
  void on_ActionShowWindowStats_triggered();

private:

  struct LogMessage
  {
    ezString m_sMsg;
  };

  ezDeque<LogMessage> m_LogList;
};


