#pragma once

#include <EditorFramework/Plugin.h>
#include <Tools/EditorFramework/ui_AssetBrowserWidget.h>
#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class ezToolBarActionMapView;

class ezAssetBrowserWidget : public QWidget, public Ui_AssetBrowserWidget
{
  Q_OBJECT

public:
  ezAssetBrowserWidget(QWidget* parent);
  ~ezAssetBrowserWidget();

  void SetDialogMode();
  void SetSelectedAsset(const char* szAssetPath);
  void ShowOnlyTheseTypeFilters(const char* szFilters);

  void SaveState(const char* szSettingsName);
  void RestoreState(const char* szSettingsName);

  ezAssetBrowserModel* GetAssetBrowserModel() { return m_pModel; }
  const ezAssetBrowserModel* GetAssetBrowserModel() const { return m_pModel; }

signals:
  void ItemChosen(QString sAssetGUID, QString sAssetPathRelative, QString sAssetPathAbsolute);
  void ItemSelected(QString sAssetGUID, QString sAssetPathRelative, QString sAssetPathAbsolute);

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
  void on_LineSearchFilter_textEdited(const QString& text);
  void on_ButtonClearSearch_clicked();
  void on_ListTypeFilter_itemChanged(QListWidgetItem* item);
  void on_TreeFolderFilter_itemSelectionChanged();
  void on_TreeFolderFilter_customContextMenuRequested(const QPoint& pt);
  void OnScrollToItem(QString sPath);
  void OnTreeOpenExplorer();
  void OnShowSubFolderItemsToggled();
  void on_ListAssets_customContextMenuRequested(const QPoint& pt);
  void OnListOpenExplorer();
  void OnListOpenAssetDocument();
  void OnListToggleSortByRecentlyUsed();

private:
  void AssetCuratorEventHandler(const ezAssetCurator::Event& e);
  void UpdateDirectoryTree();
  void BuildDirectoryTree(const char* szCurPath, QTreeWidgetItem* pParent, const char* szCurPathToItem);
  bool SelectPathFilter(QTreeWidgetItem* pParent, const QString& sPath);
  void UpdateAssetTypes();
  void ProjectEventHandler(const ezToolsProject::Event& e);

  bool m_bDialogMode;
  ezUInt32 m_uiKnownAssetFolderCount;

  ezToolBarActionMapView* m_pToolbar;
  ezString m_sAllTypesFilter;
  ezAssetBrowserModel* m_pModel;
};