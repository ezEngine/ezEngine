#include <EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/LongOperation/LongOperation.h>
#include <EditorEngineProcessFramework/LongOperation/LongOperationManager.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOp, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOpWorker, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOpProxy, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOpProxy_Simple, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOpProxyReplicant, 1, ezRTTIDefaultAllocator<ezLongOpProxyReplicant>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOpWorker_Dummy, 1, ezRTTIDefaultAllocator<ezLongOpWorker_Dummy>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOpProxy_Dummy, 1, ezRTTIDefaultAllocator<ezLongOpProxy_Dummy>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezLongOpWorker::GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description)
{
  out_sReplicationOpType = "ezLongOpProxyReplicant";
  description << GetDisplayName();
}

void ezLongOpWorker::SetCompletion(float fCompletion)
{
  m_pManager->SetCompletion(this, fCompletion);
}

//////////////////////////////////////////////////////////////////////////

ezLongOpProxy_Simple::ezLongOpProxy_Simple(const char* szDisplayName, const char* szRecplicationOpType)
{
  m_sDisplayName = szDisplayName;
  m_szRecplicationOpType = szRecplicationOpType;
}

const char* ezLongOpProxy_Simple::GetDisplayName() const
{
  return m_sDisplayName;
}

void ezLongOpProxy_Simple::GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description)
{
  EZ_ASSERT_DEBUG(!ezStringUtils::IsNullOrEmpty(m_szRecplicationOpType), "Invalid long op type to replicate");
  out_sReplicationOpType = m_szRecplicationOpType;

  //description << 
}

//////////////////////////////////////////////////////////////////////////

void ezLongOpProxyReplicant::InitializeReplicated(ezStreamReader& description)
{
  description >> m_sDisplayName;
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Time/Stopwatch.h>

void ezLongOpWorker_Dummy::Execute(const ezTask* pExecutingTask)
{
  ezStopwatch sw;

  float fCompletion = 0.0f;

  while (fCompletion < 1.0f && !pExecutingTask->HasBeenCanceled())
  {
    m_TimeTaken = sw.GetRunningTotal();
    fCompletion = ezMath::Clamp<float>(m_TimeTaken.GetSeconds() / m_Duration.GetSeconds(), 0.0f, 1.0f);

    ezThreadUtils::Sleep(ezTime::Milliseconds(200));

    ezLog::Info("{} completion: {}%%", GetDisplayName(), fCompletion * 100.0f);

    SetCompletion(fCompletion);
  }
}

void ezLongOpWorker_Dummy::InitializeReplicated(ezStreamReader& description)
{
  description >> m_Duration;
}

void ezLongOpProxy_Dummy::GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description)
{
  out_sReplicationOpType = "ezLongOpWorker_Dummy";
  description << m_Duration;
}
