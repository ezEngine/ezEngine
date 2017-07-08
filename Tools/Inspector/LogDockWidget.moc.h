#pragma once

#include <Foundation/Basics.h>
#include <QDockWidget>
#include <Tools/Inspector/ui_LogDockWidget.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Logging/Log.h>

class ezQtLogDockWidget : public QDockWidget, public Ui_LogDockWidget
{
public:
  Q_OBJECT

public:
  ezQtLogDockWidget(QWidget* parent = 0);

  void Log(const ezFormatString& sText);

  static ezQtLogDockWidget* s_pWidget;

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();
};


