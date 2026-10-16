#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation/Components/NavigationComponent.h>
#include <AiPlugin/UtilityAI/Framework/AiActionQueue.h>
#include <AiPlugin/UtilityAI/Framework/AiPerceptionManager.h>
#include <AiPlugin/UtilityAI/Impl/AlBotBehaviors.h>
#include <AiPlugin/UtilityAI/Impl/GameAiActions.h>
#include <AiPlugin/UtilityAI/Impl/GameAiPerceptions.h>
#include <Core/World/World.h>
#include <RendererCore/Components/BlackboardComponent.h>

namespace
{
  constexpr float s_fMaxShootDistance = 15.0f;
  constexpr float s_fMeleeDistance = 2.5f;

  const ezAiPerceptionTarget* GetTarget(const ezAiPerceptionManager& perceptionManager)
  {
    ezHybridArray<const ezAiPerception*, 4> perceptions;
    perceptionManager.GetPerceptionsOfType("ezAiPerceptionTarget", perceptions);

    if (perceptions.IsEmpty())
      return nullptr;

    return static_cast<const ezAiPerceptionTarget*>(perceptions[0]);
  }

  void QueueSetBlackboard(ezAiActionQueue& inout_ActionQueue, ezStringView sName, const ezVariant& value, bool bNoCancel = false)
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName.Assign(sName);
    pCmd->m_Value = value;
    pCmd->m_bNoCancel = bNoCancel;
    inout_ActionQueue.QueueAction(pCmd);
  }
} // namespace

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiActionAlShootAt);

ezAiActionAlShootAt::ezAiActionAlShootAt() = default;
ezAiActionAlShootAt::~ezAiActionAlShootAt() = default;

void ezAiActionAlShootAt::Reset()
{
  m_hTarget.Invalidate();
  m_MaxAngle = ezAngle::MakeFromDegree(20);
  m_bCanceled = false;
  m_bShooting = false;
}

void ezAiActionAlShootAt::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.SetFormat("{} (angle {}, nav state {})", m_bShooting ? "Shooting" : "Aiming", ezArgF(m_LastAngle.GetDegree(), 0), m_uiNavState);
}

ezAiActionResult ezAiActionAlShootAt::Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog)
{
  auto pBlackboard = ezBlackboardComponent::FindBlackboard(owner);
  if (pBlackboard == nullptr)
  {
    ezLog::Error(pLog, "ezAiActionAlShootAt: No Blackboard available.");
    return ezAiActionResult::Failed;
  }

  ezGameObject* pTarget = nullptr;
  if (m_bCanceled || !owner.GetWorld()->TryGetObject(m_hTarget, pTarget))
  {
    pBlackboard->SetEntryValue("Shoot", 0);
    return ezAiActionResult::Finished;
  }

  ezAiNavigationComponent* pNav;
  if (!owner.TryGetComponentOfBaseType(pNav))
  {
    ezLog::Error(pLog, "ezAiActionAlShootAt: No ezAiNavigationComponent found.");
    return ezAiActionResult::Failed;
  }

  const ezVec2 vTargetPos = pTarget->GetGlobalPosition().GetAsVec2();
  pNav->TurnTowards(vTargetPos);

  m_LastAngle = pNav->GetTurnAngleTowards(vTargetPos);
  m_uiNavState = pNav->GetState().GetValue();
  m_bShooting = ezMath::Abs(m_LastAngle.GetDegree()) < m_MaxAngle.GetDegree();
  pBlackboard->SetEntryValue("Shoot", m_bShooting ? 1 : 0);

  return ezAiActionResult::Succeded;
}

void ezAiActionAlShootAt::Cancel(ezGameObject& owner)
{
  m_bCanceled = true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiBehaviorScore ezAiBehaviorAlIdle::DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager)
{
  ezAiBehaviorScore res;
  res.SetScore(ezAiScoreCategory::Idle, 0.5f);
  return res;
}

void ezAiBehaviorAlIdle::ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue)
{
  inout_ActionQueue.CancelCurrentActions(owner);

  QueueSetBlackboard(inout_ActionQueue, "Shoot", -1); // holster weapon

  const ezUInt32 uiRnd = owner.GetWorld()->GetRandomNumberGenerator().UIntInRange(10);
  if (uiRnd < 4)
  {
    QueueSetBlackboard(inout_ActionQueue, "PlayIdleAction", static_cast<ezInt32>(uiRnd % 2)); // random idle animation
  }

  auto* pWait = ezAiActionWait::Create();
  pWait->m_Duration = ezTime::Seconds(5);
  inout_ActionQueue.QueueAction(pWait);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiBehaviorScore ezAiBehaviorAlPatrol::DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager)
{
  ezAiBehaviorScore res;
  res.SetScore(ezAiScoreCategory::ActiveIdle, 0.5f);
  return res;
}

void ezAiBehaviorAlPatrol::ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue)
{
  // the cooldown starts when the behavior gets activated, so it includes the walking time
  m_Cooldown = ezTime::Seconds(10.0 + owner.GetWorld()->GetRandomNumberGenerator().DoubleZeroToOneExclusive() * 15.0);

  inout_ActionQueue.CancelCurrentActions(owner);

  QueueSetBlackboard(inout_ActionQueue, "Shoot", -1); // holster weapon
  QueueSetBlackboard(inout_ActionQueue, "PlayIdleAction", -1);

  auto* pNav = ezAiActionNavigateToRandomPoint::Create();
  pNav->m_vCenter = owner.GetGlobalPosition();
  pNav->m_fRadius = 20.0f;
  pNav->m_fSpeed = 1.0f; // walk slowly
  pNav->m_bAllowPartialPath = true;
  inout_ActionQueue.QueueAction(pNav);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void ezAiBehaviorAlInvestigate::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager)
{
  ref_PerceptionManager.FlagPerceptionTypeAsNeeded("ezAiPerceptionTarget");
}

ezAiBehaviorScore ezAiBehaviorAlInvestigate::DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager)
{
  const ezAiPerceptionTarget* pTarget = GetTarget(perceptionManager);

  if (pTarget == nullptr || pTarget->m_bVisible || pTarget->m_LastSeen == m_DoneLastSeen)
    return {};

  ezAiBehaviorScore res;
  res.SetScore(ezAiScoreCategory::Investigate, 0.6f);
  res.m_pPerception = pTarget;
  return res;
}

void ezAiBehaviorAlInvestigate::ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue)
{
  const ezAiPerceptionTarget* pTarget = static_cast<const ezAiPerceptionTarget*>(pPerception);
  m_ActiveLastSeen = pTarget->m_LastSeen;

  inout_ActionQueue.CancelCurrentActions(owner);

  QueueSetBlackboard(inout_ActionQueue, "PlayIdleAction", -1);
  QueueSetBlackboard(inout_ActionQueue, "Shoot", 0); // draw weapon

  auto* pNav = ezAiActionNavigateTo::Create();
  pNav->m_vTargetPosition = pTarget->m_vGlobalPosition;
  pNav->m_fSpeed = pTarget->m_bAlerted ? 3.0f : 1.5f; // run when attacked
  pNav->m_bAllowPartialPath = true;
  inout_ActionQueue.QueueAction(pNav);
}

void ezAiBehaviorAlInvestigate::ReactivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue)
{
  const ezAiPerceptionTarget* pTarget = static_cast<const ezAiPerceptionTarget*>(pPerception);

  if (pTarget->m_LastSeen != m_ActiveLastSeen)
  {
    // new information, e.g. got damaged
    ActivateBehavior(owner, pPerception, inout_ActionQueue);
  }
}

ezAiBehavior::ContinuationState ezAiBehaviorAlInvestigate::ContinueBehavior(ezGameObject& owner, ezAiActionQueue& inout_ActionQueue)
{
  ContinuationState res;

  if (inout_ActionQueue.IsEmpty())
  {
    m_DoneLastSeen = m_ActiveLastSeen;
    res.m_bEndBehavior = true;
  }

  return res;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void ezAiBehaviorAlSearch::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager)
{
  ref_PerceptionManager.FlagPerceptionTypeAsNeeded("ezAiPerceptionTarget");
}

ezAiBehaviorScore ezAiBehaviorAlSearch::DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager)
{
  const ezAiPerceptionTarget* pTarget = GetTarget(perceptionManager);

  if (pTarget == nullptr || pTarget->m_bVisible || pTarget->m_LastSeen == m_DoneLastSeen)
    return {};

  ezAiBehaviorScore res;
  res.SetScore(ezAiScoreCategory::Investigate, 0.3f);
  res.m_pPerception = pTarget;
  return res;
}

void ezAiBehaviorAlSearch::ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue)
{
  const ezAiPerceptionTarget* pTarget = static_cast<const ezAiPerceptionTarget*>(pPerception);
  m_ActiveLastSeen = pTarget->m_LastSeen;
  m_uiAttempt = 0;

  inout_ActionQueue.CancelCurrentActions(owner);

  QueueSetBlackboard(inout_ActionQueue, "PlayIdleAction", -1);
  QueueSetBlackboard(inout_ActionQueue, "Shoot", 0); // draw weapon

  // first look towards where the target was last seen
  auto* pTurn = ezAiActionTurnTowards::Create();
  pTurn->m_vTargetPosition = pTarget->m_vGlobalPosition;
  pTurn->m_Timeout = ezTime::Seconds(1.5);
  inout_ActionQueue.QueueAction(pTurn);
}

ezAiBehavior::ContinuationState ezAiBehaviorAlSearch::ContinueBehavior(ezGameObject& owner, ezAiActionQueue& inout_ActionQueue)
{
  ContinuationState res;

  if (!inout_ActionQueue.IsEmpty())
    return res;

  if (m_uiAttempt < 3)
  {
    auto* pNav = ezAiActionNavigateToRandomPoint::Create();
    pNav->m_vCenter = owner.GetGlobalPosition();
    pNav->m_fRadius = 10.0f + m_uiAttempt * 3.0f;
    pNav->m_fSpeed = 1.5f;
    pNav->m_bAllowPartialPath = true;
    inout_ActionQueue.QueueAction(pNav);

    ++m_uiAttempt;
    return res;
  }

  m_DoneLastSeen = m_ActiveLastSeen;
  res.m_bEndBehavior = true;
  return res;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void ezAiBehaviorAlEngage::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager)
{
  ref_PerceptionManager.FlagPerceptionTypeAsNeeded("ezAiPerceptionTarget");
}

ezAiBehaviorScore ezAiBehaviorAlEngage::DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager)
{
  const ezAiPerceptionTarget* pTarget = GetTarget(perceptionManager);

  if (pTarget == nullptr || !pTarget->m_bVisible || pTarget->m_fDistance < s_fMaxShootDistance * 0.9f)
    return {};

  ezAiBehaviorScore res;
  res.SetScore(ezAiScoreCategory::Combat, 0.5f);
  res.m_pPerception = pTarget;
  return res;
}

void ezAiBehaviorAlEngage::ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue)
{
  const ezAiPerceptionTarget* pTarget = static_cast<const ezAiPerceptionTarget*>(pPerception);
  m_vReferencePosition = pTarget->m_vGlobalPosition;

  inout_ActionQueue.CancelCurrentActions(owner);

  QueueSetBlackboard(inout_ActionQueue, "PlayIdleAction", -1);
  QueueSetBlackboard(inout_ActionQueue, "Shoot", -1);

  auto* pNav = ezAiActionNavigateToRandomPoint::Create();
  pNav->m_vCenter = pTarget->m_vGlobalPosition;
  pNav->m_fRadius = ezMath::Min(10.0f, s_fMaxShootDistance * 0.5f);
  pNav->m_fMaxDistToCenter = s_fMaxShootDistance * 0.9f;
  pNav->m_fSpeed = 3.0f;
  inout_ActionQueue.QueueAction(pNav);
}

void ezAiBehaviorAlEngage::ReactivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue)
{
  const ezAiPerceptionTarget* pTarget = static_cast<const ezAiPerceptionTarget*>(pPerception);

  if ((pTarget->m_vGlobalPosition - m_vReferencePosition).GetLength() > 2.0f)
  {
    // the target has moved too far, pick a new point
    ActivateBehavior(owner, pPerception, inout_ActionQueue);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void ezAiBehaviorAlShoot::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager)
{
  ref_PerceptionManager.FlagPerceptionTypeAsNeeded("ezAiPerceptionTarget");
}

ezAiBehaviorScore ezAiBehaviorAlShoot::DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager)
{
  const ezAiPerceptionTarget* pTarget = GetTarget(perceptionManager);

  if (pTarget == nullptr || !pTarget->m_bVisible)
    return {};

  // hysteresis: start shooting when closer than 90% of the range, stop when further away than 110%
  const float fMaxDist = s_fMaxShootDistance * (m_bActive ? 1.1f : 0.9f);
  if (pTarget->m_fDistance > fMaxDist)
    return {};

  ezAiBehaviorScore res;
  res.SetScore(ezAiScoreCategory::Combat, 0.7f);
  res.m_pPerception = pTarget;
  return res;
}

void ezAiBehaviorAlShoot::ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue)
{
  const ezAiPerceptionTarget* pTarget = static_cast<const ezAiPerceptionTarget*>(pPerception);
  m_bActive = true;

  inout_ActionQueue.CancelCurrentActions(owner);

  QueueSetBlackboard(inout_ActionQueue, "PlayIdleAction", -1);
  QueueSetBlackboard(inout_ActionQueue, "Shoot", 0); // draw weapon

  auto* pStop = ezAiActionStopWalking::Create();
  pStop->m_fWithinDistance = 0.5f;
  inout_ActionQueue.QueueAction(pStop);

  auto* pShoot = ezAiActionAlShootAt::Create();
  pShoot->m_hTarget = pTarget->m_hTarget;
  inout_ActionQueue.QueueAction(pShoot);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void ezAiBehaviorAlMelee::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager)
{
  ref_PerceptionManager.FlagPerceptionTypeAsNeeded("ezAiPerceptionTarget");
}

ezAiBehaviorScore ezAiBehaviorAlMelee::DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager)
{
  const ezAiPerceptionTarget* pTarget = GetTarget(perceptionManager);

  if (pTarget == nullptr || !pTarget->m_bVisible || pTarget->m_fDistance > s_fMeleeDistance)
    return {};

  ezAiBehaviorScore res;
  res.SetScore(ezAiScoreCategory::Combat, 0.9f);
  res.m_pPerception = pTarget;
  return res;
}

void ezAiBehaviorAlMelee::ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue)
{
  const ezAiPerceptionTarget* pTarget = static_cast<const ezAiPerceptionTarget*>(pPerception);

  inout_ActionQueue.CancelCurrentActions(owner);

  QueueSetBlackboard(inout_ActionQueue, "PlayIdleAction", -1);
  QueueSetBlackboard(inout_ActionQueue, "Shoot", 0);

  auto* pStop = ezAiActionStopWalking::Create();
  pStop->m_fWithinDistance = 0.5f;
  inout_ActionQueue.QueueAction(pStop);

  auto* pTurn = ezAiActionTurnTowards::Create();
  pTurn->m_hTargetObject = pTarget->m_hTarget;
  pTurn->m_Timeout = ezTime::Seconds(1);
  inout_ActionQueue.QueueAction(pTurn);

  // the animation graph sets "Melee" to -2 when the attack is finished
  auto* pAttack = ezAiActionBlackboardSetAndWait::Create();
  pAttack->m_sEntryName.Assign("Melee");
  pAttack->m_SetValue = 1;
  pAttack->m_WaitValue = -2;
  pAttack->m_Timeout = ezTime::Seconds(3);
  inout_ActionQueue.QueueAction(pAttack);

  QueueSetBlackboard(inout_ActionQueue, "Melee", -1, true);

  // give the animation graph time to see the negative value, otherwise it doesn't restart the attack animation
  auto* pWait = ezAiActionWait::Create();
  pWait->m_Duration = ezTime::Seconds(0.1);
  inout_ActionQueue.QueueAction(pWait);
}

ezAiBehavior::ContinuationState ezAiBehaviorAlMelee::ContinueBehavior(ezGameObject& owner, ezAiActionQueue& inout_ActionQueue)
{
  // don't interrupt the attack
  ContinuationState res;
  res.m_bAllowBehaviorSwitch = inout_ActionQueue.IsEmpty();
  res.m_bEndBehavior = inout_ActionQueue.IsEmpty();
  return res;
}
