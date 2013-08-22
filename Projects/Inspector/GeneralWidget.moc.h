#pragma once

#include <Foundation/Basics.h>
#include <QDockWidget>
#include <Projects/Inspector/ui_GeneralWidget.h>

class ezGeneralWidget : public QDockWidget, public Ui_GeneralWidget
{
public:
  Q_OBJECT

public:
  ezGeneralWidget(QWidget* parent = 0);

  static ezGeneralWidget* s_pWidget;

private slots:
  virtual void on_ButtonConnect_clicked();

public:
  static void ProcessTelemetry_General(void* pPassThrough);

  void UpdateStats();
};


