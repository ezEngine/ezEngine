#pragma once

#include <Foundation/Basics.h>
#include <QDockWidget>
#include <Tools/Inspector/ui_GlobalEventsWidget.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Communication/GlobalEvent.h>

class ezGlobalEventsWidget : public QDockWidget, public Ui_GlobalEventsWidget
{
public:
  Q_OBJECT

public:
  ezGlobalEventsWidget(QWidget* parent = 0);

  static ezGlobalEventsWidget* s_pWidget;

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();

private:
  void UpdateTable(bool bRecreate);

  struct GlobalEventsData
  {
    ezInt32 m_iTableRow;
    ezUInt32 m_uiTimesFired;
    ezUInt16 m_uiNumHandlers;
    ezUInt16 m_uiNumHandlersOnce;

    GlobalEventsData()
    {
      m_iTableRow = -1;

      m_uiTimesFired = 0;
      m_uiNumHandlers = 0;
      m_uiNumHandlersOnce = 0;
    }
  };

  ezMap<ezString, GlobalEventsData> m_Events;

};


