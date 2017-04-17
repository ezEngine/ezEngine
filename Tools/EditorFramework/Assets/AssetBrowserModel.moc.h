#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Types/Uuid.h>
#include <QAbstractItemModel>

struct ezAssetInfo;
struct ezAssetCuratorEvent;
struct ezSubAsset;

class EZ_EDITORFRAMEWORK_DLL ezQtAssetFilter : public QObject
{
  Q_OBJECT
public:
  explicit ezQtAssetFilter(QObject* pParent);
  virtual bool IsAssetFiltered(const ezSubAsset* pInfo) const = 0;
  virtual bool Less(const ezSubAsset* pInfoA, const ezSubAsset* pInfoB) const = 0;

signals:
  void FilterChanged();
};

class EZ_EDITORFRAMEWORK_DLL ezQtAssetBrowserModel : public QAbstractItemModel
{
  Q_OBJECT
public:

  enum UserRoles
  {
    SubAssetGuid = Qt::UserRole + 0,
    AssetGuid,
    AbsolutePath,
    RelativePath,
    AssetIconPath,
    TransformState,
  };

  ezQtAssetBrowserModel(QObject* pParent, ezQtAssetFilter* pFilter);
  ~ezQtAssetBrowserModel();

  void resetModel();

  void SetIconMode(bool bIconMode) { m_bIconMode = bIconMode; }
  bool GetIconMode() { return m_bIconMode; }

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
    ezUuid m_Guid;
    mutable ezUInt32 m_uiThumbnailID;
  };

  enum class AssetOp
  {
    Add,
    Remove,
    Updated,
  };
  void AssetCuratorEventHandler(const ezAssetCuratorEvent& e);
  ezInt32 FindAssetIndex(const ezUuid& assetGuid) const;
  void HandleAsset(const ezSubAsset* pInfo, AssetOp op);
  void Init(AssetEntry& ae, const ezSubAsset* pInfo);

  ezQtAssetFilter* m_pFilter;
  ezDynamicArray<AssetEntry> m_AssetsToDisplay;
  ezSet<ezUuid> m_DisplayedEntries;

  bool m_bIconMode;
};
