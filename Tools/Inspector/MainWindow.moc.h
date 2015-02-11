#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Strings/String.h>
#include <QMainWindow>
#include <Tools/Inspector/ui_MainWindow.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Time/Time.h>
#include <qtreewidget.h>
#include <Inspector/StatVisWidget.moc.h>

class ezMainWindow : public QMainWindow, public Ui_MainWindow
{
  enum OnTopMode
  {
    Never,
    Always,
    WhenConnected
  };

public:
  Q_OBJECT

public:
  ezMainWindow();
  ~ezMainWindow();

  static ezMainWindow* s_pWidget;

  static void ProcessTelemetry(void* pUnuseed);

  virtual void closeEvent(QCloseEvent* event);

public slots:
  void DockWidgetVisibilityChanged(bool bVisible);
  void ShowStatIn();
  void UpdateNetworkTimeOut();

private slots:
  void on_ActionShowWindowLog_triggered();
  void on_ActionShowWindowMemory_triggered();
  void on_ActionShowWindowTime_triggered();
  void on_ActionShowWindowInput_triggered();
  void on_ActionShowWindowCVar_triggered();
  void on_ActionShowWindowReflection_triggered();
  void on_ActionShowWindowSubsystems_triggered();
  void on_ActionShowWindowPlugins_triggered();
  void on_ActionShowWindowFile_triggered();
  void on_ActionShowWindowGlobalEvents_triggered();
  void on_ActionShowWindowData_triggered();
  void on_ActionShowWindowResource_triggered();
  void on_ButtonConnect_clicked();

  void on_ActionOnTopWhenConnected_triggered();
  void on_ActionAlwaysOnTop_triggered();
  void on_ActionNeverOnTop_triggered();

  void on_TreeStats_itemChanged(QTreeWidgetItem* item, int column);
  void on_TreeStats_customContextMenuRequested(const QPoint& p);

private:
  void SetAlwaysOnTop(OnTopMode Mode);
  void UpdateAlwaysOnTop();
  void SetupNetworkTimer();
  void UpdateNetwork();

  void ResetStats();
  void UpdateStats();

  void SaveFavourites();
  void LoadFavourites();

  QTreeWidgetItem* CreateStat(const char* szPath, bool bParent);
  void SetFavourite(const ezString& sStat, bool bFavourite);

  ezUInt32 m_uiMaxStatSamples;
  ezTime m_MaxGlobalTime;

  struct StatSample
  {
    ezTime m_AtGlobalTime;
    double m_Value;
  };

  struct StatData
  {
    ezDeque<StatSample> m_History;

    ezString m_sValue;
    QTreeWidgetItem* m_pItem;
    QTreeWidgetItem* m_pItemFavourite;

    StatData()
    {
      m_pItem = nullptr;
      m_pItemFavourite = nullptr;
    }
  };

  friend class ezStatVisWidget;

  ezMap<ezString, StatData> m_Stats;
  ezSet<ezString> m_Favourites;
  QTimer* m_pNetworkTimer;
  OnTopMode m_OnTopMode;

  ezStatVisWidget* m_pStatHistoryWidgets[10];

  QAction* m_pActionShowStatIn[10];
  
};


