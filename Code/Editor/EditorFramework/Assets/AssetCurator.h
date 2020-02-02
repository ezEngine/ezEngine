#pragma once

#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/Assets/Declarations.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Algorithm/HashHelperString.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Logging/LogEntry.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Threading/LockedObject.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Timestamp.h>
#include <GameEngine/Configuration/PlatformProfile.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <tuple>

class ezUpdateTask;
class ezTask;
class ezAssetDocumentManager;
class ezDirectoryWatcher;
class ezProcessTask;
struct ezFileStats;
class ezAssetProcessorLog;

#if 0 // Define to enable extensive curator profile scopes
#  define CURATOR_PROFILE(szName) EZ_PROFILE_SCOPE(szName)

#else
#  define CURATOR_PROFILE(Name)

#endif

/// \brief Custom mutex that allows to profile the time in the curator lock.
class ezCuratorMutex : public ezMutex
{
public:
  void Acquire()
  {
    CURATOR_PROFILE("ezCuratorMutex");
    ezMutex::Acquire();
  }

  void Release() { ezMutex::Release(); }
};

struct EZ_EDITORFRAMEWORK_DLL ezAssetInfo
{
  ezAssetInfo() = default;
  void Update(ezUniquePtr<ezAssetInfo>& rhs);

  ezAssetDocumentManager* GetManager()
  {
    return static_cast<ezAssetDocumentManager*>(m_pDocumentTypeDescriptor->m_pManager);
  }

  enum TransformState
  {
    Unknown = 0,
    UpToDate,
    Updating,
    NeedsTransform,
    NeedsThumbnail,
    TransformError,
    MissingDependency,
    MissingReference,
    COUNT,
  };
  ezAssetExistanceState::Enum m_ExistanceState = ezAssetExistanceState::FileAdded;
  TransformState m_TransformState = TransformState::Unknown;
  ezDynamicArray<ezLogEntry> m_LogEntries;

  const ezAssetDocumentTypeDescriptor* m_pDocumentTypeDescriptor = nullptr;
  ezString m_sAbsolutePath;
  ezString m_sDataDirRelativePath;

  ezUniquePtr<ezAssetDocumentInfo> m_Info;

  ezUInt64 m_LastAssetDependencyHash = 0; ///< For debugging only.
  ezSet<ezString> m_MissingDependencies;
  ezSet<ezString> m_MissingReferences;

  ezSet<ezUuid> m_SubAssets; ///< Main asset uses the same GUID as this (see m_Info), but is NOT stored in m_SubAssets

private:
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAssetInfo);
};

/// \brief Information about an asset or sub-asset.
struct EZ_EDITORFRAMEWORK_DLL ezSubAsset
{
  ezStringView GetName() const;
  void GetSubAssetIdentifier(ezStringBuilder& out_sPath) const;

  ezAssetExistanceState::Enum m_ExistanceState = ezAssetExistanceState::FileAdded;
  ezAssetInfo* m_pAssetInfo = nullptr;
  ezTime m_LastAccess;
  bool m_bMainAsset = true;

  ezSubAssetData m_Data;
};

/// \brief Information about a single file on disk. The file might be an asset or any other file (needed for dependencies).
struct EZ_EDITORFRAMEWORK_DLL ezFileStatus
{
  enum class Status
  {
    Unknown,    ///< Since the file has been tagged as 'Unknown' it has not been encountered again on disk (yet)
    FileLocked, ///< The file is probably an asset, but we could not read it
    Valid       ///< The file exists on disk
  };

  ezFileStatus()
  {
    m_uiHash = 0;
    m_Status = Status::Unknown;
  }

  ezTimestamp m_Timestamp;
  ezUInt64 m_uiHash;
  ezUuid m_AssetGuid; ///< If the file is linked to an asset, the GUID is valid, otherwise not.
  Status m_Status;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezFileStatus);

struct ezAssetCuratorEvent
{
  enum class Type
  {
    AssetAdded,
    AssetRemoved,
    AssetUpdated,
    AssetListReset,
    ActivePlatformChanged,
  };

  ezUuid m_AssetGuid;
  const ezSubAsset* m_pInfo;
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

  /// \brief Starts init task. Need to call WaitForInitialize to finish before loading docs.
  void StartInitialize(const ezApplicationFileSystemConfig& cfg);
  /// \brief Waits for init task to finish.
  void WaitForInitialize();
  void Deinitialize();

  void MainThreadTick();

  ///@}
  /// \name Asset Platform Configurations
  ///@{

public:
  /// \brief The main platform on which development happens. E.g. "PC".
  ///
  /// TODO: review this concept
  const ezPlatformProfile* GetDevelopmentAssetProfile() const;

  /// \brief The currently active target platform for asset processing.
  const ezPlatformProfile* GetActiveAssetProfile() const;

  /// \brief Returns the index of the currently active asset platform configuration
  ezUInt32 GetActiveAssetProfileIndex() const;

  /// \brief Returns ezInvalidIndex if no config with the given name exists. Name comparison is case insensitive.
  ezUInt32 FindAssetProfileByName(const char* szPlatform);

  ezUInt32 GetNumAssetProfiles() const;

  /// \brief Always returns a valid config. E.g. even if ezInvalidIndex is passed in, it will fall back to the default config (at index 0).
  const ezPlatformProfile* GetAssetProfile(ezUInt32 index) const;

  /// \brief Always returns a valid config. E.g. even if ezInvalidIndex is passed in, it will fall back to the default config (at index 0).
  ezPlatformProfile* GetAssetProfile(ezUInt32 index);

  /// \brief Adds a new profile. The name should be set afterwards to a unique name.
  ezPlatformProfile* CreateAssetProfile();

  /// \brief Deletes the given asset profile, if possible.
  ///
  /// The function fails when the given profile is the main profile (at index 0),
  /// or it is the currently active profile.
  ezResult DeleteAssetProfile(ezPlatformProfile* pProfile);

  /// \brief Switches the currently active asset target platform.
  ///
  /// Broadcasts ezAssetCuratorEvent::Type::ActivePlatformChanged on change.
  void SetActiveAssetProfileByIndex(ezUInt32 index, bool bForceReevaluation = false);

  /// \brief Saves the current asset configurations. Returns failure if the output file could not be written to.
  ezResult SaveAssetProfiles();

private:
  void ClearAssetProfiles();
  void SetupDefaultAssetProfiles();
  ezResult LoadAssetProfiles();
  void ComputeAllDocumentManagerAssetProfileHashes();

  ezHybridArray<ezPlatformProfile*, 8> m_AssetProfiles;

  ///@}
  /// \name High Level Functions
  ///@{

public:
  /// \brief Transforms all assets and writes the lookup tables. If the given platform is empty, the active platform is used.
  void TransformAllAssets(ezBitflags<ezTransformFlags> transformFlags, const ezPlatformProfile* pAssetProfile = nullptr);
  void ResaveAllAssets();
  ezStatus TransformAsset(const ezUuid& assetGuid, ezBitflags<ezTransformFlags> transformFlags, const ezPlatformProfile* pAssetProfile = nullptr);
  ezStatus CreateThumbnail(const ezUuid& assetGuid);

  /// \brief Writes the asset lookup table for the given platform, or the currently active platform if nullptr is passed.
  ezResult WriteAssetTables(const ezPlatformProfile* pAssetProfile = nullptr);

  ///@}
  /// \name Asset Access
  ///@{
  typedef ezLockedObject<ezMutex, const ezSubAsset> ezLockedSubAsset;

  /// \brief Tries to find the asset information for an asset identified through a string.
  ///
  /// The string may be a stringyfied asset GUID or a relative or absolute path. The function will try all possibilities.
  /// If no asset can be found, an empty/invalid ezAssetInfo is returned.
  const ezLockedSubAsset FindSubAsset(const char* szPathOrGuid) const;

  /// \brief Same as GetAssteInfo, but wraps the return value into a ezLockedSubAsset struct
  const ezLockedSubAsset GetSubAsset(const ezUuid& assetGuid) const;

  typedef ezLockedObject<ezMutex, const ezMap<ezUuid, ezSubAsset>> ezLockedSubAssetTable;

  /// \brief Returns the table of all known assets in a locked structure
  const ezLockedSubAssetTable GetKnownSubAssets() const;

  /// \brief Computes the combined hash for the asset and its dependencies. Returns 0 if anything went wrong.
  ezUInt64 GetAssetDependencyHash(ezUuid assetGuid);

  /// \brief Computes the combined hash for the asset and its references. Returns 0 if anything went wrong.
  ezUInt64 GetAssetReferenceHash(ezUuid assetGuid);

  ezAssetInfo::TransformState IsAssetUpToDate(const ezUuid& assetGuid, const ezPlatformProfile* pAssetProfile, const ezAssetDocumentTypeDescriptor* pTypeDescriptor, ezUInt64& out_AssetHash,
    ezUInt64& out_ThumbHash);
  /// \brief Returns the number of assets in the system and how many are in what transform state
  void GetAssetTransformStats(ezUInt32& out_uiNumAssets, ezHybridArray<ezUInt32, ezAssetInfo::TransformState::COUNT>& out_count);

  /// \brief Iterates over all known data directories and returns the absolute path to the directory in which this asset is located
  ezString FindDataDirectoryForAsset(const char* szAbsoluteAssetPath) const;

  /// \brief The curator gathers all folders in which assets have been found. This list can only grow over the lifetime of the application.
  const ezSet<ezString>& GetAllAssetFolders() const { return m_AssetFolders; }

  /// \brief Uses knowledge about all existing files on disk to find the best match for a file. Very slow.
  ///
  /// \param sFile
  ///   File name (may include a path) to search for. Will be modified both on success and failure to give a 'reasonable' result.
  ezResult FindBestMatchForFile(ezStringBuilder& sFile, ezArrayPtr<ezString> AllowedFileExtensions) const;

  ///@}
  /// \name Manual and Automatic Change Notification
  ///@{

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
  /// \name Processing
  ///@{

  ezStatus ProcessAsset(ezAssetInfo* pAssetInfo, const ezPlatformProfile* pAssetProfile, ezBitflags<ezTransformFlags> transformFlags);
  ezStatus ResaveAsset(ezAssetInfo* pAssetInfo);
  /// \brief Returns the asset info for the asset with the given GUID or nullptr if no such asset exists.
  ezAssetInfo* GetAssetInfo(const ezUuid& assetGuid);
  ezSubAsset* GetSubAssetInternal(const ezUuid& assetGuid);

  /// \brief Returns the asset info for the asset with the given (stringyfied) GUID or nullptr if no such asset exists.
  ezAssetInfo* GetAssetInfo(const ezString& sAssetGuid);

  void HandleSingleFile(const ezString& sAbsolutePath);
  void HandleSingleFile(const ezString& sAbsolutePath, const ezSet<ezString>& validExtensions, const ezFileStats& FileStat);
  /// \brief Writes the asset lookup table for the given platform, or the currently active platform if nullptr is passed.
  ezResult WriteAssetTable(const char* szDataDirectory, const ezPlatformProfile* pAssetProfile = nullptr);
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
  /// \name Asset Hashing and Status Updates (AssetUpdates.cpp)
  ///@{

  ezUInt64 GetAssetHash(ezUuid assetGuid, bool bReferences);
  bool AddAssetHash(ezString& sPath, bool bReferences, ezUInt64& uiHashResult);

  ezResult EnsureAssetInfoUpdated(const ezUuid& assetGuid);
  ezResult EnsureAssetInfoUpdated(const char* szAbsFilePath);
  void TrackDependencies(ezAssetInfo* pAssetInfo);
  void UntrackDependencies(ezAssetInfo* pAssetInfo);
  void UpdateTrackedFiles(const ezUuid& assetGuid, const ezSet<ezString>& files, ezMap<ezString, ezHybridArray<ezUuid, 1>>& inverseTracker,
    ezSet<std::tuple<ezUuid, ezUuid>>& unresolved, bool bAdd);
  void UpdateUnresolvedTrackedFiles(ezMap<ezString, ezHybridArray<ezUuid, 1>>& inverseTracker,
    ezSet<std::tuple<ezUuid, ezUuid>>& unresolved);
  ezResult ReadAssetDocumentInfo(const char* szAbsFilePath, ezFileStatus& stat, ezUniquePtr<ezAssetInfo>& assetInfo);
  void UpdateSubAssets(ezAssetInfo& assetInfo);
  /// \brief Computes the hash of the given file. Optionally passes the data stream through into another stream writer.
  static ezUInt64 HashFile(ezStreamReader& InputStream, ezStreamWriter* pPassThroughStream);

  void RemoveAssetTransformState(const ezUuid& assetGuid);
  void InvalidateAssetTransformState(const ezUuid& assetGuid);
  void UpdateAssetTransformState(const ezUuid& assetGuid, ezAssetInfo::TransformState state);
  void UpdateAssetTransformLog(const ezUuid& assetGuid, ezDynamicArray<ezLogEntry>& logEntries);
  void SetAssetExistanceState(ezAssetInfo& assetInfo, ezAssetExistanceState::Enum state);

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
  friend class ezAssetProcessor;

  ezAtomicInteger32 m_bInitStarted =
    false; // Used to figure out whether the task system has already started the init task so we don't still its lock.
  bool m_bRunUpdateTask;
  bool m_bNeedToReloadResources;

  // Actual data stored in the curator
  ezHashTable<ezUuid, ezAssetInfo*> m_KnownAssets;
  ezMap<ezUuid, ezSubAsset> m_KnownSubAssets;
  ezHashTable<ezString, ezFileStatus, ezHashHelperString_NoCase> m_ReferencedFiles;

  // Derived dependency lookup tables
  ezMap<ezString, ezHybridArray<ezUuid, 1>> m_InverseDependency;
  ezMap<ezString, ezHybridArray<ezUuid, 1>> m_InverseReferences;
  ezSet<std::tuple<ezUuid, ezUuid>>
    m_UnresolvedDependencies; ///< If a dependency wasn't known yet when an asset info was loaded, it is put in here.
  ezSet<std::tuple<ezUuid, ezUuid>> m_UnresolvedReferences;

  // State caches
  ezSet<ezUuid> m_TransformState[ezAssetInfo::TransformState::COUNT];
  ezSet<ezUuid> m_SubAssetChanged; ///< Flushed in main thread tick
  ezSet<ezUuid> m_TransformStateStale;

  // Serialized cache
  mutable ezCuratorMutex m_CachedAssetsMutex; ///< Only locks m_CachedAssets
  ezMap<ezString, ezUniquePtr<ezAssetDocumentInfo>> m_CachedAssets;
  ezMap<ezString, ezFileStatus> m_CachedFiles;


  ezApplicationFileSystemConfig m_FileSystemConfig;
  ezUInt32 m_uiActiveAssetProfile = 0;
  ezSet<ezString> m_ValidAssetExtensions;
  ezSet<ezString> m_AssetFolders;

  mutable ezCuratorMutex m_CuratorMutex;
  ezDynamicArray<ezDirectoryWatcher*> m_Watchers;
  ezTask* m_WatcherTask = nullptr;
  ezTaskGroupID m_WatcherGroup;
  ezDynamicArray<ezString> m_WatcherResults;

  ezUpdateTask* m_pUpdateTask;
};

class ezUpdateTask : public ezTask
{
public:
  ezUpdateTask();


private:
  ezStringBuilder m_sAssetPath;

  virtual void Execute() override;
};
