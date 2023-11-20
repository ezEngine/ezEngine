#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation/Components/NavigationComponent.h>
#include <AiPlugin/UtilityAI/Framework/AiActionQueue.h>
#include <AiPlugin/UtilityAI/Framework/AiPerceptionManager.h>
#include <AiPlugin/UtilityAI/Impl/AlBotBehaviors.h>
#include <AiPlugin/UtilityAI/Impl/GameAiActions.h>
#include <AiPlugin/UtilityAI/Impl/GameAiPerceptions.h>
#include <Core/World/World.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <RendererCore/Components/BlackboardComponent.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiBehaviorAlIdle, 1, ezRTTIDefaultAllocator<ezAiBehaviorAlIdle>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_AI_BEHAVIOR_SCORE_PROPERTIES(ezAiScoreCategory::Idle, 0.5f),
    EZ_MEMBER_PROPERTY("Duration", m_Duration)->AddAttributes(new ezDefaultValueAttribute(ezTime::Seconds(5))),
    EZ_MEMBER_PROPERTY("IdleAnimationChance", m_fIdleAnimationChance)->AddAttributes(new ezDefaultValueAttribute(0.4f), new ezClampValueAttribute(0.0f, 1.0f)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiBehaviorAlPatrol, 1, ezRTTIDefaultAllocator<ezAiBehaviorAlPatrol>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_AI_BEHAVIOR_SCORE_PROPERTIES(ezAiScoreCategory::ActiveIdle, 0.5f),
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(20.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("MinCooldown", m_MinCooldown)->AddAttributes(new ezDefaultValueAttribute(ezTime::Seconds(10))),
    EZ_MEMBER_PROPERTY("MaxCooldown", m_MaxCooldown)->AddAttributes(new ezDefaultValueAttribute(ezTime::Seconds(25))),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiBehaviorAlInvestigate, 1, ezRTTIDefaultAllocator<ezAiBehaviorAlInvestigate>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Targets", m_Targets)->AddAttributes(new ezAiInputPinAttribute("ezAiPerceptionTarget"), new ezHiddenAttribute()),
    EZ_AI_BEHAVIOR_SCORE_PROPERTIES(ezAiScoreCategory::Investigate, 0.6f),
    EZ_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new ezDefaultValueAttribute(1.5f), new ezClampValueAttribute(0.0f, ezVariant())),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiBehaviorAlSearch, 1, ezRTTIDefaultAllocator<ezAiBehaviorAlSearch>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Targets", m_Targets)->AddAttributes(new ezAiInputPinAttribute("ezAiPerceptionTarget"), new ezHiddenAttribute()),
    EZ_AI_BEHAVIOR_SCORE_PROPERTIES(ezAiScoreCategory::Investigate, 0.3f),
    EZ_MEMBER_PROPERTY("NumPoints", m_uiNumPoints)->AddAttributes(new ezDefaultValueAttribute(3)),
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("RadiusIncrease", m_fRadiusIncrease)->AddAttributes(new ezDefaultValueAttribute(3.0f)),
    EZ_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new ezDefaultValueAttribute(1.5f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("TurnTimeout", m_TurnTimeout)->AddAttributes(new ezDefaultValueAttribute(ezTime::Seconds(1.5))),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiBehaviorAlCheckHint, 1, ezRTTIDefaultAllocator<ezAiBehaviorAlCheckHint>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Hints", m_Hints)->AddAttributes(new ezAiInputPinAttribute("ezAiPerceptionHint"), new ezHiddenAttribute()),
    EZ_AI_BEHAVIOR_SCORE_PROPERTIES(ezAiScoreCategory::Investigate, 0.8f),
    EZ_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new ezDefaultValueAttribute(2.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("RepathDistance", m_fRepathDistance)->AddAttributes(new ezDefaultValueAttribute(3.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("TurnTimeout", m_TurnTimeout)->AddAttributes(new ezDefaultValueAttribute(ezTime::Seconds(1.0))),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiBehaviorAlEngage, 1, ezRTTIDefaultAllocator<ezAiBehaviorAlEngage>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Targets", m_Targets)->AddAttributes(new ezAiInputPinAttribute("ezAiPerceptionTarget"), new ezHiddenAttribute()),
    EZ_AI_BEHAVIOR_SCORE_PROPERTIES(ezAiScoreCategory::Combat, 0.5f),
    EZ_MEMBER_PROPERTY("ShootDistance", m_fShootDistance)->AddAttributes(new ezDefaultValueAttribute(15.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new ezDefaultValueAttribute(3.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("RepathDistance", m_fRepathDistance)->AddAttributes(new ezDefaultValueAttribute(2.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("LostSightGracePeriod", m_LostSightGracePeriod)->AddAttributes(new ezDefaultValueAttribute(ezTime::MakeFromSeconds(1.0)), new ezClampValueAttribute(ezTime::MakeZero(), ezVariant())),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiBehaviorAlShoot, 1, ezRTTIDefaultAllocator<ezAiBehaviorAlShoot>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Targets", m_Targets)->AddAttributes(new ezAiInputPinAttribute("ezAiPerceptionTarget"), new ezHiddenAttribute()),
    EZ_AI_BEHAVIOR_SCORE_PROPERTIES(ezAiScoreCategory::Combat, 0.7f),
    EZ_MEMBER_PROPERTY("LostSightGracePeriod", m_LostSightGracePeriod)->AddAttributes(new ezDefaultValueAttribute(ezTime::MakeFromSeconds(0.5)), new ezClampValueAttribute(ezTime::MakeZero(), ezVariant())),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiBehaviorAlMelee, 1, ezRTTIDefaultAllocator<ezAiBehaviorAlMelee>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Targets", m_Targets)->AddAttributes(new ezAiInputPinAttribute("ezAiPerceptionTarget"), new ezHiddenAttribute()),
    EZ_AI_BEHAVIOR_SCORE_PROPERTIES(ezAiScoreCategory::Combat, 0.9f),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  void QueueSetBlackboard(ezAiActionQueue& inout_ActionQueue, const ezHashedString& sName, const ezVariant& value, bool bNoCancel = false)
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName = sName;
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

ezAiBehaviorAlIdle::ezAiBehaviorAlIdle() = default;

ezAiBehaviorAlPatrol::ezAiBehaviorAlPatrol() = default;

ezAiBehaviorAlInvestigate::ezAiBehaviorAlInvestigate() = default;

ezAiBehaviorAlSearch::ezAiBehaviorAlSearch() = default;

ezAiBehaviorAlCheckHint::ezAiBehaviorAlCheckHint() = default;

ezAiBehaviorAlEngage::ezAiBehaviorAlEngage() = default;

ezAiBehaviorAlShoot::ezAiBehaviorAlShoot() = default;

ezAiBehaviorAlMelee::ezAiBehaviorAlMelee() = default;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiBehaviorScore ezAiBehaviorAlIdle::DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const
{
  return MakeScore(ctx);
}

void ezAiBehaviorAlIdle::OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const
{
  ctx.m_ActionQueue.CancelCurrentActions(ctx.m_Owner);

  QueueSetBlackboard(ctx.m_ActionQueue, ezMakeHashedString("Shoot"), -1); // holster weapon

  ezRandom& rng = ctx.m_Owner.GetWorld()->GetRandomNumberGenerator();
  if (rng.DoubleZeroToOneExclusive() < m_fIdleAnimationChance)
  {
    QueueSetBlackboard(ctx.m_ActionQueue, ezMakeHashedString("PlayIdleAction"), static_cast<ezInt32>(rng.UIntInRange(2))); // random idle animation
  }

  auto* pWait = ezAiActionWait::Create();
  pWait->m_Duration = m_Duration;
  ctx.m_ActionQueue.QueueAction(pWait);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiBehaviorScore ezAiBehaviorAlPatrol::DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const
{
  return MakeScore(ctx);
}

void ezAiBehaviorAlPatrol::OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const
{
  InstanceData* pData = static_cast<InstanceData*>(pInstanceData);

  // picked per activation, GetCooldownDuration() returns it once the behavior ends
  pData->m_Cooldown = ezMath::Lerp(m_MinCooldown, m_MaxCooldown, ctx.m_Owner.GetWorld()->GetRandomNumberGenerator().DoubleZeroToOneExclusive());

  ctx.m_ActionQueue.CancelCurrentActions(ctx.m_Owner);

  QueueSetBlackboard(ctx.m_ActionQueue, ezMakeHashedString("Shoot"), -1); // holster weapon
  QueueSetBlackboard(ctx.m_ActionQueue, ezMakeHashedString("PlayIdleAction"), -1);

  auto* pNav = ezAiActionNavigateToRandomPoint::Create();
  pNav->m_vCenter = ctx.m_Owner.GetGlobalPosition();
  pNav->m_fRadius = m_fRadius;
  pNav->m_fSpeed = m_fSpeed;
  pNav->m_bAllowPartialPath = true;
  ctx.m_ActionQueue.QueueAction(pNav);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void ezAiBehaviorAlInvestigate::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const
{
  ref_PerceptionManager.FlagAsNeeded(m_Targets);
}

ezAiBehaviorScore ezAiBehaviorAlInvestigate::DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const
{
  const InstanceData* pData = static_cast<const InstanceData*>(pInstanceData);

  return ScoreCandidates<ezAiPerceptionTarget>(ctx, m_Targets, [&](const ezAiPerceptionTarget& t)
    { return !t.m_bVisible && t.m_LastSeen > pData->m_DoneTime; });
}

void ezAiBehaviorAlInvestigate::OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const
{
  InstanceData* pData = static_cast<InstanceData*>(pInstanceData);

  const ezAiPerceptionTarget* pTarget = static_cast<const ezAiPerceptionTarget*>(pPerception);
  pData->m_hActiveTarget = pTarget->m_hTarget;
  pData->m_ActiveLastSeen = pTarget->m_LastSeen;

  ctx.m_ActionQueue.CancelCurrentActions(ctx.m_Owner);

  QueueSetBlackboard(ctx.m_ActionQueue, ezMakeHashedString("PlayIdleAction"), -1);
  QueueSetBlackboard(ctx.m_ActionQueue, ezMakeHashedString("Shoot"), 0); // draw weapon

  auto* pNav = ezAiActionNavigateTo::Create();
  pNav->m_vTargetPosition = pTarget->m_vGlobalPosition;
  pNav->m_fSpeed = m_fSpeed;
  pNav->m_bAllowPartialPath = true;
  ctx.m_ActionQueue.QueueAction(pNav);
}

void ezAiBehaviorAlInvestigate::OnReactivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const
{
  InstanceData* pData = static_cast<InstanceData*>(pInstanceData);

  const ezAiPerceptionTarget* pTarget = static_cast<const ezAiPerceptionTarget*>(pPerception);

  if (pTarget->m_hTarget != pData->m_hActiveTarget || pTarget->m_LastSeen != pData->m_ActiveLastSeen)
  {
    // another target, or the target was seen again in between
    OnActivateBehavior(ctx, pInstanceData, pPerception);
  }
}

ezAiBehavior::ContinuationState ezAiBehaviorAlInvestigate::ContinueBehavior(const ezAiContext& ctx, void* pInstanceData) const
{
  InstanceData* pData = static_cast<InstanceData*>(pInstanceData);

  ContinuationState res;

  if (ctx.m_ActionQueue.IsEmpty())
  {
    pData->m_DoneTime = ctx.m_Now;
    res.m_bEndBehavior = true;
  }

  return res;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void ezAiBehaviorAlSearch::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const
{
  ref_PerceptionManager.FlagAsNeeded(m_Targets);
}

ezAiBehaviorScore ezAiBehaviorAlSearch::DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const
{
  const InstanceData* pData = static_cast<const InstanceData*>(pInstanceData);

  return ScoreCandidates<ezAiPerceptionTarget>(ctx, m_Targets, [&](const ezAiPerceptionTarget& t)
    { return !t.m_bVisible && t.m_LastSeen > pData->m_DoneTime; });
}

void ezAiBehaviorAlSearch::OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const
{
  InstanceData* pData = static_cast<InstanceData*>(pInstanceData);

  const ezAiPerceptionTarget* pTarget = static_cast<const ezAiPerceptionTarget*>(pPerception);
  pData->m_uiAttempt = 0;

  ctx.m_ActionQueue.CancelCurrentActions(ctx.m_Owner);

  QueueSetBlackboard(ctx.m_ActionQueue, ezMakeHashedString("PlayIdleAction"), -1);
  QueueSetBlackboard(ctx.m_ActionQueue, ezMakeHashedString("Shoot"), 0); // draw weapon

  // first look towards where the target was last seen
  auto* pTurn = ezAiActionTurnTowards::Create();
  pTurn->m_vTargetPosition = pTarget->m_vGlobalPosition;
  pTurn->m_Timeout = m_TurnTimeout;
  ctx.m_ActionQueue.QueueAction(pTurn);
}

ezAiBehavior::ContinuationState ezAiBehaviorAlSearch::ContinueBehavior(const ezAiContext& ctx, void* pInstanceData) const
{
  InstanceData* pData = static_cast<InstanceData*>(pInstanceData);

  ContinuationState res;

  if (!ctx.m_ActionQueue.IsEmpty())
    return res;

  if (pData->m_uiAttempt < m_uiNumPoints)
  {
    auto* pNav = ezAiActionNavigateToRandomPoint::Create();
    pNav->m_vCenter = ctx.m_Owner.GetGlobalPosition();
    pNav->m_fRadius = m_fRadius + pData->m_uiAttempt * m_fRadiusIncrease;
    pNav->m_fSpeed = m_fSpeed;
    pNav->m_bAllowPartialPath = true;
    ctx.m_ActionQueue.QueueAction(pNav);

    ++pData->m_uiAttempt;
    return res;
  }

  pData->m_DoneTime = ctx.m_Now;
  res.m_bEndBehavior = true;
  return res;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void ezAiBehaviorAlCheckHint::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const
{
  ref_PerceptionManager.FlagAsNeeded(m_Hints);
}

ezAiBehaviorScore ezAiBehaviorAlCheckHint::DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const
{
  const InstanceData* pData = static_cast<const InstanceData*>(pInstanceData);

  ezAiBehaviorScore score = ScoreCandidates<ezAiPerceptionHint>(ctx, m_Hints, [](const ezAiPerceptionHint& h)
    { return true; });

  if (score.GetScore() <= 0.0f && pData->m_bActive)
  {
    // the hint is gone, but we haven't arrived yet
    score.SetScore(m_Category, m_fScore);
  }

  return score;
}

void ezAiBehaviorAlCheckHint::OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const
{
  InstanceData* pData = static_cast<InstanceData*>(pInstanceData);

  if (pPerception != nullptr)
  {
    pData->m_vPosition = static_cast<const ezAiPerceptionHint*>(pPerception)->m_vSourcePosition;
  }

  pData->m_bActive = true;

  ctx.m_ActionQueue.CancelCurrentActions(ctx.m_Owner);

  QueueSetBlackboard(ctx.m_ActionQueue, ezMakeHashedString("PlayIdleAction"), -1);
  QueueSetBlackboard(ctx.m_ActionQueue, ezMakeHashedString("Shoot"), 0); // draw weapon

  auto* pTurn = ezAiActionTurnTowards::Create();
  pTurn->m_vTargetPosition = pData->m_vPosition;
  pTurn->m_Timeout = m_TurnTimeout;
  ctx.m_ActionQueue.QueueAction(pTurn);

  auto* pNav = ezAiActionNavigateTo::Create();
  pNav->m_vTargetPosition = pData->m_vPosition;
  pNav->m_fSpeed = m_fSpeed;
  pNav->m_bAllowPartialPath = true;
  ctx.m_ActionQueue.QueueAction(pNav);
}

void ezAiBehaviorAlCheckHint::OnReactivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const
{
  const InstanceData* pData = static_cast<const InstanceData*>(pInstanceData);

  if (pPerception == nullptr)
    return;

  const ezAiPerceptionHint* pHint = static_cast<const ezAiPerceptionHint*>(pPerception);
  if ((pHint->m_vSourcePosition - pData->m_vPosition).GetLength() > m_fRepathDistance)
  {
    OnActivateBehavior(ctx, pInstanceData, pPerception);
  }
}

void ezAiBehaviorAlCheckHint::OnDeactivateBehavior(const ezAiContext& ctx, void* pInstanceData) const
{
  static_cast<InstanceData*>(pInstanceData)->m_bActive = false;
}

ezAiBehavior::ContinuationState ezAiBehaviorAlCheckHint::ContinueBehavior(const ezAiContext& ctx, void* pInstanceData) const
{
  InstanceData* pData = static_cast<InstanceData*>(pInstanceData);

  ContinuationState res;

  if (ctx.m_ActionQueue.IsEmpty())
  {
    pData->m_bActive = false;
    res.m_bEndBehavior = true;
  }

  return res;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void ezAiBehaviorAlEngage::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const
{
  ref_PerceptionManager.FlagAsNeeded(m_Targets);
}

ezAiBehaviorScore ezAiBehaviorAlEngage::DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const
{
  return ScoreCandidates<ezAiPerceptionTarget>(ctx, m_Targets, [&](const ezAiPerceptionTarget& t)
    { return t.m_bVisible || ctx.m_Now - t.m_LastSeen < m_LostSightGracePeriod; });
}

void ezAiBehaviorAlEngage::OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const
{
  InstanceData* pData = static_cast<InstanceData*>(pInstanceData);

  const ezAiPerceptionTarget* pTarget = static_cast<const ezAiPerceptionTarget*>(pPerception);
  pData->m_vReferencePosition = pTarget->m_vGlobalPosition;

  ctx.m_ActionQueue.CancelCurrentActions(ctx.m_Owner);

  QueueSetBlackboard(ctx.m_ActionQueue, ezMakeHashedString("PlayIdleAction"), -1);
  QueueSetBlackboard(ctx.m_ActionQueue, ezMakeHashedString("Shoot"), -1);

  auto* pNav = ezAiActionNavigateToRandomPoint::Create();
  pNav->m_vCenter = pTarget->m_vGlobalPosition;
  pNav->m_fRadius = ezMath::Min(10.0f, m_fShootDistance * 0.5f);
  pNav->m_fMaxDistToCenter = m_fShootDistance * 0.9f;
  pNav->m_fSpeed = m_fSpeed;
  ctx.m_ActionQueue.QueueAction(pNav);
}

void ezAiBehaviorAlEngage::OnReactivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const
{
  InstanceData* pData = static_cast<InstanceData*>(pInstanceData);

  const ezAiPerceptionTarget* pTarget = static_cast<const ezAiPerceptionTarget*>(pPerception);

  if ((pTarget->m_vGlobalPosition - pData->m_vReferencePosition).GetLength() > m_fRepathDistance)
  {
    // the target has moved too far, pick a new point
    OnActivateBehavior(ctx, pInstanceData, pPerception);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void ezAiBehaviorAlShoot::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const
{
  ref_PerceptionManager.FlagAsNeeded(m_Targets);
}

ezAiBehaviorScore ezAiBehaviorAlShoot::DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const
{
  const InstanceData* pData = static_cast<const InstanceData*>(pInstanceData);

  return ScoreCandidates<ezAiPerceptionTarget>(ctx, m_Targets, [&](const ezAiPerceptionTarget& t)
    {
      if (t.m_bVisible)
        return true;

      // the active target was just lost, keep shooting at it for a moment
      return pData->m_bActive && t.m_hTarget == pData->m_hActiveTarget && ctx.m_Now - t.m_LastSeen < m_LostSightGracePeriod; });
}

void ezAiBehaviorAlShoot::OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const
{
  InstanceData* pData = static_cast<InstanceData*>(pInstanceData);

  const ezAiPerceptionTarget* pTarget = static_cast<const ezAiPerceptionTarget*>(pPerception);
  pData->m_hActiveTarget = pTarget->m_hTarget;
  pData->m_bActive = true;

  ctx.m_ActionQueue.CancelCurrentActions(ctx.m_Owner);

  QueueSetBlackboard(ctx.m_ActionQueue, ezMakeHashedString("PlayIdleAction"), -1);
  QueueSetBlackboard(ctx.m_ActionQueue, ezMakeHashedString("Shoot"), 0); // draw weapon

  auto* pStop = ezAiActionStopWalking::Create();
  pStop->m_fWithinDistance = 0.5f;
  ctx.m_ActionQueue.QueueAction(pStop);

  auto* pShoot = ezAiActionAlShootAt::Create();
  pShoot->m_hTarget = pTarget->m_hTarget;
  ctx.m_ActionQueue.QueueAction(pShoot);
}

void ezAiBehaviorAlShoot::OnReactivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const
{
  const ezAiPerceptionTarget* pTarget = static_cast<const ezAiPerceptionTarget*>(pPerception);

  if (pTarget != nullptr && pTarget->m_hTarget != static_cast<InstanceData*>(pInstanceData)->m_hActiveTarget)
  {
    // another target scores better now
    OnActivateBehavior(ctx, pInstanceData, pPerception);
  }
}

void ezAiBehaviorAlShoot::OnDeactivateBehavior(const ezAiContext& ctx, void* pInstanceData) const
{
  static_cast<InstanceData*>(pInstanceData)->m_bActive = false;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void ezAiBehaviorAlMelee::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const
{
  ref_PerceptionManager.FlagAsNeeded(m_Targets);
}

ezAiBehaviorScore ezAiBehaviorAlMelee::DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const
{
  return ScoreCandidates<ezAiPerceptionTarget>(ctx, m_Targets, [](const ezAiPerceptionTarget& t)
    { return t.m_bVisible; });
}

void ezAiBehaviorAlMelee::OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const
{
  const ezAiPerceptionTarget* pTarget = static_cast<const ezAiPerceptionTarget*>(pPerception);

  ctx.m_ActionQueue.CancelCurrentActions(ctx.m_Owner);

  QueueSetBlackboard(ctx.m_ActionQueue, ezMakeHashedString("PlayIdleAction"), -1);
  // the animation graph blends the (upper body) shooting pose on top of everything while "Shoot" >= 0, which distorts the melee animation
  QueueSetBlackboard(ctx.m_ActionQueue, ezMakeHashedString("Shoot"), -1);

  auto* pStop = ezAiActionStopWalking::Create();
  pStop->m_fWithinDistance = 0.5f;
  ctx.m_ActionQueue.QueueAction(pStop);

  auto* pTurn = ezAiActionTurnTowards::Create();
  pTurn->m_hTargetObject = pTarget->m_hTarget;
  pTurn->m_Timeout = ezTime::Seconds(1);
  ctx.m_ActionQueue.QueueAction(pTurn);

  // the animation graph sets "Melee" to -2 when the attack is finished
  auto* pAttack = ezAiActionBlackboardSetAndWait::Create();
  pAttack->m_sEntryName = ezMakeHashedString("Melee");
  pAttack->m_SetValue = 1;
  pAttack->m_WaitValue = -2;
  pAttack->m_Timeout = ezTime::Seconds(3);
  ctx.m_ActionQueue.QueueAction(pAttack);

  QueueSetBlackboard(ctx.m_ActionQueue, ezMakeHashedString("Melee"), -1, true);

  // give the animation graph time to see the negative value, otherwise it doesn't restart the attack animation
  auto* pWait = ezAiActionWait::Create();
  pWait->m_Duration = ezTime::Seconds(0.1);
  ctx.m_ActionQueue.QueueAction(pWait);
}

ezAiBehavior::ContinuationState ezAiBehaviorAlMelee::ContinueBehavior(const ezAiContext& ctx, void* pInstanceData) const
{
  // don't interrupt the attack
  ContinuationState res;
  res.m_bAllowBehaviorSwitch = ctx.m_ActionQueue.IsEmpty();
  res.m_bEndBehavior = ctx.m_ActionQueue.IsEmpty();
  return res;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezTime ezAiBehaviorAlPatrol::GetCooldownDuration(const void* pInstanceData) const
{
  return static_cast<const InstanceData*>(pInstanceData)->m_Cooldown;
}

#define AI_INSTANCE_DATA_DESC(Type)                                  \
  bool Type::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const \
  {                                                                  \
    out_desc.FillFromType<InstanceData>();                           \
    return true;                                                     \
  }

AI_INSTANCE_DATA_DESC(ezAiBehaviorAlPatrol);
AI_INSTANCE_DATA_DESC(ezAiBehaviorAlInvestigate);
AI_INSTANCE_DATA_DESC(ezAiBehaviorAlCheckHint);
AI_INSTANCE_DATA_DESC(ezAiBehaviorAlSearch);
AI_INSTANCE_DATA_DESC(ezAiBehaviorAlEngage);
AI_INSTANCE_DATA_DESC(ezAiBehaviorAlShoot);

#undef AI_INSTANCE_DATA_DESC
