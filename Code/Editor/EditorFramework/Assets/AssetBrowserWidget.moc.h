#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetBrowserWidget.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>
#include <ToolsFoundation/Project/ToolsProject.h>



class ezQtToolBarActionMapView;
class ezQtAssetBrowserFilter;
class ezQtAssetBrowserModel;
struct ezAssetCuratorEvent;
class ezQtAssetBrowserModel;

class ezQtAssetBrowserWidget : public QWidget, public Ui_AssetBrowserWidget
{
  Q_OBJECT
public:
  ezQtAssetBrowserWidget(QWidget* pParent);
  ~ezQtAssetBrowserWidget();

  enum class Mode
  {
    Browser,
    AssetPicker,
    FilePicker,
  };

  void SetMode(Mode mode);
  void SetSelectedAsset(ezUuid preselectedAsset);
  void SetSelectedFile(ezStringView sAbsPath);
  void ShowOnlyTheseTypeFilters(ezStringView sFilters);
  void UseFileExtensionFilters(ezStringView sFileExtensions);
  void SetRequiredTag(ezStringView sRequiredTag);

  void SaveState(const char* szSettingsName);
  void RestoreState(const char* szSettingsName);

  void dragEnterEvent(QDragEnterEvent* pEvent) override;
  void dragMoveEvent(QDragMoveEvent* pEvent) override;
  void dragLeaveEvent(QDragLeaveEvent* pEvent) override;
  void dropEvent(QDropEvent* pEvent) override;

  ezQtAssetBrowserModel* GetAssetBrowserModel() { return m_pModel; }
  const ezQtAssetBrowserModel* GetAssetBrowserModel() const { return m_pModel; }
  ezQtAssetBrowserFilter* GetAssetBrowserFilter() { return m_pFilter; }
  const ezQtAssetBrowserFilter* GetAssetBrowserFilter() const { return m_pFilter; }

Q_SIGNALS:
  void ItemChosen(ezUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, ezUInt8 uiAssetBrowserItemFlags);
  void ItemSelected(ezUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, ezUInt8 uiAssetBrowserItemFlags);
  void ItemCleared();

private Q_SLOTS:
  void OnTextFilterChanged();
  void OnTypeFilterChanged();
  void OnPathFilterChanged();
  void OnFilterChanged();
  void on_ListAssets_doubleClicked(const QModelIndex& index);
  void on_ListAssets_activated(const QModelIndex& index);
  void on_ListAssets_clicked(const QModelIndex& index);
  void on_ButtonListMode_clicked();
  void on_ButtonIconMode_clicked();
  void on_IconSizeSlider_valueChanged(int iValue);
  void on_ListAssets_ViewZoomed(ezInt32 iIconSizePercentage);
  void on_ResetTypeFilter_clicked();
  void OnSearchWidgetTextChanged(const QString& text);
  void on_TreeFolderFilter_customContextMenuRequested(const QPoint& pt);
  void on_TypeFilter_currentIndexChanged(int index);
  void OnScrollToItem(ezUuid preselectedAsset);
  void OnScrollToFile(QString sPreselectedFile);
  void OnShowSubFolderItemsToggled();
  void OnShowHiddenFolderItemsToggled();
  void OnResaveAssets();
  void on_ListAssets_customContextMenuRequested(const QPoint& pt);
  void OnListOpenExplorer();
  void OnListOpenAssetDocument();
  void OnListOpenFileWith();
  void OnTransform();
  void OnListToggleSortByRecentlyUsed();
  void OnListCopyAssetGuid();
  void OnFilterToThisPath();
  void OnListFindAllReferences(bool transitive);
  void OnSelectionTimer();
  void OnAssetSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
  void OnAssetSelectionCurrentChanged(const QModelIndex& current, const QModelIndex& previous);
  void OnModelReset();
  void NewAsset();
  void OnFileEditingFinished(const QString& sAbsPath, const QString& sNewName, bool bIsAsset);
  // void ImportSelection();
  void OnOpenImportReferenceAsset();
  void RenameCurrent();
  void DeleteSelection();
  void OnImportAsAboutToShow();
  void OnImportAsClicked();


private:
  virtual void keyPressEvent(QKeyEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;

private:
  void AssetCuratorEventHandler(const ezAssetCuratorEvent& e);
  void UpdateAssetTypes();
  void ProjectEventHandler(const ezToolsProjectEvent& e);
  void AddAssetCreatorMenu(QMenu* pMenu, bool useSelectedAsset);
  void AddImportedViaMenu(QMenu* pMenu);
  void GetSelectedImportableFiles(ezDynamicArray<ezString>& out_Files) const;

  Mode m_Mode = Mode::Browser;
  ezQtToolBarActionMapView* m_pToolbar = nullptr;
  ezString m_sAllTypesFilter;
  ezQtAssetBrowserModel* m_pModel = nullptr;
  ezQtAssetBrowserFilter* m_pFilter = nullptr;

  /// \brief After creating a new asset and renaming it, we want to open it as well.
  bool m_bOpenAfterRename = false;
};
