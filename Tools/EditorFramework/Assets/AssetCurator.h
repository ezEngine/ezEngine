#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Core/Application/Config/FileSystemConfig.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Threading/LockedObject.h>
#include <tuple>

class ezUpdateTask;
class ezTask;
class ezAssetDocumentManager;
class ezDirectoryWatcher;
class ezProcessCommunication;
class ezProcessTask;
struct ezFileStats;
struct AssetCacheEntry;

struct ezAssetInfo
{
  enum class ExistanceState
  {
    FileAdded,
    FileUnchanged,
    FileRemoved
  };

  enum class TransformState
  {
    Unknown,
    Updating,
    NeedsTransform,
    NeedsThumbnail,
    UpToDate,
    MissingDependency,
    MissingReference,
  };

  ezAssetInfo() : m_pManager(nullptr)
  {
    m_ExistanceState = ExistanceState::FileAdded;
    m_TransformState = TransformState::Unknown;
    m_LastAssetDependencyHash = 0;
  }

  ExistanceState m_ExistanceState;
  TransformState m_TransformState;
  ezAssetDocumentManager* m_pManager;
  ezString m_sAbsolutePath;
  ezString m_sDataDirRelativePath;
  ezTime m_LastAccess;

  ezAssetDocumentInfo m_Info;

  ezUInt64 m_LastAssetDependencyHash; ///< For debugging only.
  ezSet<ezString> m_MissingDependencies;
  ezSet<ezString> m_MissingReferences;
};

struct AssetCacheEntry
{
  ezString m_sFile;
  ezTimestamp m_Timestamp;
  ezUInt64 m_uiHash;
  ezAssetDocumentInfo m_Info;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, AssetCacheEntry);

struct ezAssetCuratorEvent
{
  enum class Type
  {
    AssetAdded,
    AssetRemoved,
    AssetUpdated,
    AssetListReset,
    ActivePlatformChanged,
    ProcessTaskStateChanged
  };

  ezUuid m_AssetGuid;
  const ezAssetInfo* m_pInfo;
  Type m_Type;
};

class EZ_EDITORFRAMEWORK_DLL ezAssetCurator
{
  EZ_DECLARE_SINGLETON(ezAssetCurator);

public:
  ezAssetCurator();
  ~ezAssetCurator();

  /// \name Setup
  ///@{

  void Initialize(const ezApplicationFileSystemConfig& cfg);
  void Deinitialize();

  const char* GetActivePlatform() const { return m_sActivePlatform; }
  void SetActivePlatform(const char* szPlatform);

  void MainThreadTick();

  ///@}
  /// \name High Level Functions
  ///@{

  /// \brief Transforms all assets and writes the lookup tables. If the given platform is empty, the active platform is used.
  void TransformAllAssets(const char* szPlatform = nullptr);
  ezStatus TransformAsset(const ezUuid& assetGuid, const char* szPlatform = nullptr);
  ezStatus CreateThumbnail(const ezUuid& assetGuid);

  /// \brief Writes the asset lookup table for the given platform, or the currently active platform if nullptr is passed.
  ezResult WriteAssetTables(const char* szPlatform = nullptr);

  void RestartProcessTask();
  void ShutdownProcessTask();
  bool IsProcessTaskRunning() const;

  ///@}
  /// \name Asset Access
  ///@{
  typedef ezLockedObject<ezMutex, ezAssetInfo> ezLockedAssetInfo;

  /// \brief Tries to find the asset information for an asset identified through a string.
  ///
  /// The string may be a stringyfied asset GUID or a relative or absolute path. The function will try all possibilities.
  /// If no asset can be found, an empty/invalid ezAssetInfo is returned.
  const ezLockedAssetInfo FindAssetInfo(const char* szPathOrGuid) const;

  /// \brief Same as GetAssteInfo, but wraps the return value into a ezLockedAssetInfo struct
  const ezLockedAssetInfo GetAssetInfo2(const ezUuid& assetGuid) const;

  typedef ezLockedObject<ezMutex, const ezHashTable<ezUuid, ezAssetInfo*>> ezLockedAssetTable;

  /// \brief Returns the table of all known assets in a locked structure
  const ezLockedAssetTable GetKnownAssets() const;

  /// \brief Computes the combined hash for the asset and its dependencies. Returns 0 if anything went wrong.
  ezUInt64 GetAssetDependencyHash(ezUuid assetGuid);

  /// \brief Computes the combined hash for the asset and its references. Returns 0 if anything went wrong.
  ezUInt64 GetAssetReferenceHash(ezUuid assetGuid);

  ezAssetInfo::TransformState IsAssetUpToDate(const ezUuid& assetGuid, const char* szPlatform, const ezDocumentTypeDescriptor* pTypeDescriptor, ezUInt64& out_AssetHash, ezUInt64& out_ThumbHash);
  /// \brief Returns the number of assets in the system and how many are in what transform state
  void GetAssetTransformStats(ezUInt32& out_uiNumAssets, ezUInt32& out_uiNumUnknown, ezUInt32& out_uiNumNeedTransform, ezUInt32& out_uiNumNeedThumb, ezUInt32& out_uiNumMissingDep, ezUInt32& out_uiNumMissingRef);

  /// \brief Iterates over all known data directories and returns the absolute path to the directory in which this asset is located
  ezString FindDataDirectoryForAsset(const char* szAbsoluteAssetPath) const;

  /// \brief The curator gathers all folders in which assets have been found. This list can only grow over the lifetime of the application.
  const ezSet<ezString>& GetAllAssetFolders() const { return m_AssetFolders; }

  ///@}
  /// \name Manual and Automatic Change Notification
  ///@{

  // TODO: Background file-system watcher and main thread update tick to add background info to main data

  /// \brief Allows to tell the system of a new or changed file, that might be of interest to the Curator.
  void NotifyOfFileChange(const char* szAbsolutePath);

  /// \brief Allows to tell the system to re-evaluate an assets status.
  void NotifyOfAssetChange(const ezUuid& assetGuid);
  void UpdateAssetLastAccessTime(const ezUuid& assetGuid);

  /// \brief Checks file system for any changes. Call in case the file system watcher does not pick up certain changes.
  void CheckFileSystem();

  ///@}


public:

  ezEvent<const ezAssetCuratorEvent&> m_Events;


private:

  /// \brief Information about a single file on disk. The file might be an asset or any other file (needed for dependencies).
  struct FileStatus
  {
    enum class Status
    {
      Unknown,    ///< Since the file has been tagged as 'Unknown' it has not been encountered again on disk (yet)
      FileLocked, ///< The file is probably an asset, but we could not read it
      Valid       ///< The file exists on disk
    };

    FileStatus()
    {
      m_uiHash = 0;
      m_Status = Status::Unknown;
    }

    ezTimestamp m_Timestamp;
    ezUInt64 m_uiHash;
    ezUuid m_AssetGuid; ///< If the file is linked to an asset, the GUID is valid, otherwise not.
    Status m_Status;
  };

  void DocumentManagerEventHandler(const ezDocumentManager::Event& r);

  /// \name Processing
  ///@{

  ezStatus ProcessAsset(ezAssetInfo* pAssetInfo, const char* szPlatform);

  /// \brief Returns the asset info for the asset with the given GUID or nullptr if no such asset exists.
  ezAssetInfo* GetAssetInfo(const ezUuid& assetGuid);

  /// \brief Returns the asset info for the asset with the given (stringyfied) GUID or nullptr if no such asset exists.
  ezAssetInfo* GetAssetInfo(const ezString& sAssetGuid);

  void HandleSingleFile(const ezString& sAbsolutePath);
  void HandleSingleFile(const ezString& sAbsolutePath, const ezSet<ezString>& validExtensions, const ezFileStats& FileStat);
  /// \brief Writes the asset lookup table for the given platform, or the currently active platform if nullptr is passed.
  ezResult WriteAssetTable(const char* szDataDirectory, const char* szPlatform = nullptr);
  /// \brief Some assets are vital for the engine to run. Each data directory can contain a [DataDirName].ezCollectionAsset
  ///   that has all its references transformed before any other documents are loaded.
  void ProcessAllCoreAssets();

  ///@}
  /// \name Update Task
  ///@{

  void RestartUpdateTask();
  void ShutdownUpdateTask();

  bool GetNextAssetToUpdate(ezUuid& out_guid, ezStringBuilder& out_sAbsPath);
  void OnUpdateTaskFinished(ezTask* pTask);
  void RunNextUpdateTask();

  ///@}
  /// \name Process Task
  ///@{

  bool GetNextAssetToProcess(ezAssetInfo* pInfo, ezUuid& out_guid, ezStringBuilder& out_sAbsPath);
  bool GetNextAssetToProcess(ezUuid& out_guid, ezStringBuilder& out_sAbsPath);
  void OnProcessTaskFinished(ezTask* pTask);
  void RunNextProcessTask();

  ///@}
  /// \name Asset Hashing and Status Updates (AssetUpdates.cpp)
  ///@{

  ezUInt64 GetAssetHash(ezUuid assetGuid, bool bReferences);
  bool AddAssetHash(ezString& sPath, bool bReferences, ezUInt64& uiHashResult);

  ezResult EnsureAssetInfoUpdated(const ezUuid& assetGuid);
  ezResult EnsureAssetInfoUpdated(const char* szAbsFilePath);
  void TrackDependencies(ezAssetInfo* pAssetInfo);
  void UntrackDependencies(ezAssetInfo* pAssetInfo);
  void UpdateTrackedFiles(const ezUuid& assetGuid, const ezSet<ezString>& files, ezMap<ezString, ezHybridArray<ezUuid, 1> >& inverseTracker, ezSet<std::tuple<ezUuid, ezUuid> >& unresolved, bool bAdd);
  void UpdateUnresolvedTrackedFiles(ezMap<ezString, ezHybridArray<ezUuid, 1> >& inverseTracker, ezSet<std::tuple<ezUuid, ezUuid> >& unresolved);
  ezResult UpdateAssetInfo(const char* szAbsFilePath, ezAssetCurator::FileStatus& stat, ezAssetInfo& assetInfo, const ezFileStats* pFileStat);
  /// \brief Opens the asset JSON file and reads the "Header" into the given ezAssetDocumentInfo.
  static ezStatus ReadAssetDocumentInfo(ezAssetDocumentInfo* pInfo, ezStreamReader& stream);
  /// \brief Computes the hash of the given file. Optionally passes the data stream through into another stream writer.
  static ezUInt64 HashFile(ezStreamReader& InputStream, ezStreamWriter* pPassThroughStream);

  void UpdateAssetTransformState(const ezUuid& assetGuid, ezAssetInfo::TransformState state);

  ///@}
  /// \name Check File System Helper
  ///@{

  void SetAllAssetStatusUnknown();
  void RemoveStaleFileInfos();
  static void BuildFileExtensionSet(ezSet<ezString>& AllExtensions);
  void IterateDataDirectory(const char* szDataDir, const ezSet<ezString>& validExtensions);
  void LoadCaches();
  void SaveCaches();

  ///@}

private:
  friend class ezUpdateTask;
  friend class ezProcessTask;

  bool m_bRunUpdateTask;
  bool m_bRunProcessTask;
  bool m_bNeedToReloadResources;
  ezSet<ezUuid> m_TransformStateUnknown;
  ezSet<ezUuid> m_TransformStateUpdating;
  ezSet<ezUuid> m_TransformStateNeedsTransform;
  ezSet<ezUuid> m_TransformStateNeedsThumbnail;
  ezSet<ezUuid> m_TransformStateMissingDependency;
  ezSet<ezUuid> m_TransformStateMissingReference;

  ezSet<ezUuid> m_TransformStateChanged;

  ezHashTable<ezUuid, ezAssetInfo*> m_KnownAssets;
  ezMap<ezString, FileStatus, ezCompareString_NoCase > m_ReferencedFiles;

  ezMap<ezString, ezUniquePtr<AssetCacheEntry>, ezCompareString_NoCase > m_CachedEntries;

  ezMap<ezString, ezHybridArray<ezUuid, 1> > m_InverseDependency;
  ezMap<ezString, ezHybridArray<ezUuid, 1> > m_InverseReferences;
  ezSet<std::tuple<ezUuid, ezUuid> > m_UnresolvedDependencies; ///< If a dependency wasn't known yet when an asset info was loaded, it is put in here.
  ezSet<std::tuple<ezUuid, ezUuid> > m_UnresolvedReferences;

  ezApplicationFileSystemConfig m_FileSystemConfig;
  ezString m_sActivePlatform;
  ezSet<ezString> m_ValidAssetExtensions;
  ezSet<ezString> m_AssetFolders;

  mutable ezMutex m_CuratorMutex;
  ezUpdateTask* m_pUpdateTask;
  ezDynamicArray<ezDirectoryWatcher*> m_Watchers;
  ezDynamicArray<ezProcessTask*> m_ProcessTasks;
  ezTaskGroupID m_ProcessTaskGroup; ///< TODO: Does not work at the moment as grouping marks tasks as running
};

class ezUpdateTask : public ezTask
{
public:
  ezUpdateTask();


private:
  ezStringBuilder m_sAssetPath;

  virtual void Execute() override;
};

class ezProcessTask : public ezTask
{
public:
  ezProcessTask();
  ~ezProcessTask();

  void EventHandlerIPC(const ezProcessCommunication::Event& e);
  void StartProcess();
  void ShutdownProcess();

private:
  ezUuid m_assetGuid;
  ezStringBuilder m_sAssetPath;
  ezProcessCommunication* m_pIPC;
  bool m_bProcessShouldBeRunning;
  bool m_bProcessCrashed;
  bool m_bWaiting;
  bool m_bSuccess;
  virtual void Execute() override;
};
