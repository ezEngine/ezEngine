#include <EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <EditorEngineProcessFramework/LongOps/LongOpWorkerManager.h>
#include <EditorEngineProcessFramework/LongOps/LongOps.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>

EZ_IMPLEMENT_SINGLETON(ezLongOpWorkerManager);

ezLongOpWorkerManager::ezLongOpWorkerManager()
  : m_SingletonRegistrar(this)
{
}

ezLongOpWorkerManager::~ezLongOpWorkerManager() = default;

class ezLongOpTask : public ezTask
{
public:
  ezLongOpWorker* m_pWorkerOp = nullptr;
  ezUuid m_OperationGuid;
  ezProgress* m_pProgress = nullptr;

  ezLongOpTask()
  {
    SetOnTaskFinished([](ezTask* pThis) { EZ_DEFAULT_DELETE(pThis); });

    ezStringBuilder name;
    name.Format("Long Op: '{}'", "TODO: NAME"); // TODO
    SetTaskName(name);
  }

  virtual void Execute() override
  {
    if (HasBeenCanceled())
      return;

    ezDataBuffer resultData;
    ezMemoryStreamContainerWrapperStorage<ezDataBuffer> storage(&resultData);
    ezMemoryStreamWriter writer(&storage);

    const ezResult res = m_pWorkerOp->Execute(*m_pProgress, writer);

    ezLongOpWorkerManager::GetSingleton()->WorkerOperationFinished(m_OperationGuid, res, std::move(resultData));
  }
};

void ezLongOpWorkerManager::ProcessCommunicationChannelEventHandler(const ezProcessCommunicationChannel::Event& e)
{
  if (auto pMsg = ezDynamicCast<const ezLongOpReplicationMsg*>(e.m_pMessage))
  {
    EZ_LOCK(m_Mutex);

    ezRawMemoryStreamReader reader(pMsg->m_ReplicationData);
    const ezRTTI* pRtti = ezRTTI::FindTypeByName(pMsg->m_sReplicationType);

    auto& opInfoPtr = m_WorkerOps.ExpandAndGetRef();
    opInfoPtr = EZ_DEFAULT_NEW(WorkerOpInfo);

    auto& opInfo = *opInfoPtr;
    opInfo.m_DocumentGuid = pMsg->m_DocumentGuid;
    opInfo.m_OperationGuid = pMsg->m_OperationGuid;
    opInfo.m_pWorkerOp = pRtti->GetAllocator()->Allocate<ezLongOpWorker>();

    LaunchWorkerOperation(opInfo, reader);
    return;
  }

  if (auto pMsg = ezDynamicCast<const ezLongOpResultMsg*>(e.m_pMessage))
  {
    EZ_LOCK(m_Mutex);

    if (auto pOpInfo = GetOperation(pMsg->m_OperationGuid))
    {
      EZ_ASSERT_DEBUG(pMsg->m_bSuccess == false, "Only Cancel messages are allowed to send to the processor");

      pOpInfo->m_Progress.UserClickedCancel();
    }

    return;
  }
}

void ezLongOpWorkerManager::LaunchWorkerOperation(WorkerOpInfo& opInfo, ezStreamReader& config)
{
  opInfo.m_Progress.SetCompletion(0.0f);
  opInfo.m_Progress.m_pUserData = &opInfo;
  opInfo.m_Progress.m_Events.AddEventHandler(
    ezMakeDelegate(&ezLongOpWorkerManager::WorkerProgressBarEventHandler, this), opInfo.m_ProgressSubscription);

  SendProgress(opInfo);

  if (opInfo.m_pWorkerOp->InitializeExecution(config, opInfo.m_DocumentGuid).Failed())
  {
    WorkerOperationFinished(opInfo.m_OperationGuid, EZ_FAILURE, ezDataBuffer());
  }
  else
  {
    ezLongOpTask* pTask = EZ_DEFAULT_NEW(ezLongOpTask);
    pTask->m_OperationGuid = opInfo.m_OperationGuid;
    pTask->m_pWorkerOp = opInfo.m_pWorkerOp.Borrow();
    pTask->m_pProgress = &opInfo.m_Progress;
    opInfo.m_TaskID = ezTaskSystem::StartSingleTask(pTask, ezTaskPriority::LongRunning);
  }
}

void ezLongOpWorkerManager::WorkerOperationFinished(ezUuid operationGuid, ezResult result, ezDataBuffer&& resultData)
{
  EZ_LOCK(m_Mutex);

  auto pOpInfo = GetOperation(operationGuid);

  if (pOpInfo == nullptr)
    return;

  // tell the controller about the result
  {
    ezLongOpResultMsg msg;
    msg.m_OperationGuid = operationGuid;
    msg.m_bSuccess = result.Succeeded();
    msg.m_ResultData = std::move(resultData);

    m_pCommunicationChannel->SendMessage(&msg);
  }

  RemoveOperation(operationGuid);
}

void ezLongOpWorkerManager::WorkerProgressBarEventHandler(const ezProgressEvent& e)
{
  if (e.m_Type == ezProgressEvent::Type::ProgressChanged)
  {
    auto pOpInfo = static_cast<WorkerOpInfo*>(e.m_pProgressbar->m_pUserData);

    SendProgress(*pOpInfo);
  }
}

void ezLongOpWorkerManager::RemoveOperation(ezUuid opGuid)
{
  EZ_LOCK(m_Mutex);

  for (ezUInt32 i = 0; i < m_WorkerOps.GetCount(); ++i)
  {
    if (m_WorkerOps[i]->m_OperationGuid == opGuid)
    {
      m_WorkerOps.RemoveAtAndSwap(i);
      return;
    }
  }
}

ezLongOpWorkerManager::WorkerOpInfo* ezLongOpWorkerManager::GetOperation(const ezUuid& guid) const
{
  EZ_LOCK(m_Mutex);

  for (auto& opInfoPtr : m_WorkerOps)
  {
    if (opInfoPtr->m_OperationGuid == guid)
      return opInfoPtr.Borrow();
  }

  return nullptr;
}

void ezLongOpWorkerManager::SendProgress(WorkerOpInfo& opInfo)
{
  ezLongOpProgressMsg msg;
  msg.m_OperationGuid = opInfo.m_OperationGuid;
  msg.m_fCompletion = opInfo.m_Progress.GetCompletion();

  m_pCommunicationChannel->SendMessage(&msg);
}
