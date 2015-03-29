#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Threading/Mutex.h>

class ezHashingTask;
class ezTask;

class EZ_EDITORFRAMEWORK_DLL ezAssetCurator
{
public:
  ezAssetCurator();
  ~ezAssetCurator();
  static ezAssetCurator* GetInstance() { return s_pInstance; }

  struct AssetInfoCache
  {
    AssetInfoCache() : m_pManager(nullptr) {} 

    ezDocumentManagerBase* m_pManager;
    ezString m_sPath;
    ezAssetDocumentInfo m_Info;
  };

  void CheckFileSystem();
  void WriteAssetTables();

  const AssetInfoCache* GetAssetInfo(const ezUuid& assetGuid) const;
  const ezHashTable<ezUuid, AssetInfoCache*>& GetKnownAssets() const;

  // TODO: Background filesystem watcher and main thread update tick to add background info to main data
  // TODO: Hash changed in different thread

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
    AssetInfoCache* m_pInfo;
    Type m_Type;
  };

  ezEvent<const Event&> m_Events;


private:
  struct FileStatus
  {
    enum class Status
    {
      Unknown,
      FileLocked,
      Valid
    };

    FileStatus()
    {
      m_uiHash = 0;
      m_Status = Status::Unknown;
    }

    ezTimestamp m_Timestamp;
    ezUInt64 m_uiHash;
    ezUuid m_AssetGuid;
    Status m_Status;
  };

  void DocumentManagerEventHandler(const ezDocumentManagerBase::Event& r);
  void ProjectEventHandler(const ezToolsProject::Event& e);
  static void BuildFileExtensionSet(ezSet<ezString>& AllExtensions);
  void IterateDataDirectory(const char* szDataDir, const ezSet<ezString>& validExtensions);
  void SetAllAssetStatusUnknown();
  void RemoveStaleFileInfos();
  void ClearCache();
  void QueueFilesForHashing();
  void QueueFileForHashing(const ezString& sFile);

  AssetInfoCache* UpdateAssetInfo(const char* szAbsFilePath, FileStatus& stat);

private:
  ezSet<ezUuid> m_NeedsCheck;
  ezSet<ezUuid> m_NeedsTransform;
  ezSet<ezUuid> m_Done;

  ezHashTable<ezUuid, AssetInfoCache*> m_KnownAssets;
  ezMap<ezString, FileStatus> m_ReferencedFiles;
  ezSet<ezString> m_FileHashingQueue;

private:
  friend class ezHashingTask;

  ezString GetNextFileToHash();
  static void OnHashingTaskFinished(ezTask* pTask, void* pPassThrough);
  void RunNextHashingTask();

  ezMutex m_HashingMutex;
  ezHashingTask* m_pHashingTask;

private:
  static ezAssetCurator* s_pInstance;
};
