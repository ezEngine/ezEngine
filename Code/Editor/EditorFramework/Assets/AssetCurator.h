#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/Assets/Declarations.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Algorithm/HashHelperString.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/Logging/LogEntry.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Threading/LockedObject.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Timestamp.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/FileSystem/DataDirPath.h>
#include <ToolsFoundation/FileSystem/Declarations.h>

#include <tuple>

class ezUpdateTask;
class ezTask;
class ezAssetDocumentManager;
class ezDirectoryWatcher;
class ezProcessTask;
struct ezFileStats;
class ezAssetProcessorLog;
class ezFileSystemWatcher;
class ezAssetTableWriter;
struct ezFileChangedEvent;
class ezFileSystemModel;


#if 0 // Define to enable extensive curator profile scopes
#  define CURATOR_PROFILE(szName) EZ_PROFILE_SCOPE(szName)

#else
#  define CURATOR_PROFILE(Name)

#endif

/// \brief Custom mutex that allows to profile the time in the curator lock.
class ezCuratorMutex : public ezMutex
{
public:
  void Lock()
  {
    CURATOR_PROFILE("ezCuratorMutex");
    ezMutex::Lock();
  }

  void Unlock() { ezMutex::Unlock(); }
};

struct EZ_EDITORFRAMEWORK_DLL ezAssetInfo
{
  ezAssetInfo() = default;
  void Update(ezUniquePtr<ezAssetInfo>& rhs);

  ezAssetDocumentManager* GetManager() { return static_cast<ezAssetDocumentManager*>(m_pDocumentTypeDescriptor->m_pManager); }

  enum TransformState : ezUInt8
  {
    Unknown = 0,
    UpToDate,
    NeedsImport,
    NeedsTransform,
    NeedsThumbnail,
    TransformError,
    MissingTransformDependency,
    MissingThumbnailDependency,
    MissingPackageDependency,
    CircularDependency,
    COUNT,
  };

  ezUInt8 m_LastStateUpdate = 0; ///< Changes every time m_TransformState is modified. Used to detect stale computations done outside the lock.
  ezAssetExistanceState::Enum m_ExistanceState = ezAssetExistanceState::FileAdded;
  TransformState m_TransformState = TransformState::Unknown;
  ezUInt64 m_AssetHash = 0;      ///< Valid if m_TransformState != Unknown and asset not in Curator's m_TransformStateStale list.
  ezUInt64 m_ThumbHash = 0;      ///< Valid if m_TransformState != Unknown and asset not in Curator's m_TransformStateStale list.
  ezUInt64 m_PackageHash = 0;    ///< Valid if m_TransformState != Unknown and asset not in Curator's m_TransformStateStale list.

  ezDynamicArray<ezLogEntry> m_LogEntries;

  const ezAssetDocumentTypeDescriptor* m_pDocumentTypeDescriptor = nullptr;
  ezDataDirPath m_Path;

  ezUniquePtr<ezAssetDocumentInfo> m_Info;

  ezSet<ezString> m_MissingTransformDeps;
  ezSet<ezString> m_MissingThumbnailDeps;
  ezSet<ezString> m_MissingPackageDeps;
  ezSet<ezString> m_CircularDependencies;

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



struct ezAssetCuratorEvent
{
  enum class Type
  {
    AssetAdded,
    AssetRemoved,
    AssetMoved,
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

  void MainThreadTick(bool bTopLevel);

  ///@}
  /// \name Asset Platform Configurations
  ///@{

public:
  /// \brief The main platform on which development happens. E.g. "Default".
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
  const ezPlatformProfile* GetAssetProfile(ezUInt32 uiIndex) const;

  /// \brief Always returns a valid config. E.g. even if ezInvalidIndex is passed in, it will fall back to the default config (at index 0).
  ezPlatformProfile* GetAssetProfile(ezUInt32 uiIndex);

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
  void SetActiveAssetProfileByIndex(ezUInt32 uiIndex, bool bForceReevaluation = false);

  /// \brief Saves the current asset configurations. Returns failure if the output file could not be written to.
  ezResult SaveAssetProfiles();

  void SaveRuntimeProfiles();

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
  ezDateTime GetLastFullTransformDate() const;
  void StoreFullTransformDate();

  /// \brief Transforms all assets and writes the lookup tables. If the given platform is empty, the active platform is used.
  ///
  /// Pass ezTransformFlags::TriggeredManually to make sure that all assets get transformed,
  /// even the ones that should not be transformed by background processors (mainly scenes).
  ezStatus TransformAllAssets(ezBitflags<ezTransformFlags> transformFlags = ezTransformFlags::TriggeredManually, const ezPlatformProfile* pAssetProfile = nullptr);
  ezTransformStatus TransformAsset(const ezUuid& assetGuid, ezBitflags<ezTransformFlags> transformFlags, const ezPlatformProfile* pAssetProfile = nullptr);
  ezTransformStatus CreateThumbnail(const ezUuid& assetGuid);

  void ResaveAllAssets(ezStringView sPrefixPath);

  /// Some assets are not automatically updated by the asset dependency detection (mainly Collections) because of their transitive data dependencies.
  /// So we must update them when the user does something 'significant' like doing TransformAllAssets or a scene export.
  void TransformAssetsForSceneExport(const ezPlatformProfile* pAssetProfile = nullptr);

  /// \brief Writes the asset lookup table for the given platform, or the currently active platform if nullptr is passed.
  ezResult WriteAssetTables(const ezPlatformProfile* pAssetProfile = nullptr, bool bForce = false);

  ///@}
  /// \name Asset Access
  ///@{
  using ezLockedSubAsset = ezLockedObject<ezMutex, const ezSubAsset>;

  /// \brief Tries to find the asset information for an asset identified through a string.
  ///
  /// The string may be a stringyfied asset GUID or a relative or absolute path. The function will try all possibilities.
  /// If no asset can be found, an empty/invalid ezAssetInfo is returned.
  /// If bExhaustiveSearch is set the function will go through all known assets and find the closest match.
  const ezLockedSubAsset FindSubAsset(ezStringView sPathOrGuid, bool bExhaustiveSearch = false) const;

  /// \brief Same as GetAssteInfo, but wraps the return value into a ezLockedSubAsset struct
  const ezLockedSubAsset GetSubAsset(const ezUuid& assetGuid) const;

  using ezLockedSubAssetTable = ezLockedObject<ezMutex, const ezHashTable<ezUuid, ezSubAsset>>;

  /// \brief Returns the table of all known assets in a locked structure
  const ezLockedSubAssetTable GetKnownSubAssets() const;

  using ezLockedAssetTable = ezLockedObject<ezMutex, const ezHashTable<ezUuid, ezAssetInfo*>>;

  /// \brief Returns the table of all known assets in a locked structure
  const ezLockedAssetTable GetKnownAssets() const;

  /// \brief Computes the combined hash for the asset and its dependencies. Returns 0 if anything went wrong.
  ezUInt64 GetAssetDependencyHash(ezUuid assetGuid);

  /// \brief Computes the combined hash for the asset and its references. Returns 0 if anything went wrong.
  ezUInt64 GetAssetReferenceHash(ezUuid assetGuid);

  ezAssetInfo::TransformState IsAssetUpToDate(const ezUuid& assetGuid, const ezPlatformProfile* pAssetProfile, const ezAssetDocumentTypeDescriptor* pTypeDescriptor, ezUInt64& out_uiAssetHash, ezUInt64& out_uiThumbHash, ezUInt64& out_uiPackageHash, bool bForce = false);
  /// \brief Returns the number of assets in the system and how many are in what transform state
  void GetAssetTransformStats(ezUInt32& out_uiNumAssets, ezHybridArray<ezUInt32, ezAssetInfo::TransformState::COUNT>& out_count);

  /// \brief Iterates over all known data directories and returns the absolute path to the directory in which this asset is located
  ezString FindDataDirectoryForAsset(ezStringView sAbsoluteAssetPath) const;

  /// \brief Uses knowledge about all existing files on disk to find the best match for a file. Very slow.
  ///
  /// \param sFile
  ///   File name (may include a path) to search for. Will be modified both on success and failure to give a 'reasonable' result.
  ezResult FindBestMatchForFile(ezStringBuilder& ref_sFile, ezArrayPtr<ezString> allowedFileExtensions) const;

  /// \brief Finds all uses, either as references or dependencies to a given asset.
  ///
  /// Technically this finds all references and dependencies to this asset but in practice there are no uses of transform dependencies between assets right now so the result is a list of references and can be referred to as such.
  ///
  /// \param assetGuid
  ///   The asset to find use cases for.
  /// \param uses
  ///   List of assets that use 'assetGuid'. Any previous content of the set is not removed.
  /// \param transitive
  ///   If set, will also find indirect uses of the asset.
  void FindAllUses(ezUuid assetGuid, ezSet<ezUuid>& ref_uses, bool bTransitive) const;

  /// \brief Returns all assets that use a file for transform. Use this to e.g. figure which assets still reference a .tga file in the project.
  /// \param sAbsolutePath Absolute path to any file inside a data directory.
  /// \param ref_uses List of assets that use 'sAbsolutePath'. Any previous content of the set is not removed.
  void FindAllUses(ezStringView sAbsolutePath, ezSet<ezUuid>& ref_uses) const;

  /// \brief Returns whether a file is referenced, i.e. used for transforming an asset. Use this to e.g. figure out whether a .tga file is still in use by any asset.
  /// \param sAbsolutePath Absolute path to any file inside a data directory.
  /// \return True, if at least one asset references the given file.
  bool IsReferenced(ezStringView sAbsolutePath) const;


  ///@}
  /// \name Manual and Automatic Change Notification
  ///@{

  /// \brief Allows to tell the system of a new or changed file, that might be of interest to the Curator.
  void NotifyOfFileChange(ezStringView sAbsolutePath);
  /// \brief Allows to tell the system to re-evaluate an assets status.
  void NotifyOfAssetChange(const ezUuid& assetGuid);
  void UpdateAssetLastAccessTime(const ezUuid& assetGuid);

  /// \brief Checks file system for any changes. Call in case the file system watcher does not pick up certain changes.
  void CheckFileSystem();

  void NeedsReloadResources(const ezUuid& assetGuid);

  void InvalidateAssetsWithTransformState(ezAssetInfo::TransformState state);


  ///@}

  /// \name Utilities
  ///@{

  /// \brief Generates one transitive hull for all the dependencies that are enabled. The set will contain dependencies that are reachable via any combination of enabled reference types.
  void GenerateTransitiveHull(const ezStringView sAssetOrPath, ezSet<ezString>& inout_deps, bool bIncludeTransformDeps = false, bool bIncludeThumbnailDeps = false, bool bIncludePackageDeps = false) const;

  /// \brief Generates one inverse transitive hull for all the types dependencies that are enabled. The set will contain inverse dependencies that can reach the given asset (pAssetInfo) via any combination of the enabled reference types. As only assets can have dependencies, the inverse hull is always just asset GUIDs.
  void GenerateInverseTransitiveHull(const ezAssetInfo* pAssetInfo, ezSet<ezUuid>& inout_inverseDeps, bool bIncludeTransformDeps = false, bool bIncludeThumbnailDeps = false) const;

  /// \brief Generates a DGML graph of all transform and thumbnail dependencies.
  void WriteDependencyDGML(const ezUuid& guid, ezStringView sOutputFile) const;

  ///@}

public:
  ezEvent<const ezAssetCuratorEvent&> m_Events;

private:
  /// \name Processing
  ///@{

  ezTransformStatus ProcessAsset(ezAssetInfo* pAssetInfo, const ezPlatformProfile* pAssetProfile, ezBitflags<ezTransformFlags> transformFlags);
  ezStatus ResaveAsset(ezAssetInfo* pAssetInfo);
  /// \brief Returns the asset info for the asset with the given GUID or nullptr if no such asset exists.
  ezAssetInfo* GetAssetInfo(const ezUuid& assetGuid);
  const ezAssetInfo* GetAssetInfo(const ezUuid& assetGuid) const;

  ezSubAsset* GetSubAssetInternal(const ezUuid& assetGuid);

  /// \brief Returns the asset info for the asset with the given (stringyfied) GUID or nullptr if no such asset exists.
  ezAssetInfo* GetAssetInfo(const ezString& sAssetGuid);

  void OnFileChangedEvent(const ezFileChangedEvent& e);

  /// \brief Some assets are vital for the engine to run. Each data directory can contain a [DataDirName].ezCollectionAsset
  ///   that has all its references transformed before any other documents are loaded.
  void ProcessAllCoreAssets();

  ///@}
  /// \name Update Task
  ///@{

  void RestartUpdateTask();
  void ShutdownUpdateTask();

  bool GetNextAssetToUpdate(ezUuid& out_guid, ezStringBuilder& out_sAbsPath);
  void OnUpdateTaskFinished(const ezSharedPtr<ezTask>& pTask);
  void RunNextUpdateTask();

  ///@}
  /// \name Asset Hashing and Status Updates (AssetUpdates.cpp)
  ///@{

  bool AddAssetHash(ezString& sPath, bool bIsReference, ezUInt64& out_AssetHash, ezUInt64& out_ThumbHash, ezUInt64& out_PackageHash, bool bForce);
  ezAssetInfo::TransformState HashAsset(ezUInt64 uiSettingsHash, const ezHybridArray<ezString, 16>& assetTransformDeps, const ezHybridArray<ezString, 16>& assetThumbnailDeps, const ezHybridArray<ezString, 16>& assetPackageDeps, ezSet<ezString>& missingTransformDeps, ezSet<ezString>& missingThumbnailDeps, ezSet<ezString>& missingPackageDeps, ezUInt64& out_AssetHash, ezUInt64& out_ThumbHash, ezUInt64& out_PackageHash, bool bForce);

  ezResult EnsureAssetInfoUpdated(const ezDataDirPath& absFilePath, const ezFileStatus& stat, bool bForce = false);
  void TrackDependencies(ezAssetInfo* pAssetInfo);
  void UntrackDependencies(ezAssetInfo* pAssetInfo);
  ezResult CheckForCircularDependencies(ezAssetInfo* pAssetInfo);
  void UpdateTrackedFiles(const ezUuid& assetGuid, const ezSet<ezString>& files, ezMap<ezString, ezHybridArray<ezUuid, 1>>& inverseTracker, ezSet<std::tuple<ezUuid, ezUuid>>& unresolved, bool bAdd);
  void UpdateUnresolvedTrackedFiles(ezMap<ezString, ezHybridArray<ezUuid, 1>>& inverseTracker, ezSet<std::tuple<ezUuid, ezUuid>>& unresolved);
  ezResult ReadAssetDocumentInfo(const ezDataDirPath& absFilePath, const ezFileStatus& stat, ezUniquePtr<ezAssetInfo>& assetInfo);
  void UpdateSubAssets(ezAssetInfo& assetInfo);

  void RemoveAssetTransformState(const ezUuid& assetGuid);
  void InvalidateAssetTransformState(const ezUuid& assetGuid);

  ezAssetInfo::TransformState UpdateAssetTransformState(ezUuid assetGuid, ezUInt64& out_AssetHash, ezUInt64& out_ThumbHash, ezUInt64& out_PackageHash, bool bForce);
  void UpdateAssetTransformState(const ezUuid& assetGuid, ezAssetInfo::TransformState state);
  void UpdateAssetTransformLog(const ezUuid& assetGuid, ezDynamicArray<ezLogEntry>& logEntries);
  void SetAssetExistanceState(ezAssetInfo& assetInfo, ezAssetExistanceState::Enum state);

  ///@}
  /// \name Check File System Helper
  ///@{
  void SetAllAssetStatusUnknown();
  void LoadCaches(ezMap<ezDataDirPath, ezFileStatus, ezCompareDataDirPath>& out_referencedFiles, ezMap<ezDataDirPath, ezFileStatus::Status, ezCompareDataDirPath>& out_referencedFolders);
  void SaveCaches(const ezMap<ezDataDirPath, ezFileStatus, ezCompareDataDirPath>& referencedFiles, const ezMap<ezDataDirPath, ezFileStatus::Status, ezCompareDataDirPath>& referencedFolders);
  static void BuildFileExtensionSet(ezSet<ezString>& AllExtensions);

  ///@}
  /// \name Utilities
  ///@{

public:
  /// \brief Deletes all files in all asset caches, except for the asset outputs that exceed the threshold.
  ///
  /// -> OutputReliability::Perfect -> deletes everything
  /// -> OutputReliability::Good -> keeps the 'Perfect' files
  /// -> OutputReliability::Unknown -> keeps the 'Good' and 'Perfect' files
  void ClearAssetCaches(ezAssetDocumentManager::OutputReliability threshold);

  ///@}

private:
  friend class ezUpdateTask;
  friend class ezAssetProcessor;
  friend class ezProcessTask;

  mutable ezCuratorMutex m_CuratorMutex; // Global lock
  ezTaskGroupID m_InitializeCuratorTaskID;

  ezUInt32 m_uiActiveAssetProfile = 0;

  // Actual data stored in the curator
  ezHashTable<ezUuid, ezAssetInfo*> m_KnownAssets;
  ezHashTable<ezUuid, ezSubAsset> m_KnownSubAssets;

  // Derived dependency lookup tables
  ezMap<ezString, ezHybridArray<ezUuid, 1>> m_InverseTransformDeps; // [Absolute path -> asset Guid]
  ezMap<ezString, ezHybridArray<ezUuid, 1>> m_InverseThumbnailDeps; // [Absolute path -> asset Guid]
  ezSet<std::tuple<ezUuid, ezUuid>> m_UnresolvedTransformDeps;      ///< If a dependency wasn't known yet when an asset info was loaded, it is put in here.
  ezSet<std::tuple<ezUuid, ezUuid>> m_UnresolvedThumbnailDeps;

  // State caches
  ezHashSet<ezUuid> m_TransformState[ezAssetInfo::TransformState::COUNT];
  ezHashSet<ezUuid> m_SubAssetChanged; ///< Flushed in main thread tick
  ezHashSet<ezUuid> m_TransformStateStale;
  ezHashSet<ezUuid> m_Updating;

  // Serialized cache
  mutable ezCuratorMutex m_CachedAssetsMutex; ///< Only locks m_CachedAssets
  ezMap<ezString, ezUniquePtr<ezAssetDocumentInfo>> m_CachedAssets;
  ezMap<ezString, ezFileStatus> m_CachedFiles;

  // Immutable data after StartInitialize
  ezApplicationFileSystemConfig m_FileSystemConfig;
  ezUniquePtr<ezAssetTableWriter> m_pAssetTableWriter;
  ezSet<ezString> m_ValidAssetExtensions;

  // Update task
  bool m_bRunUpdateTask = false;
  ezSharedPtr<ezUpdateTask> m_pUpdateTask;
  ezTaskGroupID m_UpdateTaskGroup;
};

class ezUpdateTask final : public ezTask
{
public:
  ezUpdateTask(ezOnTaskFinishedCallback onTaskFinished);
  ~ezUpdateTask();

private:
  ezStringBuilder m_sAssetPath;

  virtual void Execute() override;
};
