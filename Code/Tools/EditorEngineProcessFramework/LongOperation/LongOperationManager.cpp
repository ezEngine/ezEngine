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

class ezLongOpTask : public ezTask
{
public:
  ezLongOperationLocal* m_pLocalOp = nullptr;

  ezLongOpTask(ezLongOperationLocal* pLocalOp)
  {
    m_pLocalOp = pLocalOp;
    SetOnTaskFinished([](ezTask* pThis) { EZ_DEFAULT_DELETE(pThis); });

    ezStringBuilder name;
    name.Format("Long Op: '{}'", m_pLocalOp->GetDisplayName());
    SetTaskName(name);
  }

  virtual void Execute() override
  {
    if (HasBeenCanceled())
      return;

    m_pLocalOp->Execute(this);
  }
};

void ezLongOperationManager::AddLongOperation(ezUniquePtr<ezLongOperation>&& pOperation, const ezUuid& documentGuid)
{
  auto& opInfo = m_ActiveOperations.ExpandAndGetRef();
  opInfo.m_pOperation = std::move(pOperation);

  ezLongOperation* pNewOp = opInfo.m_pOperation.Borrow();
  pNewOp->m_OperationGuid.CreateNewUuid();
  pNewOp->m_DocumentGuid = documentGuid;

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

  LaunchLocalOperation(opInfo);
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
    ezRawMemoryStreamReader reader(pMsg->m_ReplicationData);
    const ezRTTI* pRtti = ezRTTI::FindTypeByName(pMsg->m_sReplicationType);

    auto& opInfo = m_ActiveOperations.ExpandAndGetRef();
    opInfo.m_pOperation = pRtti->GetAllocator()->Allocate<ezLongOperation>();

    opInfo.m_pOperation->m_DocumentGuid = pMsg->m_DocumentGuid;
    opInfo.m_pOperation->m_OperationGuid = pMsg->m_OperationGuid;
    opInfo.m_pOperation->InitializeReplicated(reader);

    LaunchLocalOperation(opInfo);
  }
}

void ezLongOperationManager::LaunchLocalOperation(LongOpInfo& opInfo)
{
  if (auto pLocalOp = ezDynamicCast<ezLongOperationLocal*>(opInfo.m_pOperation.Borrow()))
  {
    ezLongOpTask* pTask = EZ_DEFAULT_NEW(ezLongOpTask, pLocalOp);
    opInfo.m_TaskID = ezTaskSystem::StartSingleTask(pTask, ezTaskPriority::LongRunning);
  }
}
