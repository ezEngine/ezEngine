#include <EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <EditorEngineProcessFramework/LongOps/LongOpControllerManager.h>
#include <EditorEngineProcessFramework/LongOps/LongOps.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>

EZ_IMPLEMENT_SINGLETON(ezLongOpControllerManager);

ezLongOpControllerManager::ezLongOpControllerManager()
  : m_SingletonRegistrar(this)
{
}

ezLongOpControllerManager::~ezLongOpControllerManager() = default;

void ezLongOpControllerManager::ProcessCommunicationChannelEventHandler(const ezProcessCommunicationChannel::Event& e)
{
  if (auto pMsg = ezDynamicCast<const ezLongOpProgressMsg*>(e.m_pMessage))
  {
    EZ_LOCK(m_Mutex);

    if (auto pOpInfo = GetOperation(pMsg->m_OperationGuid))
    {
      pOpInfo->m_fCompletion = pMsg->m_fCompletion;

      BroadcastProgress(*pOpInfo);
    }

    return;
  }

  if (auto pMsg = ezDynamicCast<const ezLongOpResultMsg*>(e.m_pMessage))
  {
    EZ_LOCK(m_Mutex);

    if (auto pOpInfo = GetOperation(pMsg->m_OperationGuid))
    {
      pOpInfo->m_bIsRunning = false;
      pOpInfo->m_StartOrDuration = ezTime::Now() - pOpInfo->m_StartOrDuration;
      pOpInfo->m_fCompletion = 0.0f;

      pOpInfo->m_pProxyOp->Finalize(pMsg->m_bSuccess ? EZ_SUCCESS : EZ_FAILURE, pMsg->m_ResultData);

      // TODO: show success/failure in UI
      BroadcastProgress(*pOpInfo);
    }
  }
}

void ezLongOpControllerManager::StartOperation(ezUuid opGuid)
{
  EZ_LOCK(m_Mutex);

  auto pOpInfo = GetOperation(opGuid);

  if (pOpInfo == nullptr || pOpInfo->m_bIsRunning)
    return;

  pOpInfo->m_StartOrDuration = ezTime::Now();
  pOpInfo->m_bIsRunning = true;

  ReplicateToWorkerProcess(*pOpInfo);

  BroadcastProgress(*pOpInfo);
}

void ezLongOpControllerManager::CancelOperation(ezUuid opGuid)
{
  EZ_LOCK(m_Mutex);

  auto pOpInfo = GetOperation(opGuid);

  if (pOpInfo == nullptr || !pOpInfo->m_bIsRunning)
    return;

  // send a cancel message to the processor
  ezLongOpResultMsg msg;
  msg.m_OperationGuid = opGuid;
  msg.m_bSuccess = false;
  m_pCommunicationChannel->SendMessage(&msg);
}

void ezLongOpControllerManager::RemoveOperation(ezUuid opGuid)
{
  EZ_LOCK(m_Mutex);

  for (ezUInt32 i = 0; i < m_ProxyOps.GetCount(); ++i)
  {
    if (m_ProxyOps[i]->m_OperationGuid == opGuid)
    {
      m_ProxyOps.RemoveAtAndCopy(i);

      // broadcast the removal to the UI
      {
        ezLongOpControllerEvent e;
        e.m_Type = ezLongOpControllerEvent::Type::OpRemoved;
        e.m_OperationGuid = opGuid;

        m_Events.Broadcast(e);
      }

      return;
    }
  }
}

void ezLongOpControllerManager::RegisterLongOp(const ezUuid& documentGuid, const ezUuid& componentGuid, const char* szLongOpType)
{
  auto& opInfoPtr = m_ProxyOps.ExpandAndGetRef();
  opInfoPtr = EZ_DEFAULT_NEW(ProxyOpInfo);

  auto& opInfo = *opInfoPtr;
  opInfo.m_DocumentGuid = documentGuid;
  opInfo.m_ComponentGuid = componentGuid;
  opInfo.m_OperationGuid.CreateNewUuid();

  const ezRTTI* pRtti = ezRTTI::FindTypeByName(szLongOpType);
  opInfo.m_pProxyOp = pRtti->GetAllocator()->Allocate<ezLongOpProxy>();
  opInfo.m_pProxyOp->InitializeRegistered(documentGuid, componentGuid);

  ezLongOpControllerEvent e;
  e.m_Type = ezLongOpControllerEvent::Type::OpAdded;
  e.m_OperationGuid = opInfo.m_OperationGuid;
  m_Events.Broadcast(e);
}

void ezLongOpControllerManager::UnregisterLongOp(const ezUuid& documentGuid, const ezUuid& componentGuid, const char* szLongOpType)
{
  for (ezUInt32 i = 0; i < m_ProxyOps.GetCount(); ++i)
  {
    auto& opInfoPtr = m_ProxyOps[i];

    if (opInfoPtr->m_ComponentGuid == componentGuid && opInfoPtr->m_DocumentGuid == documentGuid &&
        ezStringUtils::IsEqual(opInfoPtr->m_pProxyOp->GetDynamicRTTI()->GetTypeName(), szLongOpType))
    {
      RemoveOperation(opInfoPtr->m_OperationGuid);
      return;
    }
  }
}

ezLongOpControllerManager::ProxyOpInfo* ezLongOpControllerManager::GetOperation(const ezUuid& guid)
{
  EZ_LOCK(m_Mutex);

  for (auto& opInfoPtr : m_ProxyOps)
  {
    if (opInfoPtr->m_OperationGuid == guid)
      return opInfoPtr.Borrow();
  }

  return nullptr;
}

void ezLongOpControllerManager::CancelAndRemoveAllOpsForDocument(const ezUuid& documentGuid)
{
  {
    EZ_LOCK(m_Mutex);

    for (auto& opInfoPtr : m_ProxyOps)
    {
      CancelOperation(opInfoPtr->m_OperationGuid);
    }
  }

  bool bOperationsStillActive = true;

  while (bOperationsStillActive)
  {
    bOperationsStillActive = false;
    m_pCommunicationChannel->ProcessMessages();

    {
      EZ_LOCK(m_Mutex);

      for (ezUInt32 i0 = m_ProxyOps.GetCount(); i0 > 0; --i0)
      {
        const ezUInt32 i = i0 - 1;

        auto& op = m_ProxyOps[i];
        if (op->m_DocumentGuid == documentGuid)
        {
          if (op->m_bIsRunning)
          {
            bOperationsStillActive = true;
            break;
          }

          ezLongOpControllerEvent e;
          e.m_Type = ezLongOpControllerEvent::Type::OpRemoved;
          e.m_OperationGuid = m_ProxyOps[i]->m_OperationGuid;

          m_ProxyOps.RemoveAtAndCopy(i);

          m_Events.Broadcast(e);
        }
      }
    }

    if (bOperationsStillActive)
    {
      ezThreadUtils::Sleep(ezTime::Milliseconds(100));
    }
  }
}

void ezLongOpControllerManager::ReplicateToWorkerProcess(ProxyOpInfo& opInfo)
{
  EZ_LOCK(m_Mutex);

  // send the replication message
  {
    ezLongOpReplicationMsg msg;

    ezMemoryStreamContainerWrapperStorage<ezDataBuffer> storage(&msg.m_ReplicationData);
    ezMemoryStreamWriter writer(&storage);

    ezStringBuilder replType;
    opInfo.m_pProxyOp->GetReplicationInfo(replType, writer);

    msg.m_sReplicationType = replType;
    msg.m_DocumentGuid = opInfo.m_DocumentGuid;
    msg.m_OperationGuid = opInfo.m_OperationGuid;

    m_pCommunicationChannel->SendMessage(&msg);
  }
}

void ezLongOpControllerManager::BroadcastProgress(ProxyOpInfo& opInfo)
{
  // as controller, broadcast progress to the UI
  ezLongOpControllerEvent e;
  e.m_Type = ezLongOpControllerEvent::Type::OpProgress;
  e.m_OperationGuid = opInfo.m_OperationGuid;
  m_Events.Broadcast(e);
}


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
