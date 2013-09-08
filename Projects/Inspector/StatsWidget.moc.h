#pragma once

#include <Foundation/Basics.h>
#include <QDockWidget>
#include <Projects/Inspector/ui_StatsWidget.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <qtreewidget.h>

class ezStatsWidget : public QDockWidget, public Ui_StatsWidget
{
public:
  Q_OBJECT

public:
  ezStatsWidget(QWidget* parent = 0);
  ~ezStatsWidget();

  static ezStatsWidget* s_pWidget;

private slots:
  virtual void on_TreeStats_itemChanged(QTreeWidgetItem* item, int column);

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();
  void UpdateStats();

private:
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
};


