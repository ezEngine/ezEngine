#include <PCH.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Algorithm/Hashing.h>
#include <Foundation/IO/MemoryStream.h>

ezHashingTask::ezHashingTask() : m_Result(EZ_FAILURE)
{
}

void ezHashingTask::Execute()
{
  m_Result = EZ_FAILURE;

  if (!ezAssetCurator::GetInstance()->GetNextFileToHash(m_sFileToHash, m_FileStatus))
  {
    m_Result = EZ_SUCCESS;
    return;
  }

  m_FileStatus.m_Status = ezAssetCurator::FileStatus::Status::FileLocked;
  m_FileStatus.m_uiHash = 0;
  m_FileStatus.m_Timestamp = ezTimestamp();

  if (m_FileStatus.m_AssetGuid.IsValid())
  {
    m_Result = ezAssetCurator::UpdateAssetInfo(m_sFileToHash, m_FileStatus, m_AssetInfo);
  }
  else
  {
    ezFileReader file;
    if (file.Open(m_sFileToHash).Failed())
      return;

    ezFileStats stat;
    if (ezOSFile::GetFileStats(file.GetFilePathAbsolute(), stat).Failed())
      return;

    m_FileStatus.m_Timestamp = stat.m_LastModificationTime;
    m_FileStatus.m_uiHash = ezAssetCurator::HashFile(file, nullptr);
    m_FileStatus.m_Status = ezAssetCurator::FileStatus::Status::Valid;
  }

  m_Result = EZ_SUCCESS;
}

ezUInt64 ezAssetCurator::HashFile(ezStreamReaderBase& InputStream, ezStreamWriterBase* pPassThroughStream)
{
  ezUInt8 uiCache[1024 * 10];
  ezUInt64 uiHash = 0;

  while (true)
  {
    ezUInt64 uiRead = InputStream.ReadBytes(uiCache, EZ_ARRAY_SIZE(uiCache));

    if (uiRead == 0)
      break;

    uiHash = ezHashing::MurmurHash64(uiCache, (size_t) uiRead, uiHash);

    if (pPassThroughStream != nullptr)
      pPassThroughStream->WriteBytes(uiCache, uiRead);
  }

  return uiHash;
}

void ezAssetCurator::QueueFileForHashing(const ezString& sFile)
{
  const auto& ref = m_ReferencedFiles[sFile];

  if (ref.m_uiHash != 0 || ref.m_Status == FileStatus::Status::FileLocked)
    return;

  m_FileHashingQueue[sFile] = ref;
}

bool ezAssetCurator::GetNextFileToHash(ezStringBuilder& sFile, FileStatus& status)
{
  ezLock<ezMutex> ml(m_HashingMutex);

  sFile.Clear();

  if (m_FileHashingQueue.IsEmpty())
    return false;

  auto it = m_FileHashingQueue.GetIterator();
  sFile = it.Key();
  status = it.Value();

  m_FileHashingQueue.Remove(it);

  return true;
}

void ezAssetCurator::OnHashingTaskFinished(ezTask* pTask)
{
  ezLock<ezMutex> ml(m_HashingMutex);

  if (m_bActive)
  {
    ezHashingTask* pHashTask = static_cast<ezHashingTask*>(pTask);

    if (!pHashTask->m_sFileToHash.IsEmpty())
    {
      auto& RefFile = m_ReferencedFiles[pHashTask->m_sFileToHash];

      if (pHashTask->m_Result.Succeeded())
      {
        if (RefFile.m_AssetGuid.IsValid() && RefFile.m_AssetGuid != pHashTask->m_FileStatus.m_AssetGuid)
        {
          m_KnownAssets[RefFile.m_AssetGuid]->m_State = AssetInfo::State::ToBeDeleted;

          auto& newAsset = m_KnownAssets[pHashTask->m_FileStatus.m_AssetGuid];
          if (newAsset == nullptr)
            newAsset = EZ_DEFAULT_NEW(ezAssetCurator::AssetInfo);

          *newAsset = pHashTask->m_AssetInfo;
        }
      }

      RefFile = pHashTask->m_FileStatus;
    }

  }

  RunNextHashingTask();
}

void ezAssetCurator::RunNextHashingTask()
{
  ezLock<ezMutex> ml(m_HashingMutex);

  if (!m_bActive || m_FileHashingQueue.IsEmpty())
    return;

  if (m_pHashingTask == nullptr)
  {
    m_pHashingTask = EZ_DEFAULT_NEW(ezHashingTask);
    m_pHashingTask->SetOnTaskFinished(ezMakeDelegate(&ezAssetCurator::OnHashingTaskFinished, this));
  }

  if (m_pHashingTask->IsTaskFinished())
    ezTaskSystem::StartSingleTask(m_pHashingTask, ezTaskPriority::FileAccess);
}


void ezAssetCurator::QueueFilesForHashing()
{
  ezLock<ezMutex> ml(m_HashingMutex);

  m_FileHashingQueue.Clear();

  for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
  {
    QueueFileForHashing(it.Value()->m_sAbsolutePath);

    for (const auto& dep : it.Value()->m_Info.m_FileDependencies)
    {
      QueueFileForHashing(dep);
    }
  }

  RunNextHashingTask();
}

ezUInt64 ezAssetCurator::GetAssetDependencyHash(ezUuid assetGuid)
{
  ezLock<ezMutex> ml(m_HashingMutex);

  ezAssetCurator::AssetInfo* pInfo = nullptr;
  if (!m_KnownAssets.TryGetValue(assetGuid, pInfo))
    return 0;

  FileStatus& status = m_ReferencedFiles[pInfo->m_sAbsolutePath];

  ezFileStats stat;

  {
    ezFileReader file;
    if (file.Open(pInfo->m_sAbsolutePath).Failed())
      return 0;

    if (ezOSFile::GetFileStats(file.GetFilePathAbsolute(), stat).Failed())
      return 0;
  }

  if (!stat.m_LastModificationTime.IsEqual(status.m_Timestamp, ezTimestamp::CompareMode::Identical))
  {
    // Update asset
    UpdateAssetInfo(pInfo->m_sAbsolutePath, status, *pInfo);
  }

  ezUInt64 uiHashResult = pInfo->m_Info.m_uiSettingsHash;

  // Iterate dependencies
  for (const auto& dep : pInfo->m_Info.m_FileDependencies)
  {
    ezString sPath = dep;

    if (!ezEditorApp::GetInstance()->MakeDataDirectoryRelativePathAbsolute(sPath))
      return 0;

    ezFileReader file;
    if (file.Open(sPath).Failed())
      return 0;

    if (ezOSFile::GetFileStats(file.GetFilePathAbsolute(), stat).Failed())
      return 0;

    auto& fileref = m_ReferencedFiles[dep];

    if (!fileref.m_Timestamp.IsEqual(stat.m_LastModificationTime, ezTimestamp::CompareMode::Identical))
    {
      // Update file status
      fileref.m_Timestamp = stat.m_LastModificationTime;
      fileref.m_uiHash = ezAssetCurator::HashFile(file, nullptr);
      fileref.m_Status = ezAssetCurator::FileStatus::Status::Valid;
    }

    uiHashResult += fileref.m_uiHash;
  }

  return uiHashResult;
}

