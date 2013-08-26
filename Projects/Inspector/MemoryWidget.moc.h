#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <QDockWidget>
#include <Projects/Inspector/ui_MemoryWidget.h>
#include <QGraphicsView>
#include <QListWidgetItem>

class ezMemoryWidget : public QDockWidget, public Ui_MemoryWidget
{
public:
  Q_OBJECT

public:
  static const ezUInt8 s_uiMaxColors = 9;

  ezMemoryWidget(QWidget* parent = 0);

  static ezMemoryWidget* s_pWidget;

private slots:

  void on_ListAllocators_itemChanged(QListWidgetItem* item);

public:
  static void ProcessTelemetry_Memory(void* pPassThrough);

  void ResetStats();
  void UpdateStats();

private:

  QGraphicsPathItem* m_pPath[s_uiMaxColors];
  QGraphicsScene m_Scene;

  ezTime m_LastUsedMemoryStored;

  ezUInt32 m_uiMaxSamples;

  ezInt8 m_uiDropOne;
  ezUInt8 m_uiColorsUsed;
  bool m_bAllocatorsChanged;

  struct AllocatorData
  {
    ezDeque<ezUInt64> m_UsedMemory;
  
    bool m_bDisplay;
    ezInt8 m_iColor;
    ezUInt64 m_uiAllocs;
    ezUInt64 m_uiDeallocs;
    ezUInt64 m_uiLiveAllocs;
    ezUInt64 m_uiMaxUsedMemoryRecently;
    ezUInt64 m_uiMinUsedMemory;
    ezUInt64 m_uiMaxUsedMemory;

    AllocatorData()
    {
      m_bDisplay = true;
      m_iColor = -1;
      m_uiAllocs = 0;
      m_uiDeallocs = 0;
      m_uiLiveAllocs = 0;
      m_uiMaxUsedMemoryRecently = 0;
      m_uiMinUsedMemory = 0xFFFFFFFF;
      m_uiMaxUsedMemory = 0;
    }
  };

  ezMap<ezString, AllocatorData> m_AllocatorData;
};


