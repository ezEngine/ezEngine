#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Framework/AiBehaviorManager.h>
#include <AiPlugin/UtilityAI/Framework/AiConsiderationManager.h>
#include <AiPlugin/UtilityAI/Framework/AiControllerDesc.h>
#include <AiPlugin/UtilityAI/Framework/AiPerception.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezAiScoreCategory, 1)
  EZ_ENUM_CONSTANTS(ezAiScoreCategory::Fallback, ezAiScoreCategory::Idle, ezAiScoreCategory::ActiveIdle, ezAiScoreCategory::Investigate)
  EZ_ENUM_CONSTANTS(ezAiScoreCategory::Command, ezAiScoreCategory::Combat, ezAiScoreCategory::Interrupt)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiBehavior, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Considerations", m_Considerations)->AddAttributes(new ezAiInputPinAttribute("ezAiConsideration"), new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAiBehaviorScore ezAiBehavior::MakeScore(const ezAiContext& ctx, const ezAiPerception* pCandidate, float fValueToBeat) const
{
  ezAiBehaviorScore res;
  res.m_pPerception = pCandidate;

  const ezArrayPtr<const ezUInt32> considerations = m_Considerations.GetConnections();
  res.m_ConsiderationValues.SetCount(considerations.GetCount(), -1.0f);

  const float fCompensation = considerations.IsEmpty() ? 0.0f : 1.0f - 1.0f / considerations.GetCount();

  float fValue = m_fScore;
  for (ezUInt32 i = 0; i < considerations.GetCount(); ++i)
  {
    // the value only decreases with every consideration
    if (fValue <= 0.0f || fValue <= fValueToBeat)
      return res;

    const float v = ctx.m_ConsiderationManager.Evaluate(ctx, considerations[i], pCandidate);
    res.m_ConsiderationValues[i] = v;
    fValue *= v + (1.0f - v) * fCompensation * v;
  }

  if (fValue <= 0.0f || fValue <= fValueToBeat)
    return res;

  res.SetScore(m_Category, ezMath::Min(fValue, 1.0f));
  return res;
}

ezAiBehaviorManager::ezAiBehaviorManager() = default;
ezAiBehaviorManager::~ezAiBehaviorManager() = default;

void ezAiBehaviorManager::Initialize(const ezAiControllerDesc& desc, ezByteBlobPtr instanceData)
{
  m_pDesc = &desc;
  m_InstanceData = instanceData;
  m_uiActiveBehavior = ezInvalidIndex;
  m_States.Clear();
  m_States.SetCount(desc.GetBehaviors().GetCount());
}

void ezAiBehaviorManager::Deinitialize()
{
  m_pDesc = nullptr;
  m_InstanceData.Clear();
  m_uiActiveBehavior = ezInvalidIndex;
  m_States.Clear();
}

void ezAiBehaviorManager::DetermineAvailableBehaviors(ezTime currentTime, float fActiveBehaviorScore)
{
  m_CurrentTime = currentTime;
  ++m_uiUpdateCount;

  for (ezUInt32 i = 0; i < m_States.GetCount(); ++i)
  {
    auto& state = m_States[i];

    if (state.m_CooldownUntil > currentTime)
      continue;

    if (GetBehavior(i)->IsAvailable(GetInstanceData(i), fActiveBehaviorScore))
    {
      state.m_uiNeededInUpdate = m_uiUpdateCount;
    }
  }
}

void ezAiBehaviorManager::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager, const ezAiConsiderationManager& considerationManager)
{
  for (ezUInt32 i = 0; i < m_States.GetCount(); ++i)
  {
    if (m_States[i].m_uiNeededInUpdate == m_uiUpdateCount)
    {
      GetBehavior(i)->FlagNeededPerceptions(ref_PerceptionManager);
      considerationManager.FlagNeededPerceptions(GetBehavior(i)->m_Considerations, ref_PerceptionManager);
    }
  }
}

ezAiBehaviorCandidate ezAiBehaviorManager::DetermineBehaviorCandidate(const ezAiContext& ctx)
{
  ezAiBehaviorCandidate candidate;

  for (ezUInt32 i = 0; i < m_States.GetCount(); ++i)
  {
    auto& state = m_States[i];

    if (state.m_uiNeededInUpdate != m_uiUpdateCount)
      continue;

    const ezAiBehaviorScore scored = GetBehavior(i)->DetermineBehaviorScore(ctx, GetInstanceData(i));
    state.m_fLastScore = scored.GetScore();
    state.m_LastConsiderationValues = scored.m_ConsiderationValues;
    state.m_pLastCandidateType = scored.m_pPerception ? scored.m_pPerception->GetDynamicRTTI() : nullptr;
    state.m_bLastCandidateHasPosition = scored.m_pPerception && scored.m_pPerception->GetDebugPosition(state.m_vLastCandidatePosition);

    if (i == m_uiActiveBehavior)
    {
      candidate.m_fActiveBehaviorScore = scored.GetScore();
    }

    if (scored.GetScore() > candidate.m_fScore)
    {
      candidate.m_uiBehaviorIndex = i;
      candidate.m_pBehavior = GetBehavior(i);
      candidate.m_pPerception = scored.m_pPerception;
      candidate.m_fScore = scored.GetScore();
    }
  }

  return candidate;
}

void ezAiBehaviorManager::SetActiveBehavior(const ezAiContext& ctx, ezUInt32 uiBehaviorIndex, const ezAiPerception* pPerception)
{
  if (m_uiActiveBehavior == uiBehaviorIndex && pPerception == nullptr)
  {
    // do not deactivate and activate again, just keep calling ContinueBehavior()
    return;
  }

  if (m_uiActiveBehavior != ezInvalidIndex)
  {
    const ezAiBehavior* pBehavior = GetBehavior(m_uiActiveBehavior);
    void* pInstanceData = GetInstanceData(m_uiActiveBehavior);

    pBehavior->OnDeactivateBehavior(ctx, pInstanceData);

    if (m_uiActiveBehavior != uiBehaviorIndex)
    {
      m_States[m_uiActiveBehavior].m_CooldownUntil = ctx.m_Now + pBehavior->GetCooldownDuration(pInstanceData);
    }
  }

  m_uiActiveBehavior = uiBehaviorIndex;

  if (m_uiActiveBehavior != ezInvalidIndex)
  {
    GetBehavior(m_uiActiveBehavior)->OnActivateBehavior(ctx, GetInstanceData(m_uiActiveBehavior), pPerception);
  }
}

void ezAiBehaviorManager::KeepActiveBehavior(const ezAiContext& ctx, const ezAiPerception* pPerception)
{
  if (m_uiActiveBehavior != ezInvalidIndex)
  {
    GetBehavior(m_uiActiveBehavior)->OnReactivateBehavior(ctx, GetInstanceData(m_uiActiveBehavior), pPerception);
  }
}

ezAiBehavior::ContinuationState ezAiBehaviorManager::ContinueActiveBehavior(const ezAiContext& ctx)
{
  if (m_uiActiveBehavior != ezInvalidIndex)
  {
    return GetBehavior(m_uiActiveBehavior)->ContinueBehavior(ctx, GetInstanceData(m_uiActiveBehavior));
  }

  return {};
}

const ezAiBehavior* ezAiBehaviorManager::GetActiveBehavior() const
{
  return m_uiActiveBehavior != ezInvalidIndex ? GetBehavior(m_uiActiveBehavior) : nullptr;
}

void ezAiBehaviorManager::GetDebugText(ezStringBuilder& inout_sText) const
{
  for (ezUInt32 i = 0; i < m_States.GetCount(); ++i)
  {
    const auto& state = m_States[i];
    const ezStringView sName = GetBehavior(i)->GetName();
    const char* szPrefix = (i == m_uiActiveBehavior) ? "> " : "";

    if (state.m_uiNeededInUpdate != m_uiUpdateCount)
    {
      if (state.m_CooldownUntil > m_CurrentTime)
        inout_sText.AppendFormat("{}{}: cooldown {}s\n", szPrefix, sName, ezArgF((state.m_CooldownUntil - m_CurrentTime).GetSeconds(), 1));
      else
        inout_sText.AppendFormat("{}{}: n/a\n", szPrefix, sName);
    }
    else
    {
      inout_sText.AppendFormat("{}{}: {}", szPrefix, sName, ezArgF(state.m_fLastScore, 2));

      // e.g. 'Shoot: 5.65 (Range 1.00, Closer 0.81)', the results for the best candidate
      const ezArrayPtr<const ezUInt32> considerations = GetBehavior(i)->m_Considerations.GetConnections();
      if (!considerations.IsEmpty() && state.m_LastConsiderationValues.GetCount() == considerations.GetCount())
      {
        for (ezUInt32 c = 0; c < considerations.GetCount(); ++c)
        {
          const ezStringView sConsideration = m_pDesc->GetConsiderations()[considerations[c]].m_pConsideration->GetDisplayName();
          const float fValue = state.m_LastConsiderationValues[c];

          inout_sText.Append(c == 0 ? " (" : ", ");

          if (fValue < 0.0f)
            inout_sText.AppendFormat("{} -", sConsideration);
          else
            inout_sText.AppendFormat("{} {}", sConsideration, ezArgF(fValue, 2));
        }

        inout_sText.Append(")");
      }

      inout_sText.Append("\n");
    }
  }
}

const ezAiBehavior* ezAiBehaviorManager::GetBehavior(ezUInt32 uiIndex) const
{
  return m_pDesc->GetBehaviors()[uiIndex].m_pBehavior.Borrow();
}

void* ezAiBehaviorManager::GetInstanceData(ezUInt32 uiIndex) const
{
  return ezInstanceDataAllocator::GetInstanceData(m_InstanceData, m_pDesc->GetBehaviors()[uiIndex].m_uiInstanceDataOffset);
}
