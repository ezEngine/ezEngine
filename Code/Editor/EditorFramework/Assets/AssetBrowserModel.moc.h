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

/// Verdict of ezQtAssetFilter::IsAssetFiltered() for a single item.
///
/// Everything but Visible means the item is not shown. The values other than Filtered name the single reason an item
/// was excluded, which allows counting how many items a specific switch is hiding. They are only reported for items
/// that pass every other filter.
enum class ezAssetFilterResult : ezUInt8
{
  Visible,           ///< The item passes all filters and is shown.
  Filtered,          ///< The item is excluded, either for several reasons or for one that has no dedicated switch.
  HiddenFolder,      ///< Excluded only because it resides in a hidden folder. \see ezQtAssetBrowserFilter::SetShowItemsInHiddenFolders()
  NonAssetFile,      ///< Excluded only because it is a plain file and files are not shown. \see ezQtAssetBrowserFilter::SetShowFiles()
  NonImportableFile, ///< Excluded only because it is a file that can't be imported and those are not shown. \see ezQtAssetBrowserFilter::SetShowNonImportableFiles()
};

/// Interface class of the asset filter used to decide which items are shown in the asset browser.
class EZ_EDITORFRAMEWORK_DLL ezQtAssetFilter : public QObject
{
  Q_OBJECT
public:
  explicit ezQtAssetFilter(QObject* pParent);

  /// Decides whether the given item is shown.
  ///
  /// Returning ezAssetFilterResult::HiddenFolder instead of ezAssetFilterResult::Filtered is optional. It allows
  /// the model to count items that are only excluded because of the hidden-folder rule, so that their number can
  /// be reported to the user. Such items are not shown either way.
  virtual ezAssetFilterResult IsAssetFiltered(ezStringView sDataDirParentRelativePath, bool bIsFolder, const ezSubAsset* pInfo) const = 0;
  virtual bool GetSortByRecentUse() const { return false; }

Q_SIGNALS:
  void FilterChanged();
};

/// Each item in the asset browser can be multiple things at the same time as described by these flags.
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

/// Model of the item view in the asset browser.
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

  /// Number of items that pass all filters, but are not displayed because of a single switch.
  ezUInt32 GetNumExcludedItems(ezAssetFilterResult reason) const;

public Q_SLOTS:
  void ThumbnailLoaded(QString sPath, QModelIndex index, QVariant userData1, QVariant userData2);
  void ThumbnailInvalidated(QString sPath, ezUInt32 uiImageID);
  void OnFileSystemUpdate();

signals:
  void editingFinished(const QString& sAbsPath, const QString& sNewName, bool bIsAsset) const;

  /// Emitted when the result of GetNumExcludedItems() changed for any reason.
  void ExcludedItemCountsChanged();

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

  /// Re-runs the filter for every asset that lists the given asset among its missing dependencies.
  /// Needed because a filter's verdict can depend on whether those dependencies resolve, which
  /// changes when an asset is removed without any event being sent for the dependents themselves.
  void ReEvaluateDependents(const ezUuid& removedAssetGuid);

  void HandleEntry(const VisibleEntry& entry, AssetOp op);

  /// Records under which single-reason exclusion the given item currently falls, if any.
  ///
  /// Emits ExcludedItemCountsChanged() if this changed any of the counts, unless m_bSuppressExcludedItemSignal is set,
  /// in which case the emit is deferred to whoever set that flag.
  ///
  /// Set bKnownUntracked only when the caller can guarantee that the path is not currently recorded under any reason,
  /// which is the case while resetModel() refills the empty sets. It skips the scan that would drop the path from the
  /// other reasons, so passing it wrongly leaves the item counted twice.
  void TrackExcludedItem(const ezDataDirPath& path, ezAssetFilterResult reason, bool bKnownUntracked = false);

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

  // One set of paths per single-reason exclusion, indexed by ezAssetFilterResult.
  // Keyed by path rather than by GUID, because plain files that are no assets have no GUID.
  ezMap<ezAssetFilterResult, ezSet<ezString>> m_ExcludedItems;

  // Set while resetModel() is between beginResetModel() and endResetModel(). It stops TrackExcludedItem() from
  // emitting once per item, which would not only be wasteful but would also make listeners query rowCount() while
  // the model is in reset state.
  bool m_bSuppressExcludedItemSignal = false;
  bool m_bExcludedItemCountsChanged = false;

  QFileIconProvider m_IconProvider;
};
