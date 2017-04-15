#pragma once

#include <EditorFramework/Plugin.h>
#include <Tools/EditorFramework/ui_AssetBrowserWidget.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class ezQtToolBarActionMapView;
class ezQtAssetBrowserFilter;
class ezQtAssetBrowserModel;
struct ezAssetCuratorEvent;

class ezQtAssetBrowserWidget : public QWidget, public Ui_AssetBrowserWidget
{
  Q_OBJECT
public:
  ezQtAssetBrowserWidget(QWidget* parent);
  ~ezQtAssetBrowserWidget();

  void SetDialogMode();
  void SetSelectedAsset(const char* szAssetPath);
  void ShowOnlyTheseTypeFilters(const char* szFilters);

  void SaveState(const char* szSettingsName);
  void RestoreState(const char* szSettingsName);

  ezQtAssetBrowserModel* GetAssetBrowserModel() { return m_pModel; }
  const ezQtAssetBrowserModel* GetAssetBrowserModel() const { return m_pModel; }
  ezQtAssetBrowserFilter* GetAssetBrowserFilter() { return m_pFilter; }
  const ezQtAssetBrowserFilter* GetAssetBrowserFilter() const { return m_pFilter; }

signals:
  void ItemChosen(ezUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute);
  void ItemSelected(ezUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute);
  void ItemCleared();

private slots:
  void OnTextFilterChanged();
  void OnTypeFilterChanged();
  void OnPathFilterChanged();
  void on_ListAssets_doubleClicked(const QModelIndex& index);
  void on_ListAssets_activated(const QModelIndex & index);
  void on_ListAssets_clicked(const QModelIndex & index);
  void on_ButtonListMode_clicked();
  void on_ButtonIconMode_clicked();
  void on_IconSizeSlider_valueChanged(int iValue);
  void on_ListAssets_ViewZoomed(ezInt32 iIconSizePercentage);
  void OnSearchWidgetTextChanged(const QString& text);
  void on_ListTypeFilter_itemChanged(QListWidgetItem* item);
  void on_TreeFolderFilter_itemSelectionChanged();
  void on_TreeFolderFilter_customContextMenuRequested(const QPoint& pt);
  void OnScrollToItem(QString sPath);
  void OnTreeOpenExplorer();
  void OnShowSubFolderItemsToggled();
  void on_ListAssets_customContextMenuRequested(const QPoint& pt);
  void OnListOpenExplorer();
  void OnListOpenAssetDocument();
  void OnTransform();
  void OnListToggleSortByRecentlyUsed();
  void OnListCopyAssetGuid();
  void OnSelectionTimer();
  void OnAssetSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
  void OnAssetSelectionCurrentChanged(const QModelIndex &current, const QModelIndex &previous);
  void OnModelReset();
  void OnNewAsset();

private:
  void AssetCuratorEventHandler(const ezAssetCuratorEvent& e);
  void UpdateDirectoryTree();
  void ClearDirectoryTree();
  void BuildDirectoryTree(const char* szCurPath, QTreeWidgetItem* pParent, const char* szCurPathToItem);
  bool SelectPathFilter(QTreeWidgetItem* pParent, const QString& sPath);
  void UpdateAssetTypes();
  void ProjectEventHandler(const ezToolsProjectEvent& e);
  void AddAssetCreatorMenu(QMenu* pMenu, bool useSelectedAsset);

  bool m_bDialogMode;
  ezUInt32 m_uiKnownAssetFolderCount;

  ezQtToolBarActionMapView* m_pToolbar;
  ezString m_sAllTypesFilter;
  ezQtAssetBrowserModel* m_pModel;
  ezQtAssetBrowserFilter* m_pFilter;
};
