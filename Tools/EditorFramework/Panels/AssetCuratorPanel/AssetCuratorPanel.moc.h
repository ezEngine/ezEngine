#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/Plugin.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <Tools/EditorFramework/ui_AssetCuratorPanel.h>
#include <EditorFramework/Assets/AssetBrowserModel.moc.h>

class ezQtCuratorControl;
struct ezLoggingEventData;

class EZ_EDITORFRAMEWORK_DLL ezQtAssetCuratorFilter : public ezQtAssetFilter
{
  Q_OBJECT
public:
  explicit ezQtAssetCuratorFilter(QObject* pParent);

public:
  virtual bool IsAssetFiltered(const ezAssetInfo* pInfo) const override;
  virtual bool Less(ezAssetInfo* pInfoA, ezAssetInfo* pInfoB) const override;
};

class EZ_EDITORFRAMEWORK_DLL ezQtAssetCuratorPanel : public ezQtApplicationPanel, public Ui_AssetCuratorPanel
{
  Q_OBJECT

  EZ_DECLARE_SINGLETON(ezQtAssetCuratorPanel);

public:
  ezQtAssetCuratorPanel();
  ~ezQtAssetCuratorPanel();

public slots:
  void OnAssetSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

private:
  void LogWriter(const ezLoggingEventData& e);
  void UpdateIssueInfo();

  ezQtAssetBrowserModel* m_pModel;
  ezQtAssetCuratorFilter* m_pFilter;
  QPersistentModelIndex m_selectedIndex;
};
