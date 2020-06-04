#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <EditorFramework/ui_AssetCuratorPanel.h>
#include <EditorFramework/Assets/AssetBrowserModel.moc.h>

class ezQtCuratorControl;
struct ezLoggingEventData;

class EZ_EDITORFRAMEWORK_DLL ezQtAssetCuratorFilter : public ezQtAssetFilter
{
  Q_OBJECT
public:
  explicit ezQtAssetCuratorFilter(QObject* pParent);

  void SetFilterTransitive(bool bFilterTransitive);

public:
  virtual bool IsAssetFiltered(const ezSubAsset* pInfo) const override;
  virtual bool Less(const ezSubAsset* pInfoA, const ezSubAsset* pInfoB) const override;

  bool m_bFilterTransitive = true;
};

class EZ_EDITORFRAMEWORK_DLL ezQtAssetCuratorPanel : public ezQtApplicationPanel, public Ui_AssetCuratorPanel
{
  Q_OBJECT

  EZ_DECLARE_SINGLETON(ezQtAssetCuratorPanel);

public:
  ezQtAssetCuratorPanel();
  ~ezQtAssetCuratorPanel();

public Q_SLOTS:
  void OnAssetSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

private Q_SLOTS:
  // note, because of the way we set up the widget, auto-connect doesn't work
  void onListAssetsDoubleClicked(const QModelIndex& index);
  void onCheckIndirectToggled(bool checked);

private:
  void LogWriter(const ezLoggingEventData& e);
  void UpdateIssueInfo();

  ezQtAssetBrowserModel* m_pModel;
  ezQtAssetCuratorFilter* m_pFilter;
  QPersistentModelIndex m_selectedIndex;
};
