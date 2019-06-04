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
  ezUuid m_OperationGuid;

  ezLongOpTask(ezLongOperationLocal* pLocalOp, ezUuid OperationGuid)
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

    m_pLocalOp->Execute(this);

    ezLongOperationManager::GetSingleton()->FinishOperation(m_OperationGuid);
  }
};

void ezLongOperationManager::AddLongOperation(ezUniquePtr<ezLongOperation>&& pOperation, const ezUuid& documentGuid)
{
  EZ_LOCK(m_Mutex);

  auto& opInfo = m_Operations.ExpandAndGetRef();
  opInfo.m_pOperation = std::move(pOperation);
  opInfo.m_OperationGuid.CreateNewUuid();
  opInfo.m_DocumentGuid = documentGuid;

  ezLongOperation* pNewOp = opInfo.m_pOperation.Borrow();

  if (m_ReplicationMode == ReplicationMode::AllOperations || ezDynamicCast<ezLongOperationRemote*>(pNewOp) != nullptr)
  {
    ezStringBuilder replType;

    ezLongOperationReplicationMsg msg;

    ezMemoryStreamContainerWrapperStorage<ezDataBuffer> storage(&msg.m_ReplicationData);
    ezMemoryStreamWriter writer(&storage);

    pNewOp->GetReplicationInfo(replType, writer);

    msg.m_sReplicationType = replType;
    msg.m_DocumentGuid = opInfo.m_DocumentGuid;
    msg.m_OperationGuid = opInfo.m_OperationGuid;
    msg.m_sDisplayName = pNewOp->GetDisplayName();

    m_pCommunicationChannel->SendMessage(&msg);
  }

  LaunchLocalOperation(opInfo);

  {
    ezLongOperationManagerEvent e;
    e.m_Type = ezLongOperationManagerEvent::Type::OpAdded;
    e.m_uiOperationIndex = m_Operations.GetCount() - 1;
    m_Events.Broadcast(e);
  }
}

void ezLongOperationManager::Startup(ezProcessCommunicationChannel* pCommunicationChannel)
{
  m_pCommunicationChannel = pCommunicationChannel;
  m_pCommunicationChannel->m_Events.AddEventHandler(
    ezMakeDelegate(&ezLongOperationManager::ProcessCommunicationChannelEventHandler, this), m_Unsubscriber);
}

void ezLongOperationManager::Shutdown()
{
  EZ_LOCK(m_Mutex);

  m_Operations.Clear();
  m_Unsubscriber.Unsubscribe();
  m_pCommunicationChannel = nullptr;
}

void ezLongOperationManager::ProcessCommunicationChannelEventHandler(const ezProcessCommunicationChannel::Event& e)
{
  if (auto pMsg = ezDynamicCast<const ezLongOperationReplicationMsg*>(e.m_pMessage))
  {
    EZ_LOCK(m_Mutex);

    ezRawMemoryStreamReader reader(pMsg->m_ReplicationData);
    const ezRTTI* pRtti = ezRTTI::FindTypeByName(pMsg->m_sReplicationType);

    auto& opInfo = m_Operations.ExpandAndGetRef();
    opInfo.m_pOperation = pRtti->GetAllocator()->Allocate<ezLongOperation>();

    opInfo.m_DocumentGuid = pMsg->m_DocumentGuid;
    opInfo.m_OperationGuid = pMsg->m_OperationGuid;
    opInfo.m_pOperation->InitializeReplicated(reader);

    LaunchLocalOperation(opInfo);

    {
      ezLongOperationManagerEvent e;
      e.m_Type = ezLongOperationManagerEvent::Type::OpAdded;
      e.m_uiOperationIndex = m_Operations.GetCount() - 1;
      m_Events.Broadcast(e);
    }
  }
  else if (auto pMsg = ezDynamicCast<const ezLongOperationProgressMsg*>(e.m_pMessage))
  {
    EZ_LOCK(m_Mutex);

    if (pMsg->m_fCompletion < 0.0f)
    {
      FinishOperation(pMsg->m_OperationGuid);
    }
    else
    {
      for (ezUInt32 i = 0; i < m_Operations.GetCount(); ++i)
      {
        auto& opInfo = m_Operations[i];

        if (opInfo.m_OperationGuid == pMsg->m_OperationGuid)
        {
          opInfo.m_fCompletion = pMsg->m_fCompletion;

          ezLongOperationManagerEvent e;
          e.m_Type = ezLongOperationManagerEvent::Type::OpProgress;
          e.m_uiOperationIndex = i;
          m_Events.Broadcast(e);

          return;
        }
      }
    }
  }
}

void ezLongOperationManager::LaunchLocalOperation(LongOpInfo& opInfo)
{
  if (auto pLocalOp = ezDynamicCast<ezLongOperationLocal*>(opInfo.m_pOperation.Borrow()))
  {
    pLocalOp->m_pManager = this;

    ezLongOpTask* pTask = EZ_DEFAULT_NEW(ezLongOpTask, pLocalOp, opInfo.m_OperationGuid);
    opInfo.m_TaskID = ezTaskSystem::StartSingleTask(pTask, ezTaskPriority::LongRunning);
  }
}

void ezLongOperationManager::SetCompletion(ezLongOperation* pOperation, float fCompletion)
{
  fCompletion = ezMath::Clamp(fCompletion, 0.0f, 1.0f);

  EZ_LOCK(m_Mutex);

  for (ezUInt32 i = 0; i < m_Operations.GetCount(); ++i)
  {
    auto& opInfo = m_Operations[i];

    if (opInfo.m_pOperation.Borrow() == pOperation)
    {
      if (opInfo.m_fCompletion != fCompletion)
      {
        opInfo.m_fCompletion = fCompletion;

        {
          ezLongOperationManagerEvent e;
          e.m_Type = ezLongOperationManagerEvent::Type::OpProgress;
          e.m_uiOperationIndex = i;
          m_Events.Broadcast(e);
        }

        if (m_ReplicationMode == ReplicationMode::AllOperations)
        {
          if (auto* pLocalOp = ezDynamicCast<ezLongOperationLocal*>(pOperation))
          {
            ezLongOperationProgressMsg msg;
            msg.m_OperationGuid = opInfo.m_OperationGuid;
            msg.m_fCompletion = opInfo.m_fCompletion;

            m_pCommunicationChannel->SendMessage(&msg);
          }
        }
      }

      break;
    }
  }
}

void ezLongOperationManager::FinishOperation(ezUuid operationGuid)
{
  EZ_LOCK(m_Mutex);

  for (ezUInt32 i = 0; i < m_Operations.GetCount(); ++i)
  {
    auto& opInfo = m_Operations[i];

    if (opInfo.m_OperationGuid == operationGuid)
    {
      opInfo.m_fCompletion = 1.0f;

      {
        ezLongOperationManagerEvent e;
        e.m_Type = ezLongOperationManagerEvent::Type::OpFinished;
        e.m_uiOperationIndex = i;
        m_Events.Broadcast(e);
      }

      if (m_ReplicationMode == ReplicationMode::AllOperations)
      {
        ezLongOperationProgressMsg msg;
        msg.m_OperationGuid = opInfo.m_OperationGuid;
        msg.m_fCompletion = -1.0f;

        m_pCommunicationChannel->SendMessage(&msg);
      }

      opInfo.m_pOperation = nullptr;
      return;
    }
  }
}
