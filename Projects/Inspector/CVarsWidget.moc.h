#pragma once

#include <Foundation/Basics.h>
#include <QDockWidget>
#include <Projects/Inspector/ui_CVarsWidget.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Configuration/CVar.h>

class ezCVarsWidget : public QDockWidget, public Ui_CVarsWidget
{
public:
  Q_OBJECT

public:
  ezCVarsWidget(QWidget* parent = 0);

  static ezCVarsWidget* s_pWidget;

private slots:
  virtual void BoolChanged(int index);
  virtual void FloatChanged(double d);
  virtual void IntChanged(int i);
  virtual void StringChanged(const QString& val);

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();
  void UpdateStats();

private:
  void UpdateCVarsTable(bool bRecreate);

  struct CVarData
  {
    ezInt32 m_iTableRow;

    ezString m_sPlugin;
    ezUInt8 m_uiFlags;
    ezUInt8 m_uiType;

    bool m_bValue;
    float m_fValue;
    ezString m_sValue;
    ezInt32 m_iValue;

    CVarData()
    {
      m_iTableRow = -1;

      m_uiFlags = 0;
      m_uiType = 0;

      m_bValue = false;
      m_fValue = 0;
      m_iValue = 0;
    }
  };

  ezMap<ezString, CVarData> m_CVars;

};


