#pragma once

#include <Foundation/Basics.h>
#include <QDockWidget>
#include <Tools/Inspector/ui_CVarsWidget.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Configuration/CVar.h>
#include <GuiFoundation/Widgets/CVarWidget.moc.h>

class ezQtCVarsWidget : public QDockWidget, public Ui_CVarsWidget
{
public:
  Q_OBJECT

public:
  ezQtCVarsWidget(QWidget* parent = 0);

  static ezQtCVarsWidget* s_pWidget;

private slots:
  void BoolChanged(const char* szCVar, bool newValue);
  void FloatChanged(const char* szCVar, float newValue);
  void IntChanged(const char* szCVar, int newValue);
  void StringChanged(const char* szCVar, const char* newValue);

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();

private:
  //void UpdateCVarsTable(bool bRecreate);
  

  void SendCVarUpdateToServer(const char* szName, const ezCVarWidgetData& cvd);
  void SyncAllCVarsToServer();

  ezMap<ezString, ezCVarWidgetData> m_CVars;
  ezMap<ezString, ezCVarWidgetData> m_CVarsBackup;

};


