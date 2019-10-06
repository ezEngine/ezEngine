#pragma once

#include <Foundation/Basics.h>
#include <Tools/Inspector/ui_SubsystemsWidget.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Configuration/Startup.h>
#include <ads/DockWidget.h>

class ezQtSubsystemsWidget : public ads::CDockWidget, public Ui_SubsystemsWidget
{
public:
  Q_OBJECT

public:
  ezQtSubsystemsWidget(QWidget* parent = 0);

  static ezQtSubsystemsWidget* s_pWidget;

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


