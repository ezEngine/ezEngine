#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/Widgets/CVarWidget.moc.h>
#include <Inspector/ui_CVarsWidget.h>
#include <ads/DockWidget.h>

class ezQtCVarsWidget : public ads::CDockWidget, public Ui_CVarsWidget
{
public:
  Q_OBJECT

public:
  ezQtCVarsWidget(QWidget* pParent = 0);

  static ezQtCVarsWidget* s_pWidget;

private Q_SLOTS:
  void BoolChanged(const char* szCVar, bool newValue);
  void FloatChanged(const char* szCVar, float newValue);
  void IntChanged(const char* szCVar, int newValue);
  void StringChanged(const char* szCVar, const char* newValue);

public:
  static void ProcessTelemetry(void* pUnuseed);
  static void ProcessTelemetryConsole(void* pUnuseed);

  void ResetStats();

private:
  // void UpdateCVarsTable(bool bRecreate);


  void SendCVarUpdateToServer(const char* szName, const ezCVarWidgetData& cvd);
  void SyncAllCVarsToServer();

  ezMap<ezString, ezCVarWidgetData> m_CVars;
  ezMap<ezString, ezCVarWidgetData> m_CVarsBackup;
};

