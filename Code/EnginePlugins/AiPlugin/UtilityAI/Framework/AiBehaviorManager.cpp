#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Framework/AiBehaviorManager.h>

ezAiBehaviorManager::ezAiBehaviorManager() = default;
ezAiBehaviorManager::~ezAiBehaviorManager() = default;

void ezAiBehaviorManager::DetermineAvailableBehaviors(ezTime currentTime, float fActiveBehaviorScore)
{
  m_CurrentTime = currentTime;
  ++m_uiUpdateCount;

  for (auto& info : m_Behaviors)
  {
    if (info.m_CooldownUntil > currentTime)
      continue;

    if (info.m_pBehavior->IsAvailable(fActiveBehaviorScore))
    {
      info.m_uiNeededInUpdate = m_uiUpdateCount;
    }
  }
}

void ezAiBehaviorManager::AddBehavior(ezUniquePtr<ezAiBehavior>&& pBehavior)
{
  auto& info = m_Behaviors.ExpandAndGetRef();
  info.m_pBehavior = std::move(pBehavior);
}

void ezAiBehaviorManager::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager)
{
  for (auto& info : m_Behaviors)
  {
    if (info.m_uiNeededInUpdate == m_uiUpdateCount)
    {
      info.m_pBehavior->FlagNeededPerceptions(ref_PerceptionManager);
    }
  }
}

ezAiBehaviorCandidate ezAiBehaviorManager::DetermineBehaviorCandidate(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager)
{
  ezAiBehaviorScore res;
  ezAiBehaviorCandidate candidate;

  for (auto& info : m_Behaviors)
  {
    if (info.m_uiNeededInUpdate == m_uiUpdateCount)
    {
      const ezAiBehaviorScore scored = info.m_pBehavior->DetermineBehaviorScore(owner, perceptionManager);

      if (scored.GetScore() > res.GetScore())
      {
        res = scored;
        candidate.m_pBehavior = info.m_pBehavior.Borrow();
        candidate.m_pPerception = scored.m_pPerception;
        candidate.m_fScore = res.GetScore();
      }
    }
  }

  return candidate;
}

void ezAiBehaviorManager::SetActiveBehavior(ezGameObject& owner, ezAiBehavior* pBehavior, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue)
{
  if (m_pActiveBehavior == pBehavior && pPerception == nullptr)
  {
    // do not deactivate and activate again, just keep calling ContinueBehavior()
    return;
  }

  if (m_pActiveBehavior)
  {
    m_pActiveBehavior->DeactivateBehavior(owner, inout_ActionQueue);
  }

  m_pActiveBehavior = pBehavior;

  if (m_pActiveBehavior)
  {
    m_pActiveBehavior->ActivateBehavior(owner, pPerception, inout_ActionQueue);

    for (auto& info : m_Behaviors)
    {
      if (info.m_pBehavior == m_pActiveBehavior)
      {
        info.m_CooldownUntil = m_CurrentTime + m_pActiveBehavior->GetCooldownDuration();
        break;
      }
    }
  }
}

void ezAiBehaviorManager::KeepActiveBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue)
{
  if (m_pActiveBehavior)
  {
    m_pActiveBehavior->ReactivateBehavior(owner, pPerception, inout_ActionQueue);
  }
}

ezAiBehavior::ContinuationState ezAiBehaviorManager::ContinueActiveBehavior(ezGameObject& owner, ezAiActionQueue& inout_ActionQueue)
{
  if (m_pActiveBehavior)
  {
    return m_pActiveBehavior->ContinueBehavior(owner, inout_ActionQueue);
  }

  return {};
}
