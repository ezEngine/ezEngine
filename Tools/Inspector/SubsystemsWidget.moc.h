#pragma once

#include <Foundation/Basics.h>
#include <QDockWidget>
#include <Tools/Inspector/ui_SubsystemsWidget.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Configuration/Startup.h>

class ezSubsystemsWidget : public QDockWidget, public Ui_SubsystemsWidget
{
public:
  Q_OBJECT

public:
  ezSubsystemsWidget(QWidget* parent = 0);

  static ezSubsystemsWidget* s_pWidget;

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


