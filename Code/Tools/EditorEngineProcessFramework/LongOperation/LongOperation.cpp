#include <EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/LongOperation/LongOperation.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOperation, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOperationLocal, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOperationRemote, 1, ezRTTIDefaultAllocator<ezLongOperationRemote>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOperationLocal_Dummy, 1, ezRTTIDefaultAllocator<ezLongOperationLocal_Dummy>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOperationRemote_Dummy, 1, ezRTTIDefaultAllocator<ezLongOperationRemote_Dummy>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezLongOperationLocal_Dummy::Step()
{
  m_TimeTaken += ezClock::GetGlobalClock()->GetTimeDiff();
  m_fCompletation = ezMath::Clamp<float>(m_TimeTaken.GetSeconds() / m_Duration.GetSeconds(), 0.0f, 1.0f);
}

void ezLongOperationLocal_Dummy::GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description)
{
  out_sReplicationOpType = "ezLongOperationRemote";
}

void ezLongOperationLocal_Dummy::InitializeReplicated(ezStreamReader& description)
{
  description >> m_Duration;
}

void ezLongOperationRemote_Dummy::GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description)
{
  out_sReplicationOpType = "ezLongOperationLocal_Dummy";
  description << m_Duration;
}
