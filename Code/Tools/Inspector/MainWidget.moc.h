#pragma once

#include <ads/DockManager.h>
#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>
#include <Tools/Inspector/ui_MainWidget.h>
#include <QMainWindow>

class QTreeWidgetItem;

class ezQtMainWidget : public ads::CDockWidget, public Ui_MainWidget
{
  Q_OBJECT
public:

  static ezQtMainWidget* s_pWidget;

  ezQtMainWidget(QWidget* parent = nullptr);
  ~ezQtMainWidget();

  void ResetStats();
  void UpdateStats();
  virtual void closeEvent(QCloseEvent* event) override;

  static void ProcessTelemetry(void* pUnuseed);

public Q_SLOTS:
  void ShowStatIn(bool);

private Q_SLOTS:
  void on_ButtonConnect_clicked();

  void on_TreeStats_itemChanged(QTreeWidgetItem* item, int column);
  void on_TreeStats_customContextMenuRequested(const QPoint& p);

private:

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

    ezVariant m_Value;
    QTreeWidgetItem* m_pItem;
    QTreeWidgetItem* m_pItemFavourite;

    StatData()
    {
      m_pItem = nullptr;
      m_pItemFavourite = nullptr;
    }
  };

  friend class ezQtStatVisWidget;
  ezMap<ezString, StatData> m_Stats;
  ezSet<ezString> m_Favourites;
};
