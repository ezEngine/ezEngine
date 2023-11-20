#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Framework/AiActionQueue.h>
#include <AiPlugin/UtilityAI/Framework/AiPerceptionManager.h>
#include <AiPlugin/UtilityAI/Impl/GameAiBehaviors.h>
#include <AiPlugin/UtilityAI/Impl/GameAiPerceptions.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiBehaviorGoToPOI::ezAiBehaviorGoToPOI() = default;
ezAiBehaviorGoToPOI::~ezAiBehaviorGoToPOI() = default;

void ezAiBehaviorGoToPOI::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager)
{
  ref_PerceptionManager.FlagPerceptionTypeAsNeeded("ezAiPerceptionPOI");
}

ezAiBehaviorScore ezAiBehaviorGoToPOI::DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager)
{
  if (!perceptionManager.HasPerceptionsOfType("ezAiPerceptionPOI"))
    return {};

  ezHybridArray<const ezAiPerception*, 32> perceptions;
  perceptionManager.GetPerceptionsOfType("ezAiPerceptionPOI", perceptions);

  const ezVec3 vOwnerPos = owner.GetGlobalPosition();
  float fClosestSqr = ezMath::HighValue<float>();

  ezAiBehaviorScore res;

  for (const ezAiPerception* pPerception0 : perceptions)
  {
    const ezAiPerceptionPOI* pPerception = static_cast<const ezAiPerceptionPOI*>(pPerception0);

    const float fDistSqr = (pPerception->m_vGlobalPosition - vOwnerPos).GetLengthSquared();

    if (fDistSqr < fClosestSqr)
    {
      fClosestSqr = fDistSqr;
      res.m_pPerception = pPerception;
    }
  }

  if (res.m_pPerception != nullptr)
  {
    res.SetScore(ezAiScoreCategory::Command, 0.5f);
  }

  return res;
}

void ezAiBehaviorGoToPOI::ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception0, ezAiActionQueue& inout_ActionQueue)
{
  const ezAiPerceptionPOI* pPerception = static_cast<const ezAiPerceptionPOI*>(pPerception0);

  m_vTargetPosition = pPerception->m_vGlobalPosition;

  inout_ActionQueue.CancelCurrentActions(owner);

  //{
  //  auto pAct = ezAiActionLerpRotationTowards::Create();
  //  pAct->m_vTargetPosition = pPerception->m_vGlobalPosition;
  //  pAct->m_TurnAnglesPerSec = ezAngle::MakeFromDegree(90);
  //  inout_ActionQueue.QueueAction(pAct);
  //}
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName.Assign("MoveForwards");
    pCmd->m_Value = 1.0f;
    inout_ActionQueue.QueueAction(pCmd);
  }
  //{
  //  auto pAct = ezAiActionCCMoveTo::Create();
  //  pAct->m_vTargetPosition = pPerception->m_vGlobalPosition;
  //  pAct->m_fSpeed = 1.0f;
  //  inout_ActionQueue.QueueAction(pAct);
  //}
  {
    auto pAct = ezAiActionNavigateTo::Create();
    pAct->m_pTargetPosition = &m_vTargetPosition;
    pAct->m_fSpeed = 3.0f;
    pAct->m_fReachedDist = 3.0f;
    inout_ActionQueue.QueueAction(pAct);
  }
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName.Assign("MoveForwards");
    pCmd->m_Value = 0.0f;
    pCmd->m_bNoCancel = true;
    inout_ActionQueue.QueueAction(pCmd);
  }
}

void ezAiBehaviorGoToPOI::ReactivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception0, ezAiActionQueue& inout_ActionQueue)
{
  const ezAiPerceptionPOI* pPerception = static_cast<const ezAiPerceptionPOI*>(pPerception0);
  m_vTargetPosition = pPerception->m_vGlobalPosition;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiBehaviorWander::ezAiBehaviorWander() = default;
ezAiBehaviorWander::~ezAiBehaviorWander() = default;

void ezAiBehaviorWander::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager)
{
  ref_PerceptionManager.FlagPerceptionTypeAsNeeded("ezAiPerceptionWander");
}

ezAiBehaviorScore ezAiBehaviorWander::DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager)
{
  if (!perceptionManager.HasPerceptionsOfType("ezAiPerceptionWander"))
    return {};

  ezHybridArray<const ezAiPerception*, 32> perceptions;
  perceptionManager.GetPerceptionsOfType("ezAiPerceptionWander", perceptions);

  if (perceptions.IsEmpty())
    return {};

  const ezUInt32 uiPerceptionIdx = owner.GetWorld()->GetRandomNumberGenerator().UIntInRange(perceptions.GetCount());

  ezAiBehaviorScore res;
  res.SetScore(ezAiScoreCategory::ActiveIdle, 0.1f);
  res.m_pPerception = perceptions[uiPerceptionIdx];

  return res;
}

void ezAiBehaviorWander::ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception0, ezAiActionQueue& inout_ActionQueue)
{
  const ezAiPerceptionWander* pPerception = static_cast<const ezAiPerceptionWander*>(pPerception0);
  m_vTargetPosition = pPerception->m_vGlobalPosition;

  inout_ActionQueue.CancelCurrentActions(owner);

  //{
  //  auto pAct = ezAiActionLerpRotationTowards::Create();
  //  pAct->m_vTargetPosition = pPerception->m_vGlobalPosition;
  //  pAct->m_TurnAnglesPerSec = ezAngle::MakeFromDegree(90);
  //  inout_ActionQueue.QueueAction(pAct);
  //}
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName.Assign("MoveForwards");
    pCmd->m_Value = 0.5f;
    inout_ActionQueue.QueueAction(pCmd);
  }
  //{
  //  auto pAct = ezAiActionCCMoveTo::Create();
  //  pAct->m_vTargetPosition = pPerception->m_vGlobalPosition;
  //  pAct->m_fSpeed = 0.5f;
  //  inout_ActionQueue.QueueAction(pAct);
  //}
  {
    auto pAct = ezAiActionNavigateTo::Create();
    pAct->m_pTargetPosition = &m_vTargetPosition;
    pAct->m_fSpeed = 2.0f;
    pAct->m_fReachedDist = 3.0f;
    inout_ActionQueue.QueueAction(pAct);
  }
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName.Assign("MoveForwards");
    pCmd->m_Value = 0.0f;
    pCmd->m_bNoCancel = true;
    inout_ActionQueue.QueueAction(pCmd);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiBehaviorGoToCheckpoint::ezAiBehaviorGoToCheckpoint() = default;
ezAiBehaviorGoToCheckpoint::~ezAiBehaviorGoToCheckpoint() = default;

void ezAiBehaviorGoToCheckpoint::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager)
{
  ref_PerceptionManager.FlagPerceptionTypeAsNeeded("ezAiPerceptionCheckpoint");
}

ezAiBehaviorScore ezAiBehaviorGoToCheckpoint::DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager)
{
  if (!perceptionManager.HasPerceptionsOfType("ezAiPerceptionCheckpoint"))
    return {};

  ezHybridArray<const ezAiPerception*, 32> perceptions;
  perceptionManager.GetPerceptionsOfType("ezAiPerceptionCheckpoint", perceptions);

  if (perceptions.IsEmpty())
    return {};

  const ezUInt32 uiPerceptionIdx = owner.GetWorld()->GetRandomNumberGenerator().UIntInRange(perceptions.GetCount());

  ezAiBehaviorScore res;
  res.SetScore(ezAiScoreCategory::ActiveIdle, 0.2f);
  res.m_pPerception = perceptions[uiPerceptionIdx];

  return res;
}

void ezAiBehaviorGoToCheckpoint::ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception0, ezAiActionQueue& inout_ActionQueue)
{
  const ezAiPerceptionCheckpoint* pPerception = static_cast<const ezAiPerceptionCheckpoint*>(pPerception0);
  m_vTargetPosition = pPerception->m_vGlobalPosition;

  inout_ActionQueue.CancelCurrentActions(owner);

  //{
  //  auto pAct = ezAiActionLerpRotationTowards::Create();
  //  pAct->m_vTargetPosition = pPerception->m_vGlobalPosition;
  //  pAct->m_TurnAnglesPerSec = ezAngle::MakeFromDegree(90);
  //  inout_ActionQueue.QueueAction(pAct);
  //}
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName.Assign("MoveForwards");
    pCmd->m_Value = 0.5f;
    inout_ActionQueue.QueueAction(pCmd);
  }
  //{
  //  auto pAct = ezAiActionCCMoveTo::Create();
  //  pAct->m_vTargetPosition = pPerception->m_vGlobalPosition;
  //  pAct->m_fSpeed = 0.5f;
  //  inout_ActionQueue.QueueAction(pAct);
  //}
  {
    auto pAct = ezAiActionNavigateTo::Create();
    pAct->m_pTargetPosition = &m_vTargetPosition;
    pAct->m_fSpeed = 2.0f;
    pAct->m_fReachedDist = 3.0f;
    inout_ActionQueue.QueueAction(pAct);
  }
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName.Assign("MoveForwards");
    pCmd->m_Value = 0.0f;
    pCmd->m_bNoCancel = true;
    inout_ActionQueue.QueueAction(pCmd);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiBehaviorShoot::ezAiBehaviorShoot() = default;
ezAiBehaviorShoot::~ezAiBehaviorShoot() = default;

void ezAiBehaviorShoot::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager)
{
  ref_PerceptionManager.FlagPerceptionTypeAsNeeded("ezAiPerceptionPOI");
}

ezAiBehaviorScore ezAiBehaviorShoot::DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager)
{
  if (!perceptionManager.HasPerceptionsOfType("ezAiPerceptionPOI"))
    return {};

  ezHybridArray<const ezAiPerception*, 32> perceptions;
  perceptionManager.GetPerceptionsOfType("ezAiPerceptionPOI", perceptions);

  const ezVec3 vOwnerPos = owner.GetGlobalPosition();
  const ezVec3 vOwnerDir = owner.GetGlobalDirForwards();
  float fClosest = ezMath::HighValue<float>();

  ezAiBehaviorScore res;

  for (const ezAiPerception* pPerception0 : perceptions)
  {
    const ezAiPerceptionPOI* pPerception = static_cast<const ezAiPerceptionPOI*>(pPerception0);

    ezVec3 vDirTo = pPerception->m_vGlobalPosition - vOwnerPos;
    const float fDist = vDirTo.GetLengthAndNormalize();

    if (fDist < 2.0f || fDist > 5.0f)
      continue;

    if (fDist >= fClosest)
      continue;

    fClosest = fDist;
    res.m_pPerception = pPerception;
  }

  if (res.m_pPerception != nullptr)
  {
    res.SetScore(ezAiScoreCategory::Combat, 0.1f);
  }

  return res;
}

void ezAiBehaviorShoot::ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception0, ezAiActionQueue& inout_ActionQueue)
{
  const ezAiPerceptionPOI* pPerception = static_cast<const ezAiPerceptionPOI*>(pPerception0);

  inout_ActionQueue.CancelCurrentActions(owner);

  {
    auto pAct = ezAiActionLerpRotationTowards::Create();
    pAct->m_vTargetPosition = pPerception->m_vGlobalPosition;
    pAct->m_TurnAnglesPerSec = ezAngle::MakeFromDegree(90);
    inout_ActionQueue.QueueAction(pAct);
  }
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName.Assign("Aim");
    pCmd->m_Value = 1;
    inout_ActionQueue.QueueAction(pCmd);
  }
  {
    auto* pCmd = ezAiActionWait::Create();
    pCmd->m_Duration = ezTime::Seconds(0.5);
    inout_ActionQueue.QueueAction(pCmd);
  }
  {
    auto pAct = ezAiActionSpawn::Create();
    pAct->m_sChildObjectName = ezTempHashedString("Spawn");
    inout_ActionQueue.QueueAction(pAct);
  }
  {
    auto* pCmd = ezAiActionBlackboardSetAndWait::Create();
    pCmd->m_sEntryName.Assign("Shoot");
    pCmd->m_SetValue = 1;
    pCmd->m_WaitValue = 0;
    inout_ActionQueue.QueueAction(pCmd);
  }
  {
    auto* pCmd = ezAiActionWait::Create();
    pCmd->m_Duration = ezTime::Seconds(0.5);
    inout_ActionQueue.QueueAction(pCmd);
  }
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName.Assign("Aim");
    pCmd->m_Value = 0;
    pCmd->m_bNoCancel = true;
    inout_ActionQueue.QueueAction(pCmd);
  }
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiBehaviorQuip::ezAiBehaviorQuip() = default;
ezAiBehaviorQuip::~ezAiBehaviorQuip() = default;

void ezAiBehaviorQuip::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager)
{
  ref_PerceptionManager.FlagPerceptionTypeAsNeeded("ezAiPerceptionPOI");
}

ezAiBehaviorScore ezAiBehaviorQuip::DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager)
{
  if (!perceptionManager.HasPerceptionsOfType("ezAiPerceptionPOI"))
    return {};

  ezHybridArray<const ezAiPerception*, 32> perceptions;
  perceptionManager.GetPerceptionsOfType("ezAiPerceptionPOI", perceptions);

  ezAiBehaviorScore res;
  res.SetScore(ezAiScoreCategory::Command, 0.6f);

  return res;
}

void ezAiBehaviorQuip::ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception0, ezAiActionQueue& inout_ActionQueue)
{
  inout_ActionQueue.CancelCurrentActions(owner);

  {
    auto pAct = ezAiActionQuip::Create();
    pAct->m_sLogMsg = "I SEE YOU !";
    inout_ActionQueue.QueueAction(pAct);
  }
}

ezTime ezAiBehaviorQuip::GetCooldownDuration()
{
  return ezTime::Seconds(5);
}
