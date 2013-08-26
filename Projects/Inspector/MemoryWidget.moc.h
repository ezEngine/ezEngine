#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Containers/Deque.h>
#include <QDockWidget>
#include <Projects/Inspector/ui_MemoryWidget.h>
#include <QGraphicsView>
//#include <QGraphicsPathItem>

class ezMemoryWidget : public QDockWidget, public Ui_MemoryWidget
{
public:
  Q_OBJECT

public:
  ezMemoryWidget(QWidget* parent = 0);

  static ezMemoryWidget* s_pWidget;

private slots:

public:
  static void ProcessTelemetry_Memory(void* pPassThrough);

  void ResetStats();
  void UpdateStats();

private:
  QGraphicsPathItem* m_pPath;
  QGraphicsScene m_Scene;

  ezTime m_LastUsedMemoryStored;
  ezDeque<ezUInt64> m_UsedMemory;

  ezInt8 m_uiDropOne;
  ezUInt32 m_uiMaxSamples;
  ezUInt64 m_uiMinUsedMemory;
  ezUInt64 m_uiMaxUsedMemory;
  ezUInt64 m_uiMaxUsedMemoryRecently;

  ezUInt64 m_uiAllocs;
  ezUInt64 m_uiDeallocs;
  ezUInt64 m_uiLiveAllocs;
};


