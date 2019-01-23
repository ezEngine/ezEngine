#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <QDockWidget>
#include <Tools/Inspector/ui_MemoryWidget.h>
#include <QGraphicsView>

class QTreeWidgetItem;

class ezQtMemoryWidget : public QDockWidget, public Ui_MemoryWidget
{
public:
  Q_OBJECT

public:
  static const ezUInt8 s_uiMaxColors = 9;

  ezQtMemoryWidget(QWidget* parent = 0);

  static ezQtMemoryWidget* s_pWidget;

private Q_SLOTS:

  void on_ListAllocators_itemChanged(QTreeWidgetItem* item);
  void on_ComboTimeframe_currentIndexChanged(int index);

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();
  void UpdateStats();

private:

  QGraphicsPathItem* m_pPath[s_uiMaxColors];
  QGraphicsPathItem* m_pPathMax;
  QGraphicsScene m_Scene;

  ezTime m_LastUsedMemoryStored;
  ezTime m_LastUpdatedAllocatorList;

  ezUInt32 m_uiMaxSamples;
  ezUInt32 m_uiDisplaySamples;

  ezUInt8 m_uiColorsUsed;
  bool m_bAllocatorsChanged;

  struct AllocatorData
  {
    ezDeque<ezUInt64> m_UsedMemory;
    ezString m_sName;

    bool m_bReceivedData;
    bool m_bDisplay;
    ezInt8 m_iColor;
    ezUInt32 m_uiParentId;
    ezUInt64 m_uiAllocs;
    ezUInt64 m_uiDeallocs;
    ezUInt64 m_uiLiveAllocs;
    ezUInt64 m_uiMaxUsedMemoryRecently;
    ezUInt64 m_uiMaxUsedMemory;
    QTreeWidgetItem* m_pTreeItem;

    AllocatorData()
    {
      m_bReceivedData = false;
      m_bDisplay = true;
      m_iColor = -1;
      m_uiParentId = ezInvalidIndex;
      m_uiAllocs = 0;
      m_uiDeallocs = 0;
      m_uiLiveAllocs = 0;
      m_uiMaxUsedMemoryRecently = 0;
      m_uiMaxUsedMemory = 0;
      m_pTreeItem = nullptr;
    }
  };

  AllocatorData m_Accu;

  ezMap<ezUInt32, AllocatorData> m_AllocatorData;
};


