#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Strings/String.h>
#include <QMainWindow>
#include <Projects/Inspector/ui_mainwindow.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <qtreewidget.h>

class ezMainWindow : public QMainWindow, public Ui_MainWindow
{
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
  void UpdateNetworkTimeOut();

private slots:
  void on_ActionShowWindowLog_triggered();
  void on_ActionShowWindowMemory_triggered();
  void on_ActionShowWindowInput_triggered();
  void on_ActionShowWindowCVar_triggered();
  void on_ActionShowWindowSubsystems_triggered();
  void on_ActionShowWindowPlugins_triggered();
  void on_ActionShowWindowFile_triggered();
  void on_ActionShowWindowGlobalEvents_triggered();
  void on_ButtonConnect_clicked();

  void on_TreeStats_itemChanged(QTreeWidgetItem* item, int column);

private:
  void SetupNetworkTimer();
  void UpdateNetwork();

  void ResetStats();
  void UpdateStats();

  void SaveFavourites();
  void LoadFavourites();

  QTreeWidgetItem* CreateStat(const char* szPath, bool bParent);
  void SetFavourite(const ezString& sStat, bool bFavourite);

  struct StatData
  {
    ezString m_sValue;
    QTreeWidgetItem* m_pItem;
    QTreeWidgetItem* m_pItemFavourite;

    StatData()
    {
      m_pItem = NULL;
      m_pItemFavourite = NULL;
    }
  };

  ezMap<ezString, StatData> m_Stats;
  ezSet<ezString> m_Favourites;
  QTimer* m_pNetworkTimer;
};


