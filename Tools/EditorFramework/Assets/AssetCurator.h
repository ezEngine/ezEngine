#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/TaskSystem.h>

class ezHashingTask;
class ezTask;
class ezAssetDocumentManager;

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
    ezString m_sPath;
    ezAssetDocumentInfo m_Info;
  };

  void CheckFileSystem();
  ezResult WriteAssetTables();
  void MainThreadTick();

  const AssetInfo* GetAssetInfo(const ezUuid& assetGuid) const;
  const ezHashTable<ezUuid, AssetInfo*>& GetKnownAssets() const;
  ezUInt64 GetAssetDependencyHash(ezUuid assetGuid);

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
    AssetInfo* m_pInfo;
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
  void InitCurator();
  void DeInitCurator();
  void QueueFilesForHashing();
  void QueueFileForHashing(const ezString& sFile);

  static ezResult UpdateAssetInfo(const char* szAbsFilePath, ezAssetCurator::FileStatus& stat, ezAssetCurator::AssetInfo& assetInfo);

private:
  bool m_bActive;
  ezSet<ezUuid> m_NeedsCheck;
  ezSet<ezUuid> m_NeedsTransform;
  ezSet<ezUuid> m_Done;

  ezHashTable<ezUuid, AssetInfo*> m_KnownAssets;
  ezMap<ezString, FileStatus, ezCompareString_NoCase<ezString> > m_ReferencedFiles;

  ezMap<ezString, FileStatus> m_FileHashingQueue;

private:
  friend class ezHashingTask;

  static ezUInt64 HashFile(ezStreamReaderBase& InputStream, ezStreamWriterBase* pPassThroughStream);
  static void ReadAssetDocumentInfo(ezAssetDocumentInfo* pInfo, ezStreamReaderBase& stream);

  bool GetNextFileToHash(ezStringBuilder& sFile, FileStatus& status);
  void OnHashingTaskFinished(ezTask* pTask);

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