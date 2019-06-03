#include <EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <EditorEngineProcessFramework/LongOperation/LongOperationManager.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>

EZ_IMPLEMENT_SINGLETON(ezLongOperationManager);

ezLongOperationManager::ezLongOperationManager(ReplicationMode mode)
  : m_SingletonRegistrar(this)
{
  m_ReplicationMode = mode;
}

ezLongOperationManager::~ezLongOperationManager() = default;

void ezLongOperationManager::AddLongOperation(ezUniquePtr<ezLongOperation>&& pOperation)
{
  m_ActiveOperations.ExpandAndGetRef();
  m_ActiveOperations.PeekBack() = std::move(pOperation);

  ezLongOperation* pNewOp = m_ActiveOperations.PeekBack().Borrow();

  if (m_ReplicationMode == ReplicationMode::AllOperations || ezDynamicCast<ezLongOperationRemote*>(pNewOp) != nullptr)
  {
    ezStringBuilder replType;

    ezReplicateLongOperationMsg msg;

    ezMemoryStreamContainerWrapperStorage<ezDataBuffer> storage(&msg.m_ReplicationData);
    ezMemoryStreamWriter writer(&storage);

    pNewOp->GetReplicationInfo(replType, writer);

    msg.m_sReplicationType = replType;
    msg.m_DocumentGuid = pNewOp->m_DocumentGuid;
    msg.m_OperationGuid = pNewOp->m_OperationGuid;
    msg.m_sDisplayName = pNewOp->GetDisplayName();

    m_pCommunicationChannel->SendMessage(&msg);
  }
}

const ezDynamicArray<ezUniquePtr<ezLongOperation>>& ezLongOperationManager::GetActiveOperations() const
{
  return m_ActiveOperations;
}

void ezLongOperationManager::Startup(ezProcessCommunicationChannel* pCommunicationChannel)
{
  m_pCommunicationChannel = pCommunicationChannel;
  m_pCommunicationChannel->m_Events.AddEventHandler(
    ezMakeDelegate(&ezLongOperationManager::ProcessCommunicationChannelEventHandler, this), m_Unsubscriber);
}

void ezLongOperationManager::Shutdown()
{
  m_ActiveOperations.Clear();
  m_Unsubscriber.Unsubscribe();
  m_pCommunicationChannel = nullptr;
}

void ezLongOperationManager::ProcessCommunicationChannelEventHandler(const ezProcessCommunicationChannel::Event& e)
{
  if (auto pMsg = ezDynamicCast<const ezReplicateLongOperationMsg*>(e.m_pMessage))
  {
    const ezRTTI* pRtti = ezRTTI::FindTypeByName(pMsg->m_sReplicationType);
    ezLongOperation* pOp = pRtti->GetAllocator()->Allocate<ezLongOperation>();

    pOp->m_DocumentGuid = pMsg->m_DocumentGuid;
    pOp->m_OperationGuid = pMsg->m_OperationGuid;

    ezRawMemoryStreamReader reader(pMsg->m_ReplicationData);

    pOp->InitializeReplicated(reader);
  }
}
