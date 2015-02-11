#pragma once

#include <Foundation/Basics.h>
#include <QDockWidget>
#include <Tools/Inspector/ui_ResourceWidget.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <Core/ResourceManager/Resource.h>

class ezResourceWidget : public QDockWidget, public Ui_ResourceWidget
{
public:
  Q_OBJECT

public:
  ezResourceWidget(QWidget* parent = 0);

  static ezResourceWidget* s_pWidget;

private slots:

  //virtual void on_SpinLimitToRecent_valueChanged(int val);
  //virtual void on_SpinMinDuration_valueChanged(double val);
  //virtual void on_LineFilterByName_textChanged();
  //virtual void on_ComboThread_currentIndexChanged(int state);

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();
  void UpdateStats();

  void UpdateTable();

private:
  struct ResourceData
  {
    ResourceData()
    {
      m_pMainItem = nullptr;
      m_bUpdate = true;
    }

    bool m_bUpdate;
    QTableWidgetItem* m_pMainItem;
    ezString m_sResourceID;
    ezString m_sResourceType;
    ezResourcePriority m_Priority;
    ezBitflags<ezResourceFlags> m_Flags;
    ezResourceLoadDesc m_LoadingState;
    ezResourceBase::MemoryUsage m_Memory;
  };

  ezTime m_LastTableUpdate;
  bool m_bUpdateTable;

  ezHashTable<ezUInt32, ResourceData> m_Resources;
};


