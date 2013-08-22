#pragma once

#include <Foundation/Basics.h>
#include <QDockWidget>
#include <Projects/Inspector/ui_MemoryWidget.h>

class ezMemoryWidget : public QDockWidget, public Ui_MemoryWidget
{
public:
  Q_OBJECT

public:
  ezMemoryWidget(QWidget* parent = 0);

  static ezMemoryWidget* s_pWidget;

private slots:

public:
  static void ProcessTelemetry_Memory(void* pPassThrough);

  void UpdateStats();
};


