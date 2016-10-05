#pragma once

#include <Foundation/Basics.h>
#include <QDockWidget>
#include <Tools/Inspector/ui_PluginsWidget.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>

class ezQtPluginsWidget : public QDockWidget, public Ui_PluginsWidget
{
public:
  Q_OBJECT

public:
  ezQtPluginsWidget(QWidget* parent = 0);

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


