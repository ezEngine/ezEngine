#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/String.h>
#include <Inspector/ui_LogDockWidget.h>
#include <ads/DockWidget.h>

class ezQtLogDockWidget : public ads::CDockWidget, public Ui_LogDockWidget
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
