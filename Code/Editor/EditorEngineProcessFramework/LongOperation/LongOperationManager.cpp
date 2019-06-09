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

    ezResult res = m_pLocalOp->Execute(*m_pProgress);

    // TODO: pass along result
    ezLongOpManager::GetSingleton()->FinishOperation(m_OperationGuid);
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
  // if (auto pMsg = ezDynamicCast<const ezLongOpReplicationMsg*>(e.m_pMessage))
  //{
  //  EZ_LOCK(m_Mutex);

  //  ezRawMemoryStreamReader reader(pMsg->m_ReplicationData);
  //  const ezRTTI* pRtti = ezRTTI::FindTypeByName(pMsg->m_sReplicationType);

  //  auto& opInfoPtr = m_Operations.ExpandAndGetRef();
  //  opInfoPtr = EZ_DEFAULT_NEW(LongOpInfo);

  //  auto& opInfo = *opInfoPtr;
  //  opInfo.m_pOperation = pRtti->GetAllocator()->Allocate<ezLongOp>();
  //  opInfo.m_StartOrDuration = ezTime::Now();
  //  opInfo.m_Progress.m_pUserData = opInfo.m_pOperation.Borrow();
  //  opInfo.m_Progress.m_Events.AddEventHandler(
  //    ezMakeDelegate(&ezLongOpManager::ProgressBarEventHandler, this), opInfo.m_ProgressSubscription);

  //  opInfo.m_DocumentGuid = pMsg->m_DocumentGuid;
  //  opInfo.m_OperationGuid = pMsg->m_OperationGuid;
  //  opInfo.m_pOperation->InitializeReplicated(reader);

  //  LaunchWorkerOperation(opInfo);

  //  {
  //    ezLongOpManagerEvent e;
  //    e.m_Type = ezLongOpManagerEvent::Type::OpAdded;
  //    e.m_uiOperationIndex = m_Operations.GetCount() - 1;
  //    m_Events.Broadcast(e);
  //  }
  //}
  // else if (auto pMsg = ezDynamicCast<const ezLongOpProgressMsg*>(e.m_pMessage))
  //{
  //  EZ_LOCK(m_Mutex);

  //  if (pMsg->m_fCompletion < 0.0f)
  //  {
  //    FinishOperation(pMsg->m_OperationGuid);
  //  }
  //  else
  //  {
  //    for (ezUInt32 i = 0; i < m_Operations.GetCount(); ++i)
  //    {
  //      auto& opInfo = *m_Operations[i];

  //      if (opInfo.m_OperationGuid == pMsg->m_OperationGuid)
  //      {
  //        opInfo.m_Progress.SetCompletion(pMsg->m_fCompletion);

  //        ezLongOpManagerEvent e;
  //        e.m_Type = ezLongOpManagerEvent::Type::OpProgress;
  //        e.m_uiOperationIndex = i;
  //        m_Events.Broadcast(e);

  //        return;
  //      }
  //    }
  //  }
  //}
  // else if (auto pMsg = ezDynamicCast<const ezLongOpFinishMsg*>(e.m_pMessage))
  //{
  //  if (pMsg->m_bSuccess == false)
  //  {
  //    for (ezUInt32 i = 0; i < m_Operations.GetCount(); ++i)
  //    {
  //      auto& opInfo = *m_Operations[i];

  //      if (opInfo.m_OperationGuid == pMsg->m_OperationGuid)
  //      {
  //        opInfo.m_Progress.UserClickedCancel();
  //        return;
  //      }
  //    }
  //  }
  //}
}

void ezLongOpManager::LaunchWorkerOperation(LongOpInfo& opInfo)
{
  // if (auto pLocalOp = ezDynamicCast<ezLongOpWorker*>(opInfo.m_pOperation.Borrow()))
  //{
  //  pLocalOp->m_pManager = this;
  //  if (pLocalOp->InitializeExecution(opInfo.m_DocumentGuid).Failed())
  //  {
  //    // TODO: return failure
  //    FinishOperation(opInfo.m_OperationGuid);
  //  }
  //  else
  //  {
  //    ezLongOpTask* pTask = EZ_DEFAULT_NEW(ezLongOpTask, pLocalOp, opInfo.m_OperationGuid);
  //    pTask->m_pProgress = &opInfo.m_Progress;
  //    opInfo.m_TaskID = ezTaskSystem::StartSingleTask(pTask, ezTaskPriority::LongRunning);
  //  }
  //}
}

void ezLongOpManager::SetCompletion(ezLongOp* pOperation, float fCompletion)
{
  // fCompletion = ezMath::Clamp(fCompletion, 0.0f, 1.0f);

  // EZ_LOCK(m_Mutex);

  // for (ezUInt32 i = 0; i < m_Operations.GetCount(); ++i)
  //{
  //  auto& opInfo = *m_Operations[i];

  //  if (opInfo.m_pOperation.Borrow() == pOperation)
  //  {
  //    {
  //      ezLongOpManagerEvent e;
  //      e.m_Type = ezLongOpManagerEvent::Type::OpProgress;
  //      e.m_uiOperationIndex = i;
  //      m_Events.Broadcast(e);
  //    }

  //    if (m_Mode == Mode::Processor)
  //    {
  //      if (auto* pLocalOp = ezDynamicCast<ezLongOpWorker*>(pOperation))
  //      {
  //        ezLongOpProgressMsg msg;
  //        msg.m_OperationGuid = opInfo.m_OperationGuid;
  //        msg.m_fCompletion = opInfo.m_Progress.GetCompletion();

  //        m_pCommunicationChannel->SendMessage(&msg);
  //      }
  //    }

  //    break;
  //  }
  //}
}

void ezLongOpManager::FinishOperation(ezUuid operationGuid)
{
  // EZ_LOCK(m_Mutex);

  // for (ezUInt32 i = 0; i < m_Operations.GetCount(); ++i)
  //{
  //  auto& opInfo = *m_Operations[i];

  //  if (opInfo.m_OperationGuid == operationGuid)
  //  {
  //    opInfo.m_Progress.SetCompletion(1.0f);
  //    opInfo.m_ProgressSubscription.Unsubscribe(); // we are done here
  //    opInfo.m_StartOrDuration = ezTime::Now() - opInfo.m_StartOrDuration;

  //    {
  //      ezLongOpManagerEvent e;
  //      e.m_Type = ezLongOpManagerEvent::Type::OpFinished;
  //      e.m_uiOperationIndex = i;
  //      m_Events.Broadcast(e);
  //    }

  //    if (m_Mode == Mode::Processor)
  //    {
  //      ezLongOpProgressMsg msg;
  //      msg.m_OperationGuid = opInfo.m_OperationGuid;
  //      msg.m_fCompletion = -1.0f;

  //      m_pCommunicationChannel->SendMessage(&msg);
  //    }

  //    opInfo.m_pOperation = nullptr;
  //    return;
  //  }
  //}
}

void ezLongOpManager::ProgressBarEventHandler(const ezProgressEvent& e)
{
  if (e.m_Type == ezProgressEvent::Type::ProgressChanged)
  {
    auto pOp = static_cast<ezLongOp*>(e.m_pProgressbar->m_pUserData);
    SetCompletion(pOp, e.m_pProgressbar->GetCompletion());
  }
  else if (e.m_Type == ezProgressEvent::Type::CancelClicked)
  {
    if (m_Mode == Mode::Controller)
    {
      if (auto* pProxyOp = ezDynamicCast<ezLongOpProxy*>(static_cast<ezLongOp*>(e.m_pProgressbar->m_pUserData)))
      {
        for (const auto& opInfo : m_Operations)
        {
          if (opInfo->m_pOperation.Borrow() == pProxyOp)
          {
            ezLongOpFinishMsg msg;
            msg.m_OperationGuid = opInfo->m_OperationGuid;
            msg.m_bSuccess = false;

            m_pCommunicationChannel->SendMessage(&msg);
            break;
          }
        }
      }
    }
  }
}

void ezLongOpManager::CancelOperation(ezUuid opGuid)
{
  EZ_LOCK(m_Mutex);

  if (auto pOpInfo = GetOperation(opGuid))
  {
    pOpInfo->m_Progress.UserClickedCancel();
  }
}

void ezLongOpManager::RegisterLongOp(const ezUuid& documentGuid, const ezUuid& componentGuid, const char* szLongOpType)
{
  auto& opInfoPtr = m_Operations.ExpandAndGetRef();
  opInfoPtr = EZ_DEFAULT_NEW(LongOpInfo);

  auto& opInfo = *opInfoPtr;
  opInfo.m_DocumentGuid = documentGuid;
  opInfo.m_ComponentGuid = componentGuid;
  opInfo.m_sLongOpType = szLongOpType;
  opInfo.m_OperationGuid.CreateNewUuid();

  const ezRTTI* pRtti = ezRTTI::FindTypeByName(szLongOpType);
  opInfo.m_pOperation = pRtti->GetAllocator()->Allocate<ezLongOp>();

  ezLongOpManagerEvent e;
  e.m_Type = ezLongOpManagerEvent::Type::OpAdded;
  e.m_OperationGuid = opInfo.m_OperationGuid;
  m_Events.Broadcast(e);
}

void ezLongOpManager::UnregisterLongOp(const ezUuid& documentGuid, const ezUuid& componentGuid, const char* szLongOpType)
{
  for (ezUInt32 i = 0; i < m_Operations.GetCount(); ++i)
  {
    auto& opInfoPtr = m_Operations[i];

    if (opInfoPtr->m_ComponentGuid == componentGuid && opInfoPtr->m_sLongOpType == szLongOpType &&
        opInfoPtr->m_DocumentGuid == documentGuid)
    {
      ezLongOpManagerEvent e;
      e.m_Type = ezLongOpManagerEvent::Type::OpRemoved;
      e.m_OperationGuid = m_Operations[i]->m_OperationGuid;

      m_Operations.RemoveAtAndCopy(i);

      m_Events.Broadcast(e);
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
