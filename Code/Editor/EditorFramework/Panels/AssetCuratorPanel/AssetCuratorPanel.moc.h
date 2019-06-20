#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <Editor/EditorFramework/ui_AssetCuratorPanel.h>
#include <EditorFramework/Assets/AssetBrowserModel.moc.h>

class ezQtCuratorControl;
struct ezLoggingEventData;

class EZ_EDITORFRAMEWORK_DLL ezQtAssetCuratorFilter : public ezQtAssetFilter
{
  Q_OBJECT
public:
  explicit ezQtAssetCuratorFilter(QObject* pParent);

public:
  virtual bool IsAssetFiltered(const ezSubAsset* pInfo) const override;
  virtual bool Less(const ezSubAsset* pInfoA, const ezSubAsset* pInfoB) const override;
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
  void on_ListAssets_doubleClicked(const QModelIndex& index);

private:
  void LogWriter(const ezLoggingEventData& e);
  void UpdateIssueInfo();

  ezQtAssetBrowserModel* m_pModel;
  ezQtAssetCuratorFilter* m_pFilter;
  QPersistentModelIndex m_selectedIndex;
};
