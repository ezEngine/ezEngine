#pragma once

#include <Foundation/Basics.h>
#include <QDockWidget>
#include <Projects/Inspector/ui_LogWidget.h>

class ezLogWidget : public QDockWidget, public Ui_LogWidget
{
public:
  Q_OBJECT

public:
  ezLogWidget(QWidget* parent = 0);

  static ezLogWidget* s_pWidget;

private slots:
  virtual void on_ButtonClearLog_clicked();

public:
  static void ProcessTelemetry_Log(void* pPassThrough);

  void UpdateStats();
};


