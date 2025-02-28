#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Inspector/ui_PluginsWidget.h>
#include <ads/DockWidget.h>

class ezQtPluginsWidget : public ads::CDockWidget, public Ui_PluginsWidget
{
public:
  Q_OBJECT

public:
  ezQtPluginsWidget(ads::CDockManager* pDockManager, QWidget* pParent = 0);

  static ezQtPluginsWidget* s_pWidget;

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();
  void UpdateStats();

private:
  void UpdatePlugins();

  struct PluginsData
  {
    bool m_bReloadable;
    ezString m_sDependencies;
  };

  bool m_bUpdatePlugins;
  ezMap<ezString, PluginsData> m_Plugins;
};
