#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>
#include <Inspector/ui_MainWidget.h>
#include <QMainWindow>
#include <ads/DockManager.h>

class QTreeWidgetItem;

class ezQtMainWidget : public ads::CDockWidget, public Ui_MainWidget
{
  Q_OBJECT
public:
  static ezQtMainWidget* s_pWidget;

  ezQtMainWidget(ads::CDockManager* pDockManager, QWidget* pParent = nullptr);
  ~ezQtMainWidget();

  void ResetStats();
  void UpdateStats();
  virtual void closeEvent(QCloseEvent* pEvent) override;

  static void ProcessTelemetry(void* pUnuseed);

public Q_SLOTS:
  void ShowStatIn(bool);

private Q_SLOTS:
  void on_ButtonConnect_clicked();

  void on_TreeStats_itemChanged(QTreeWidgetItem* item, int column);
  void on_TreeStats_customContextMenuRequested(const QPoint& p);

private:
  void SaveFavorites();
  void LoadFavorites();

  QTreeWidgetItem* CreateStat(ezStringView sPath, bool bParent);
  void SetFavorite(const ezString& sStat, bool bFavorite);

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
    QTreeWidgetItem* m_pItemFavorite;

    StatData()
    {
      m_pItem = nullptr;
      m_pItemFavorite = nullptr;
    }
  };

  friend class ezQtStatVisWidget;
  ezMap<ezString, StatData> m_Stats;
  ezSet<ezString> m_Favorites;
};
