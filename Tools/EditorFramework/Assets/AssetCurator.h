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

class ezHashingTask;
class ezTask;
class ezAssetDocumentManager;
struct ezFileStats;

class EZ_EDITORFRAMEWORK_DLL ezAssetCurator
{
public:
  ezAssetCurator();
  ~ezAssetCurator();
  static ezAssetCurator* GetInstance() { return s_pInstance; }

  struct AssetInfo
  {
    enum class State
    {
      New,
      Default,
      ToBeDeleted
    };

    AssetInfo() : m_pManager(nullptr) 
    {
      m_State = State::New;
    }

    State m_State;
    ezAssetDocumentManager* m_pManager;
    ezString m_sAbsolutePath;
    ezString m_sRelativePath;
    ezAssetDocumentInfo m_Info;
    ezTime m_LastAccess;
  };

  void CheckFileSystem();

  void TransformAllAssets();
  ezResult WriteAssetTables();
  void MainThreadTick();

  void Initialize(const ezApplicationFileSystemConfig& cfg);
  void Deinitialize();

  const AssetInfo* FindAssetInfo(const char* szRelativePath) const;

  const AssetInfo* GetAssetInfo(const ezUuid& assetGuid) const;
  const ezHashTable<ezUuid, AssetInfo*>& GetKnownAssets() const;

  void UpdateAssetLastAccessTime(const ezUuid& assetGuid);

  /// \brief Computes the combined hash for the asset and its dependencies. Returns 0 if anything went wrong.
  ezUInt64 GetAssetDependencyHash(ezUuid assetGuid);

  /// \brief Iterates over all known data directories and returns the absolute path to the directory in which this asset is located
  ezString FindDataDirectoryForAsset(const char* szAbsoluteAssetPath) const;

  // TODO: Background filesystem watcher and main thread update tick to add background info to main data
  // TODO: Hash changed in different thread

  const char* GetActivePlatform() const { return m_sActivePlatform; }
  void SetActivePlatform(const char* szPlatform) { m_sActivePlatform = szPlatform; /* TODO: send an event */ }

  /// \brief Allows to tell the system of a new or changed file, that might be of interest to the Curator.
  void NotifyOfPotentialAsset(const char* szAbsolutePath);

  /// \brief The curator gathers all folders in which assets have been found. This list can only grow over the lifetime of the application.
  const ezSet<ezString>& GetAllAssetFolders() const { return m_AssetFolders; }

public:
  struct Event
  {
    enum class Type
    {
      AssetAdded,
      AssetRemoved,
      AssetChanged,
      AssetListReset,
    };

    ezUuid m_AssetGuid;
    AssetInfo* m_pInfo;
    Type m_Type;
  };

  ezEvent<const Event&> m_Events;


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

  void DocumentManagerEventHandler(const ezDocumentManagerBase::Event& r);
  static void BuildFileExtensionSet(ezSet<ezString>& AllExtensions);
  void IterateDataDirectory(const char* szDataDir, const ezSet<ezString>& validExtensions);
  void HandleSingleFile(const ezString& sAbsolutePath);
  void HandleSingleFile(const ezString& sAbsolutePath, const ezSet<ezString>& validExtensions, const ezFileStats& FileStat);

  void SetAllAssetStatusUnknown();
  void RemoveStaleFileInfos();
  void QueueFilesForHashing();
  void QueueFileForHashing(const ezString& sFile);

  /// \brief Reads the asset JSON file
  static ezResult UpdateAssetInfo(const char* szAbsFilePath, ezAssetCurator::FileStatus& stat, ezAssetCurator::AssetInfo& assetInfo, const ezFileStats* pFileStat);

private:
  bool m_bActive;
  ezSet<ezUuid> m_NeedsCheck;
  ezSet<ezUuid> m_NeedsTransform;
  ezSet<ezUuid> m_Done;

  ezHashTable<ezUuid, AssetInfo*> m_KnownAssets;
  ezMap<ezString, FileStatus, ezCompareString_NoCase<ezString> > m_ReferencedFiles;

  ezMap<ezString, FileStatus> m_FileHashingQueue;
  ezApplicationFileSystemConfig m_FileSystemConfig;
  ezString m_sActivePlatform;
  ezSet<ezString> m_ValidAssetExtensions;
  ezSet<ezString> m_AssetFolders;

private:
  friend class ezHashingTask;

  /// \brief Computes the hash of the given file. Optionally passes the data stream through into another stream writer.
  static ezUInt64 HashFile(ezStreamReaderBase& InputStream, ezStreamWriterBase* pPassThroughStream);

  /// \brief Opens the asset JSON file and reads the "Header" into the given ezAssetDocumentInfo.
  static void ReadAssetDocumentInfo(ezAssetDocumentInfo* pInfo, ezStreamReaderBase& stream);

  bool GetNextFileToHash(ezStringBuilder& sFile, FileStatus& status);
  void OnHashingTaskFinished(ezTask* pTask);
  ezResult WriteAssetTable(const char* szDataDirectory);

  void RunNextHashingTask();

  mutable ezMutex m_HashingMutex;
  ezHashingTask* m_pHashingTask;

private:
  static ezAssetCurator* s_pInstance;
};

class ezHashingTask : public ezTask
{
public:
  ezHashingTask();

  ezStringBuilder m_sFileToHash;
  ezResult m_Result;

  ezAssetCurator::FileStatus m_FileStatus;
  ezAssetCurator::AssetInfo m_AssetInfo;

private:
  virtual void Execute() override;
};