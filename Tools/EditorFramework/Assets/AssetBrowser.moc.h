#pragma once

#include <EditorFramework/Plugin.h>
#include <Tools/EditorFramework/ui_AssetBrowser.h>
#include <EditorFramework/Assets/AssetCuratorModel.moc.h>

class ezAssetBrowser : public QWidget, public Ui_AssetBrowser
{
  Q_OBJECT

public:
  ezAssetBrowser(QWidget* parent);
  ~ezAssetBrowser();

private slots:
  void OnTextFilterChanged();
  void OnTypeFilterChanged();
  void OnPathFilterChanged();
  void on_ListAssets_doubleClicked(const QModelIndex& index);
  void on_ButtonListMode_clicked();
  void on_ButtonIconMode_clicked();
  void on_IconSizeSlider_valueChanged(int iValue);
  void on_ListAssets_ViewZoomed(ezInt32 iIconSizePercentage);
  void on_LineSearchFilter_textEdited(const QString& text);
  void on_ButtonClearSearch_clicked();
  void on_ListTypeFilter_itemChanged(QListWidgetItem* item);
  void on_TreeFolderFilter_itemSelectionChanged();

private:
  void AssetCuratorEventHandler(const ezAssetCurator::Event& e);
  void UpdateDirectoryTree();
  void BuildDirectoryTree(const char* szCurPath, QTreeWidgetItem* pParent, const char* szCurPathToItem);
  bool SelectPathFilter(QTreeWidgetItem* pParent, const QString& sPath);

  ezUInt32 m_uiKnownAssetFolderCount;

  ezAssetCuratorModel* m_pModel;

};