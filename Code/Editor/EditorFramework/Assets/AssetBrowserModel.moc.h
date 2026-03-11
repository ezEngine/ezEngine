#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Types/Uuid.h>
#include <QAbstractItemModel>
#include <QFileIconProvider>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

struct ezAssetInfo;
struct ezAssetCuratorEvent;
struct ezSubAsset;
class ezQtAssetFilter;

/// \brief Interface class of the asset filter used to decide which items are shown in the asset browser.
class EZ_EDITORFRAMEWORK_DLL ezQtAssetFilter : public QObject
{
  Q_OBJECT
public:
  explicit ezQtAssetFilter(QObject* pParent);
  virtual bool IsAssetFiltered(ezStringView sDataDirParentRelativePath, bool bIsFolder, const ezSubAsset* pInfo) const = 0;
  virtual bool GetSortByRecentUse() const { return false; }

Q_SIGNALS:
  void FilterChanged();
};

/// \brief Each item in the asset browser can be multiple things at the same time as described by these flags.
/// Retrieved via user role ezQtAssetBrowserModel::UserRoles::ItemFlags.
struct EZ_EDITORFRAMEWORK_DLL ezAssetBrowserItemFlags
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Folder = EZ_BIT(0),        // Any folder inside a data directory
    DataDirectory = EZ_BIT(1), // mutually exclusive with Folder
    File = EZ_BIT(2),          // any file, could also be an Asset
    Asset = EZ_BIT(3),         // main asset: mutually exclusive with SubAsset
    SubAsset = EZ_BIT(4),      // sub-asset (imaginary, not a File or Asset)
    Default = 0
  };

  struct Bits
  {
    StorageType Folder : 1;
    StorageType DataDirectory : 1;
    StorageType File : 1;
    StorageType Asset : 1;
    StorageType SubAsset : 1;
  };
};
EZ_DECLARE_FLAGS_OPERATORS(ezAssetBrowserItemFlags);

/// \brief Model of the item view in the asset browser.
class EZ_EDITORFRAMEWORK_DLL ezQtAssetBrowserModel : public QAbstractItemModel, public QEnableSharedFromThis<ezQtAssetBrowserModel>
{
  Q_OBJECT
public:
  enum UserRoles
  {
    SubAssetGuid = Qt::UserRole + 0, // ezUuid
    AssetGuid,                       // ezUuid
    AbsolutePath,                    // QString
    RelativePath,                    // QString
    AssetIcon,                       // QIcon
    TransformState,                  // QString
    Importable,                      // bool
    ItemFlags,                       // ezAssetBrowserItemFlags as int
  };

  ezQtAssetBrowserModel(QObject* pParent, ezQtAssetFilter* pFilter);
  ~ezQtAssetBrowserModel();
  void Initialize();

  void resetModel();

  void SetIconMode(bool bIconMode) { m_bIconMode = bIconMode; }
  bool GetIconMode() { return m_bIconMode; }

  ezInt32 FindAssetIndex(const ezUuid& assetGuid) const;
  ezInt32 FindIndex(ezStringView sAbsPath) const;

public Q_SLOTS:
  void ThumbnailLoaded(QString sPath, QModelIndex index, QVariant userData1, QVariant userData2);
  void ThumbnailInvalidated(QString sPath, ezUInt32 uiImageID);
  void OnFileSystemUpdate();

signals:
  void editingFinished(const QString& sAbsPath, const QString& sNewName, bool bIsAsset) const;

public: // QAbstractItemModel interface
  virtual QVariant data(const QModelIndex& index, int iRole) const override;
  virtual bool setData(const QModelIndex& index, const QVariant& value, int iRole = Qt::EditRole) override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QVariant headerData(int iSection, Qt::Orientation orientation, int iRole = Qt::DisplayRole) const override;
  virtual QModelIndex index(int iRow, int iColumn, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual QStringList mimeTypes() const override;
  virtual QMimeData* mimeData(const QModelIndexList& indexes) const override;
  virtual Qt::DropActions supportedDropActions() const override;

private:
  friend struct FileComparer;

  enum class AssetOp
  {
    Add,
    Remove,
    Updated,
  };

  struct VisibleEntry
  {
    ezDataDirPath m_sAbsFilePath;
    ezUuid m_Guid;
    ezBitflags<ezAssetBrowserItemFlags> m_Flags;
    mutable ezUInt32 m_uiThumbnailID;
  };

  struct FsEvent
  {
    ezFileChangedEvent m_FileEvent;
    ezFolderChangedEvent m_FolderEvent;
  };

private:
  void AssetCuratorEventHandler(const ezAssetCuratorEvent& e);
  void HandleEntry(const VisibleEntry& entry, AssetOp op);
  void FileSystemFileEventHandler(const ezFileChangedEvent& e);
  void FileSystemFolderEventHandler(const ezFolderChangedEvent& e);
  void HandleFile(const ezFileChangedEvent& e);
  void HandleFolder(const ezFolderChangedEvent& e);

private:
  ezQtAssetFilter* m_pFilter = nullptr;
  bool m_bIconMode = true;
  ezSet<ezString> m_ImportExtensions;
  ezEventSubscriptionID m_FileChangedSubscription = 0;
  ezEventSubscriptionID m_FolderChangedSubscription = 0;

  ezMutex m_Mutex;
  ezDynamicArray<FsEvent> m_QueuedFileSystemEvents;

  ezDynamicArray<VisibleEntry> m_EntriesToDisplay;
  ezSet<ezUuid> m_DisplayedEntries;

  QFileIconProvider m_IconProvider;
};
