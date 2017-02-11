#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <QAbstractItemModel>

class EZ_EDITORFRAMEWORK_DLL ezQtAssetBrowserModel : public QAbstractItemModel
{
  Q_OBJECT

public:

  enum UserRoles
  {
    AssetGuid = Qt::UserRole + 0,
    AbsolutePath = Qt::UserRole + 1,
    RelativePath = Qt::UserRole + 2,
    AssetIconPath = Qt::UserRole + 3,
    TransformState = Qt::UserRole + 4,
  };

  ezQtAssetBrowserModel(QObject* pParent);
  ~ezQtAssetBrowserModel();
  
  void resetModel();


  void SetIconMode(bool bIconMode) { m_bIconMode = bIconMode; }
  bool GetIconMode() { return m_bIconMode; }

  void SetShowItemsInSubFolders(bool bShow);
  bool GetShowItemsInSubFolders() { return m_bShowItemsInSubFolders; }

  void SetSortByRecentUse(bool bSort);
  bool GetSortByRecentUse() { return m_bSortByRecentUse; }

  void SetTextFilter(const char* szText);
  const char* GetTextFilter() const { return m_sTextFilter; }

  void SetPathFilter(const char* szPath);
  const char* GetPathFilter() const { return m_sPathFilter; }

  void SetTypeFilter(const char* szTypes);
  const char* GetTypeFilter() const { return m_sTypeFilter; }

signals:
  void TextFilterChanged();
  void TypeFilterChanged();
  void PathFilterChanged();
  void ShowSubFolderItemsChanged();
  void SortByRecentUseChanged();

private slots:
  void ThumbnailLoaded(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2);
  void ThumbnailInvalidated(QString sPath, ezUInt32 uiImageID);

public: //QAbstractItemModel interface
  virtual QVariant data(const QModelIndex& index, int role) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual QStringList mimeTypes() const override;
  virtual QMimeData* mimeData(const QModelIndexList& indexes) const override;

private:
  friend struct AssetComparer;
  struct AssetEntry
  {
    ezString m_sSortingKey;
    ezUuid m_Guid;
    mutable ezUInt32 m_uiThumbnailID;
  };

  void AssetCuratorEventHandler(const ezAssetCuratorEvent& e);
  ezInt32 FindAssetIndex(const ezUuid& assetGuid) const;
  bool IsAssetFiltered(const ezAssetInfo* pInfo) const;
  void HandleAsset(const ezAssetInfo* pInfo, bool bAdd);
  void Init(AssetEntry& ae, const ezAssetInfo* pInfo);

  ezString m_sTextFilter, m_sTypeFilter, m_sPathFilter;
  ezDynamicArray<AssetEntry> m_AssetsToDisplay;

  bool m_bIconMode;
  bool m_bShowItemsInSubFolders;
  bool m_bSortByRecentUse;
};