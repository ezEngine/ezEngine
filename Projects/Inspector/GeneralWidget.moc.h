#pragma once

#include <Foundation/Basics.h>
#include <QDockWidget>
#include <Projects/Inspector/ui_GeneralWidget.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Configuration/Startup.h>

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
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();
  void UpdateStats();

private:
  void UpdateSubSystems();

  struct SubsystemData
  {
    ezString m_sPlugin;
    bool m_bStartupDone[ezStartupStage::ENUM_COUNT];
    ezString m_sDependencies;
  };

  bool m_bUpdateSubsystems;
  ezMap<ezString, SubsystemData> m_Subsystems;
};


