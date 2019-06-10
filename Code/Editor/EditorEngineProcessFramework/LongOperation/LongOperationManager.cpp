#include <EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <EditorEngineProcessFramework/LongOperation/LongOperationManager.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>

EZ_IMPLEMENT_SINGLETON(ezLongOpManager);

ezLongOpManager::ezLongOpManager(Mode mode)
  : m_SingletonRegistrar(this)
{
  m_Mode = mode;
}

ezLongOpManager::~ezLongOpManager() = default;

class ezLongOpTask : public ezTask
{
public:
  ezLongOpWorker* m_pLocalOp = nullptr;
  ezUuid m_OperationGuid;
  ezProgress* m_pProgress = nullptr;

  ezLongOpTask(ezLongOpWorker* pLocalOp, ezUuid OperationGuid)
  {
    m_pLocalOp = pLocalOp;
    m_OperationGuid = OperationGuid;
    SetOnTaskFinished([](ezTask* pThis) { EZ_DEFAULT_DELETE(pThis); });

    ezStringBuilder name;
    name.Format("Long Op: '{}'", m_pLocalOp->GetDisplayName());
    SetTaskName(name);
  }

  virtual void Execute() override
  {
    if (HasBeenCanceled())
      return;

    const ezResult res = m_pLocalOp->Execute(*m_pProgress);

    ezLongOpManager::GetSingleton()->WorkerOperationFinished(m_OperationGuid, res);
  }
};

// void ezLongOpManager::AddLongOperation(ezUniquePtr<ezLongOp>&& pOperation, const ezUuid& documentGuid)
//{
//  EZ_LOCK(m_Mutex);
//
//  auto& opInfoPtr = m_Operations.ExpandAndGetRef();
//  opInfoPtr = EZ_DEFAULT_NEW(LongOpInfo);
//
//  auto& opInfo = *opInfoPtr;
//  opInfo.m_pOperation = std::move(pOperation);
//  opInfo.m_OperationGuid.CreateNewUuid();
//  opInfo.m_DocumentGuid = documentGuid;
//  opInfo.m_StartOrDuration = ezTime::Now();
//  opInfo.m_Progress.m_pUserData = opInfo.m_pOperation.Borrow();
//  opInfo.m_Progress.m_Events.AddEventHandler(
//    ezMakeDelegate(&ezLongOpManager::ProgressBarEventHandler, this), opInfo.m_ProgressSubscription);
//
//  ezLongOp* pNewOp = opInfo.m_pOperation.Borrow();
//
//  if (m_Mode == Mode::Processor || ezDynamicCast<ezLongOpProxy*>(pNewOp) != nullptr)
//  {
//    ezStringBuilder replType;
//
//    ezLongOpReplicationMsg msg;
//
//    ezMemoryStreamContainerWrapperStorage<ezDataBuffer> storage(&msg.m_ReplicationData);
//    ezMemoryStreamWriter writer(&storage);
//
//    pNewOp->GetReplicationInfo(replType, writer);
//
//    msg.m_sReplicationType = replType;
//    msg.m_DocumentGuid = opInfo.m_DocumentGuid;
//    msg.m_OperationGuid = opInfo.m_OperationGuid;
//    msg.m_sDisplayName = pNewOp->GetDisplayName();
//
//    m_pCommunicationChannel->SendMessage(&msg);
//  }
//
//  LaunchWorkerOperation(opInfo);
//
//  {
//    ezLongOpManagerEvent e;
//    e.m_Type = ezLongOpManagerEvent::Type::OpAdded;
//    e.m_uiOperationIndex = m_Operations.GetCount() - 1;
//    m_Events.Broadcast(e);
//  }
//}

void ezLongOpManager::Startup(ezProcessCommunicationChannel* pCommunicationChannel)
{
  m_pCommunicationChannel = pCommunicationChannel;
  m_pCommunicationChannel->m_Events.AddEventHandler(
    ezMakeDelegate(&ezLongOpManager::ProcessCommunicationChannelEventHandler, this), m_Unsubscriber);
}

void ezLongOpManager::Shutdown()
{
  EZ_LOCK(m_Mutex);

  m_Operations.Clear();
  m_Unsubscriber.Unsubscribe();
  m_pCommunicationChannel = nullptr;
}

void ezLongOpManager::ProcessCommunicationChannelEventHandler(const ezProcessCommunicationChannel::Event& e)
{
  if (auto pMsg = ezDynamicCast<const ezLongOpReplicationMsg*>(e.m_pMessage))
  {
    EZ_LOCK(m_Mutex);

    ezRawMemoryStreamReader reader(pMsg->m_ReplicationData);
    const ezRTTI* pRtti = ezRTTI::FindTypeByName(pMsg->m_sReplicationType);

    auto& opInfoPtr = m_Operations.ExpandAndGetRef();
    opInfoPtr = EZ_DEFAULT_NEW(LongOpInfo);

    auto& opInfo = *opInfoPtr;
    opInfo.m_DocumentGuid = pMsg->m_DocumentGuid;
    opInfo.m_OperationGuid = pMsg->m_OperationGuid;
    opInfo.m_pOperation = pRtti->GetAllocator()->Allocate<ezLongOp>();
    opInfo.m_pOperation->InitializeReplicated(reader);

    if (m_Mode == Mode::Controller)
    {
      ezLongOpManagerEvent e;
      e.m_Type = ezLongOpManagerEvent::Type::OpAdded;
      e.m_OperationGuid = opInfo.m_OperationGuid;
      m_Events.Broadcast(e);
    }

    if (m_Mode == Mode::Processor)
    {
      LaunchWorkerOperation(opInfo);
    }

    return;
  }

  if (auto pMsg = ezDynamicCast<const ezLongOpProgressMsg*>(e.m_pMessage))
  {
    EZ_ASSERT_DEBUG(m_Mode == Mode::Controller, "This message can only be handled by the controller process.");

    EZ_LOCK(m_Mutex);

    if (auto pOpInfo = GetOperation(pMsg->m_OperationGuid))
    {
      pOpInfo->m_Progress.SetCompletion(pMsg->m_fCompletion);

      BroadcastProgress(*pOpInfo);
    }

    return;
  }

  if (auto pMsg = ezDynamicCast<const ezLongOpResultMsg*>(e.m_pMessage))
  {
    EZ_LOCK(m_Mutex);

    if (auto pOpInfo = GetOperation(pMsg->m_OperationGuid))
    {
      if (m_Mode == Mode::Processor)
      {
        EZ_ASSERT_DEBUG(pMsg->m_bSuccess == false, "Only Cancel messages are allowed to send to the processor");

        pOpInfo->m_Progress.UserClickedCancel();
      }

      if (m_Mode == Mode::Controller)
      {
        pOpInfo->m_bIsRunning = false;
        pOpInfo->m_StartOrDuration = ezTime::Now() - pOpInfo->m_StartOrDuration;
        pOpInfo->m_Progress.SetCompletion(0.0f);

        // TODO: show success/failure in UI
        BroadcastProgress(*pOpInfo);
      }
    }
  }
}

void ezLongOpManager::LaunchWorkerOperation(LongOpInfo& opInfo)
{
  auto pLocalOp = ezDynamicCast<ezLongOpWorker*>(opInfo.m_pOperation.Borrow());

  EZ_ASSERT_DEBUG(pLocalOp != nullptr, "This function can only be called for worker operations.");

  opInfo.m_bIsRunning = true;
  opInfo.m_StartOrDuration = ezTime::Now();
  opInfo.m_Progress.SetCompletion(0.0f);
  opInfo.m_Progress.m_pUserData = &opInfo;
  opInfo.m_Progress.m_Events.AddEventHandler(
    ezMakeDelegate(&ezLongOpManager::WorkerProgressBarEventHandler, this), opInfo.m_ProgressSubscription);

  BroadcastProgress(opInfo);

  pLocalOp->m_pManager = this;
  if (pLocalOp->InitializeExecution(opInfo.m_DocumentGuid).Failed())
  {
    WorkerOperationFinished(opInfo.m_OperationGuid, EZ_FAILURE);
  }
  else
  {
    ezLongOpTask* pTask = EZ_DEFAULT_NEW(ezLongOpTask, pLocalOp, opInfo.m_OperationGuid);
    pTask->m_pProgress = &opInfo.m_Progress;
    opInfo.m_TaskID = ezTaskSystem::StartSingleTask(pTask, ezTaskPriority::LongRunning);
  }
}

void ezLongOpManager::WorkerOperationFinished(ezUuid operationGuid, ezResult result)
{
  EZ_LOCK(m_Mutex);

  auto pOpInfo = GetOperation(operationGuid);

  if (pOpInfo == nullptr)
    return;

  // update the UI to 100%
  if (result.Succeeded())
  {
    pOpInfo->m_Progress.SetCompletion(1.0f);
    BroadcastProgress(*pOpInfo);
  }

  if (m_Mode == Mode::Processor)
  {
    // tell the controller about the result

    ezLongOpResultMsg msg;
    msg.m_OperationGuid = operationGuid;
    msg.m_bSuccess = result.Succeeded();

    m_pCommunicationChannel->SendMessage(&msg);
  }

  if (m_Mode == Mode::Controller)
  {
    pOpInfo->m_StartOrDuration = ezTime::Now() - pOpInfo->m_StartOrDuration;
  }

  RemoveOperation(operationGuid);
}

void ezLongOpManager::WorkerProgressBarEventHandler(const ezProgressEvent& e)
{
  if (e.m_Type == ezProgressEvent::Type::ProgressChanged)
  {
    auto pOpInfo = static_cast<LongOpInfo*>(e.m_pProgressbar->m_pUserData);

    BroadcastProgress(*pOpInfo);
  }
}

void ezLongOpManager::StartOperation(ezUuid opGuid)
{
  EZ_ASSERT_DEBUG(m_Mode == Mode::Controller, "This function can only be executed by the controller process.");

  EZ_LOCK(m_Mutex);

  auto pOpInfo = GetOperation(opGuid);

  if (pOpInfo == nullptr || pOpInfo->m_bIsRunning)
    return;

  pOpInfo->m_StartOrDuration = ezTime::Now();
  pOpInfo->m_bIsRunning = true;

  // TODO: do this distinction here ?
  if (ezDynamicCast<ezLongOpProxy*>(pOpInfo->m_pOperation.Borrow()))
  {
    ReplicateToOtherProcess(*pOpInfo);
  }
  else
  {
    LaunchWorkerOperation(*pOpInfo);
  }

  BroadcastProgress(*pOpInfo);
}

void ezLongOpManager::CancelOperation(ezUuid opGuid)
{
  EZ_ASSERT_DEBUG(m_Mode == Mode::Controller, "This function can only be executed by the controller process.");

  EZ_LOCK(m_Mutex);

  auto pOpInfo = GetOperation(opGuid);

  if (pOpInfo == nullptr || !pOpInfo->m_bIsRunning)
    return;

  if (ezDynamicCast<ezLongOpProxy*>(pOpInfo->m_pOperation.Borrow()))
  {
    // send a cancel message to the processor
    ezLongOpResultMsg msg;
    msg.m_OperationGuid = opGuid;
    msg.m_bSuccess = false;
    m_pCommunicationChannel->SendMessage(&msg);
  }
  else
  {
    // if it is a worker, cancel it through progress bar
    pOpInfo->m_Progress.UserClickedCancel();
  }
}

void ezLongOpManager::RemoveOperation(ezUuid opGuid)
{
  EZ_LOCK(m_Mutex);

  for (ezUInt32 i = 0; i < m_Operations.GetCount(); ++i)
  {
    if (m_Operations[i]->m_OperationGuid == opGuid)
    {
      m_Operations.RemoveAtAndSwap(i);

      if (m_Mode == Mode::Controller)
      {
        // broadcast the removal to the UI

        ezLongOpManagerEvent e;
        e.m_Type = ezLongOpManagerEvent::Type::OpRemoved;
        e.m_OperationGuid = opGuid;

        m_Events.Broadcast(e);
      }

      return;
    }
  }
}

void ezLongOpManager::RegisterLongOp(const ezUuid& documentGuid, const ezUuid& componentGuid, const char* szLongOpType)
{
  EZ_ASSERT_DEBUG(m_Mode == Mode::Controller, "This function can only be executed by the controller process.");

  auto& opInfoPtr = m_Operations.ExpandAndGetRef();
  opInfoPtr = EZ_DEFAULT_NEW(LongOpInfo);

  auto& opInfo = *opInfoPtr;
  opInfo.m_DocumentGuid = documentGuid;
  opInfo.m_ComponentGuid = componentGuid;
  opInfo.m_OperationGuid.CreateNewUuid();

  const ezRTTI* pRtti = ezRTTI::FindTypeByName(szLongOpType);
  opInfo.m_pOperation = pRtti->GetAllocator()->Allocate<ezLongOp>();
  opInfo.m_pOperation->InitializeRegistered(documentGuid, componentGuid);

  ezLongOpManagerEvent e;
  e.m_Type = ezLongOpManagerEvent::Type::OpAdded;
  e.m_OperationGuid = opInfo.m_OperationGuid;
  m_Events.Broadcast(e);
}

void ezLongOpManager::UnregisterLongOp(const ezUuid& documentGuid, const ezUuid& componentGuid, const char* szLongOpType)
{
  EZ_ASSERT_DEBUG(m_Mode == Mode::Controller, "This function can only be executed by the controller process.");

  for (ezUInt32 i = 0; i < m_Operations.GetCount(); ++i)
  {
    auto& opInfoPtr = m_Operations[i];

    if (opInfoPtr->m_ComponentGuid == componentGuid && opInfoPtr->m_DocumentGuid == documentGuid &&
        ezStringUtils::IsEqual(opInfoPtr->m_pOperation->GetDynamicRTTI()->GetTypeName(), szLongOpType))
    {
      RemoveOperation(opInfoPtr->m_OperationGuid);
      return;
    }
  }
}

ezLongOpManager::LongOpInfo* ezLongOpManager::GetOperation(const ezUuid& guid) const
{
  EZ_LOCK(m_Mutex);

  for (auto& opInfoPtr : m_Operations)
  {
    if (opInfoPtr->m_OperationGuid == guid)
      return opInfoPtr.Borrow();
  }

  return nullptr;
}

void ezLongOpManager::DocumentClosed(const ezUuid& documentGuid)
{
  EZ_ASSERT_DEBUG(m_Mode == Mode::Controller, "This function can only be executed by the controller process.");

  EZ_LOCK(m_Mutex);

  for (ezUInt32 i = m_Operations.GetCount(); i > 0; --i)
  {
    auto& op = m_Operations[i - 1];
    if (op->m_DocumentGuid == documentGuid)
    {
      op->m_Progress.UserClickedCancel();
    }
  }

  for (ezUInt32 i = m_Operations.GetCount(); i > 0; --i)
  {
    auto& op = m_Operations[i - 1];
    if (op->m_DocumentGuid == documentGuid)
    {
      // TODO: wait till canceled

      ezLongOpManagerEvent e;
      e.m_Type = ezLongOpManagerEvent::Type::OpRemoved;
      e.m_OperationGuid = m_Operations[i - 1]->m_OperationGuid;

      m_Operations.RemoveAtAndSwap(i - 1);

      m_Events.Broadcast(e);
    }
  }
}

void ezLongOpManager::ReplicateToOtherProcess(LongOpInfo& opInfo)
{
  EZ_LOCK(m_Mutex);

  // send the replication message
  {
    ezStringBuilder replType;

    ezLongOpReplicationMsg msg;

    ezMemoryStreamContainerWrapperStorage<ezDataBuffer> storage(&msg.m_ReplicationData);
    ezMemoryStreamWriter writer(&storage);

    opInfo.m_pOperation->GetReplicationInfo(replType, writer);

    msg.m_sReplicationType = replType;
    msg.m_DocumentGuid = opInfo.m_DocumentGuid;
    msg.m_OperationGuid = opInfo.m_OperationGuid;
    msg.m_sDisplayName = opInfo.m_pOperation->GetDisplayName();

    m_pCommunicationChannel->SendMessage(&msg);
  }
}

void ezLongOpManager::BroadcastProgress(LongOpInfo& opInfo)
{
  if (m_Mode == Mode::Controller)
  {
    // as controller, broadcast progress to the UI
    ezLongOpManagerEvent e;
    e.m_Type = ezLongOpManagerEvent::Type::OpProgress;
    e.m_OperationGuid = opInfo.m_OperationGuid;
    m_Events.Broadcast(e);
  }
  else // if (m_Mode == Mode::Processor)
  {
    // as processor send progress message to controller

    ezLongOpProgressMsg msg;
    msg.m_OperationGuid = opInfo.m_OperationGuid;
    msg.m_fCompletion = opInfo.m_Progress.GetCompletion();

    m_pCommunicationChannel->SendMessage(&msg);
  }
}
