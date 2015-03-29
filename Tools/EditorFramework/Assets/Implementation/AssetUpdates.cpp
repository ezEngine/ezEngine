#include <PCH.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/Threading/TaskSystem.h>

class ezHashingTask : public ezTask
{
public:

private:
  virtual void Execute() override
  {
    const ezString sFileToHash = ezAssetCurator::GetInstance()->GetNextFileToHash();

    if (sFileToHash.IsEmpty())
      return;


  }
};

void ezAssetCurator::QueueFileForHashing(const ezString& sFile)
{
  const auto& ref = m_ReferencedFiles[sFile];

  if (ref.m_uiHash != 0 || ref.m_Status == FileStatus::Status::FileLocked)
    return;

  m_FileHashingQueue.Insert(sFile);
}

ezString ezAssetCurator::GetNextFileToHash()
{
  ezLock<ezMutex> ml(m_HashingMutex);

  if (m_FileHashingQueue.IsEmpty())
    return ezString();
  
  auto it = m_FileHashingQueue.GetIterator();
  ezString ret = it.Key();
  m_FileHashingQueue.Remove(it);

  return ret;
}

void ezAssetCurator::OnHashingTaskFinished(ezTask* pTask, void* pPassThrough)
{
  ezAssetCurator::GetInstance()->RunNextHashingTask();
}

void ezAssetCurator::RunNextHashingTask()
{
  ezLock<ezMutex> ml(m_HashingMutex);

  if (m_FileHashingQueue.IsEmpty())
  {
    EZ_DEFAULT_DELETE(m_pHashingTask);
    return;
  }

  if (m_pHashingTask == nullptr)
  {
    m_pHashingTask = EZ_DEFAULT_NEW(ezHashingTask);
    m_pHashingTask->SetOnTaskFinished(OnHashingTaskFinished);
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
    QueueFileForHashing(it.Value()->m_sPath);

    for (const auto& dep : it.Value()->m_Info.m_FileDependencies)
    {
      QueueFileForHashing(dep);
    }
  }

  RunNextHashingTask();
}

