#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation/Components/NavigationComponent.h>
#include <AiPlugin/UtilityAI/Impl/GameAiActions.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <GameEngine/Gameplay/SpawnComponent.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiActionWait);

ezAiActionWait::ezAiActionWait() = default;
ezAiActionWait::~ezAiActionWait() = default;

void ezAiActionWait::Reset()
{
  m_Duration = ezTime::MakeZero();
}

void ezAiActionWait::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.SetFormat("Wait: {}", m_Duration);
}

ezAiActionResult ezAiActionWait::Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog)
{
  if (tDiff >= m_Duration)
    return ezAiActionResult::Finished; // or canceled

  m_Duration -= tDiff;
  return ezAiActionResult::Succeded;
}

void ezAiActionWait::Cancel(ezGameObject& owner)
{
  m_Duration = ezTime::MakeZero();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiActionLerpRotation);

ezAiActionLerpRotation::ezAiActionLerpRotation() = default;
ezAiActionLerpRotation::~ezAiActionLerpRotation() = default;

void ezAiActionLerpRotation::Reset()
{
  m_vTurnAxis = ezVec3::MakeAxisZ();
  m_TurnAngle = {};
  m_TurnAnglesPerSec = {};
}

void ezAiActionLerpRotation::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.SetFormat("Turn: {}/{}/{} - {} @ {}/sec", m_vTurnAxis.x, m_vTurnAxis.y, m_vTurnAxis.z, m_TurnAngle, m_TurnAnglesPerSec);
}

ezAiActionResult ezAiActionLerpRotation::Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog)
{
  if (m_TurnAnglesPerSec <= ezAngle())
    return ezAiActionResult::Finished; // or canceled

  if (m_TurnAngle < ezAngle())
  {
    m_TurnAngle = -m_TurnAngle;
    m_vTurnAxis = -m_vTurnAxis;
  }

  const ezAngle turnAmount = tDiff.AsFloatInSeconds() * m_TurnAnglesPerSec;
  const ezAngle toTurn = ezMath::Min(m_TurnAngle, turnAmount);

  ezQuat qRot = ezQuat::MakeFromAxisAndAngle(m_vTurnAxis, toTurn);

  const ezQuat qCurRot = owner.GetGlobalRotation();

  owner.SetGlobalRotation(qRot * qCurRot);

  if (turnAmount >= m_TurnAngle)
    return ezAiActionResult::Finished;

  m_TurnAngle -= toTurn;
  return ezAiActionResult::Succeded;
}

void ezAiActionLerpRotation::Cancel(ezGameObject& owner)
{
  m_TurnAnglesPerSec = {};
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiActionLerpPosition);

ezAiActionLerpPosition::ezAiActionLerpPosition() = default;
ezAiActionLerpPosition::~ezAiActionLerpPosition() = default;

void ezAiActionLerpPosition::Reset()
{
  m_fSpeed = 0.0f;
  m_vLocalSpaceSlide = ezVec3::MakeZero();
}

void ezAiActionLerpPosition::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.SetFormat("Slide: {}/{}/{} @ {}/sec", m_vLocalSpaceSlide.x, m_vLocalSpaceSlide.y, m_vLocalSpaceSlide.z, m_fSpeed);
}

ezAiActionResult ezAiActionLerpPosition::Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog)
{
  if (m_vLocalSpaceSlide.IsZero())
    return ezAiActionResult::Finished; // or canceled

  if (m_fSpeed <= 0.0f)
  {
    ezLog::Error(pLog, "ezAiActionLerpPosition: Invalid speed '{}'", m_fSpeed);
    return ezAiActionResult::Failed;
  }

  ezVec3 vSlideDir = m_vLocalSpaceSlide;
  const float fMaxSlide = vSlideDir.GetLengthAndNormalize();

  const float fSlideAmount = tDiff.AsFloatInSeconds() * m_fSpeed;
  const float fSlideDist = ezMath::Min(fMaxSlide, fSlideAmount);
  const ezVec3 vSlide = fSlideDist * vSlideDir;

  const ezVec3 vCurGlobalPos = owner.GetGlobalPosition();

  owner.SetGlobalPosition(vCurGlobalPos + owner.GetGlobalRotation() * vSlide);

  if (fSlideAmount >= fMaxSlide)
    return ezAiActionResult::Finished;

  m_vLocalSpaceSlide -= vSlide;
  return ezAiActionResult::Succeded;
}

void ezAiActionLerpPosition::Cancel(ezGameObject& owner)
{
  m_fSpeed = 0.0f;
  m_vLocalSpaceSlide.SetZero();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiActionLerpRotationTowards);

ezAiActionLerpRotationTowards::ezAiActionLerpRotationTowards() = default;
ezAiActionLerpRotationTowards::~ezAiActionLerpRotationTowards() = default;

void ezAiActionLerpRotationTowards::Reset()
{
  m_vTargetPosition = ezVec3::MakeZero();
  m_hTargetObject.Invalidate();
  m_TurnAnglesPerSec = {};
}

void ezAiActionLerpRotationTowards::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.SetFormat("Turn Towards: {}/{}/{} - @{}/sec", m_vTargetPosition.x, m_vTargetPosition.y, m_vTargetPosition.z, m_TurnAnglesPerSec);
}

ezAiActionResult ezAiActionLerpRotationTowards::Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog)
{
  if (m_TurnAnglesPerSec <= ezAngle())
    return ezAiActionResult::Finished; // or canceled

  if (!m_hTargetObject.IsInvalidated())
  {
    ezGameObject* pTarget;
    if (!owner.GetWorld()->TryGetObject(m_hTargetObject, pTarget))
    {
      ezLog::Error(pLog, "ezAiActionLerpRotationTowards: Target object doesn't exist (anymore).");
      return ezAiActionResult::Failed;
    }

    m_vTargetPosition = pTarget->GetGlobalPosition();
  }

  const ezVec3 vOwnPos = owner.GetGlobalPosition();

  ezVec3 vCurDir = owner.GetGlobalDirForwards();
  vCurDir.z = 0.0f;

  ezVec3 vTargetDir = m_vTargetPosition - vOwnPos;
  vTargetDir.z = 0.0f;

  m_vTargetPosition.z = vOwnPos.z;

  if (vCurDir.NormalizeIfNotZero(ezVec3::MakeZero()).Failed())
  {
    ezLog::Error(pLog, "ezAiActionLerpRotationTowards: Invalid current direction {}/{}/{}", vCurDir.x, vCurDir.y, vCurDir.z);
    return ezAiActionResult::Failed;
  }

  if (vTargetDir.NormalizeIfNotZero(ezVec3::MakeZero()).Failed())
  {
    ezLog::Error(pLog, "ezAiActionLerpRotationTowards: Invalid target direction {}/{}/{}", vTargetDir.x, vTargetDir.y, vTargetDir.z);
    return ezAiActionResult::Failed;
  }

  if (vCurDir.IsEqual(vTargetDir, 0.001f))
    return ezAiActionResult::Finished;

  const ezAngle turnAngle = vCurDir.GetAngleBetween(vTargetDir);

  const ezVec3 vTurnAxis = vCurDir.CrossRH(vTargetDir).GetNormalized();

  const ezAngle turnAmount = tDiff.AsFloatInSeconds() * m_TurnAnglesPerSec;
  const ezAngle toTurn = ezMath::Min(turnAngle, turnAmount);

  ezQuat qRot = ezQuat::MakeFromAxisAndAngle(vTurnAxis, toTurn);

  const ezQuat qCurRot = owner.GetGlobalRotation();

  owner.SetGlobalRotation(qRot * qCurRot);

  if (turnAngle - toTurn <= m_TargetReachedAngle)
    return ezAiActionResult::Finished;

  return ezAiActionResult::Succeded;
}

void ezAiActionLerpRotationTowards::Cancel(ezGameObject& owner)
{
  m_TurnAnglesPerSec = {};
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//
// EZ_IMPLEMENT_AICMD(ezAiActionFollowPath);
//
// ezAiActionFollowPath::ezAiActionFollowPath() = default;
// ezAiActionFollowPath::~ezAiActionFollowPath() = default;
//
// void ezAiActionFollowPath::Reset()
//{
//  m_fSpeed = 0.0f;
//  m_hPath.Invalidate();
//}
//
// void ezAiActionFollowPath::GetDebugDesc(ezStringBuilder& inout_sText)
//{
//  inout_sText.Format("Follow Path: @{}/sec", m_fSpeed);
//}
//
// ezAiActionResult ezAiActionFollowPath::Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog)
//{
//  if (m_hPath.IsInvalidated())
//    return ezAiActionResult::Finished; // or canceled
//
//  ezGameObject* pPath;
//  if (!owner.GetWorld()->TryGetObject(m_hPath, pPath))
//  {
//    ezLog::Error(pLog, "ezAiActionFollowPath: Target object doesn't exist (anymore).");
//    return ezAiActionResult::Failed;
//  }
//
//
//  return ezAiActionResult::Succeded;
//}
//
// void ezAiActionFollowPath::Cancel(ezGameObject& owner)
//{
//  m_hPath.Invalidate();
//}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiActionBlackboardSetEntry);

ezAiActionBlackboardSetEntry::ezAiActionBlackboardSetEntry() = default;
ezAiActionBlackboardSetEntry::~ezAiActionBlackboardSetEntry() = default;

void ezAiActionBlackboardSetEntry::Reset()
{
  m_sEntryName.Clear();
  m_Value = {};
}

void ezAiActionBlackboardSetEntry::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.SetFormat("Set Blackboard Entry '{}' to '{}'", m_sEntryName.GetHash(), m_Value);
}

ezAiActionResult ezAiActionBlackboardSetEntry::Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog)
{
  if (m_sEntryName.IsEmpty() || !m_Value.IsValid())
    return ezAiActionResult::Finished; // or canceled

  auto pBlackboard = ezBlackboardComponent::FindBlackboard(&owner);

  if (pBlackboard == nullptr)
  {
    ezLog::Error(pLog, "ezAiActionBlackboardSetEntry: No Blackboard available.");
    return ezAiActionResult::Failed;
  }

  ezLog::Debug("Setting '{}' to {}", m_sEntryName, m_Value.ConvertTo<ezString>());
  pBlackboard->SetEntryValue(m_sEntryName, m_Value);
  return ezAiActionResult::Finished;
}

void ezAiActionBlackboardSetEntry::Cancel(ezGameObject& owner)
{
  if (!m_bNoCancel)
  {
    Reset();
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiActionBlackboardWait);

ezAiActionBlackboardWait::ezAiActionBlackboardWait() = default;
ezAiActionBlackboardWait::~ezAiActionBlackboardWait() = default;

void ezAiActionBlackboardWait::Reset()
{
  m_sEntryName.Clear();
  m_Value = {};
  m_bEquals = true;
}

void ezAiActionBlackboardWait::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.SetFormat("Wait for Blackboard Entry '{}' {} '{}'", m_sEntryName.GetHash(), m_bEquals ? "==" : "!=", m_Value);
}

ezAiActionResult ezAiActionBlackboardWait::Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog)
{
  if (m_sEntryName.IsEmpty())
    return ezAiActionResult::Finished; // or canceled

  auto pBlackboard = ezBlackboardComponent::FindBlackboard(&owner);

  if (pBlackboard == nullptr)
  {
    ezLog::Error(pLog, "ezAiActionBlackboardWait: No Blackboard available.");
    return ezAiActionResult::Failed;
  }

  const ezVariant val = pBlackboard->GetEntryValue(m_sEntryName, m_Value);
  const bool bIsEqual = (val == m_Value);

  if (m_bEquals == bIsEqual)
    return ezAiActionResult::Finished;

  return ezAiActionResult::Succeded;
}

void ezAiActionBlackboardWait::Cancel(ezGameObject& owner)
{
  Reset();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiActionBlackboardSetAndWait);

ezAiActionBlackboardSetAndWait::ezAiActionBlackboardSetAndWait() = default;
ezAiActionBlackboardSetAndWait::~ezAiActionBlackboardSetAndWait() = default;

void ezAiActionBlackboardSetAndWait::Reset()
{
  m_sEntryName.Clear();
  m_SetValue = {};
  m_WaitValue = {};
  m_bEqualsWaitValue = true;
}

void ezAiActionBlackboardSetAndWait::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.SetFormat("Set BB '{}' to {}, wait {} '{}'", m_sEntryName.GetHash(), m_SetValue, m_bEqualsWaitValue ? "==" : "!=", m_WaitValue);
}

ezAiActionResult ezAiActionBlackboardSetAndWait::Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog)
{
  if (m_sEntryName.IsEmpty())
    return ezAiActionResult::Finished; // or canceled

  auto pBlackboard = ezBlackboardComponent::FindBlackboard(&owner);

  if (pBlackboard == nullptr)
  {
    ezLog::Error(pLog, "ezAiActionBlackboardSetAndWait: No Blackboard available.");
    return ezAiActionResult::Failed;
  }

  if (m_SetValue.IsValid())
  {
    pBlackboard->SetEntryValue(m_sEntryName, m_SetValue);
    m_SetValue = {};
  }

  const ezVariant val = pBlackboard->GetEntryValue(m_sEntryName, m_WaitValue);
  const bool bIsEqual = (val == m_WaitValue);

  if (m_bEqualsWaitValue == bIsEqual)
    return ezAiActionResult::Finished;

  return ezAiActionResult::Succeded;
}

void ezAiActionBlackboardSetAndWait::Cancel(ezGameObject& owner)
{
  Reset();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiActionCCMoveTo);

ezAiActionCCMoveTo::ezAiActionCCMoveTo() = default;
ezAiActionCCMoveTo::~ezAiActionCCMoveTo() = default;

void ezAiActionCCMoveTo::Reset()
{
  m_vTargetPosition = ezVec3::MakeZero();
  m_hTargetObject.Invalidate();
  m_fSpeed = 0.0f;
  m_fReachedDistSQR = 1.0f;
}

void ezAiActionCCMoveTo::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.SetFormat("CCMoveTo: {}/{}/{} - @{}/sec", m_vTargetPosition.x, m_vTargetPosition.y, m_vTargetPosition.z, m_fSpeed);
}

ezAiActionResult ezAiActionCCMoveTo::Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog)
{
  if (m_fSpeed <= 0.0f)
    return ezAiActionResult::Finished; // or canceled

  if (!m_hTargetObject.IsInvalidated())
  {
    ezGameObject* pTarget;
    if (!owner.GetWorld()->TryGetObject(m_hTargetObject, pTarget))
    {
      ezLog::Error(pLog, "ezAiActionCCMoveTo: Target object doesn't exist (anymore).");
      return ezAiActionResult::Failed;
    }

    m_vTargetPosition = pTarget->GetGlobalPosition();
  }

  const ezVec3 vOwnPos = owner.GetGlobalPosition();
  ezVec3 vDir = m_vTargetPosition - vOwnPos;
  vDir.z = 0.0f; // TODO: not the best idea


  if (vDir.GetLengthSquared() <= m_fReachedDistSQR)
    return ezAiActionResult::Finished;

  vDir.z = 0.0f;

  ezQuat qRot = ezQuat::MakeShortestRotation(ezVec3::MakeAxisX(), vDir.GetNormalized());
  owner.SetGlobalRotation(qRot);

  // const ezVec3 vLocalDir = -owner.GetGlobalTransform().m_qRotation * vDir;

  ezMsgMoveCharacterController msg;
  msg.m_fMoveForwards = m_fSpeed;
  // msg.m_fMoveForwards = ezMath::Clamp(vLocalDir.x, 0.0f, 1.0f);
  // msg.m_fMoveBackwards = ezMath::Clamp(-vLocalDir.x, 0.0f, 1.0f);
  // msg.m_fStrafeLeft = ezMath::Clamp(-vLocalDir.y, 0.0f, 1.0f);
  // msg.m_fStrafeRight = ezMath::Clamp(vLocalDir.y, 0.0f, 1.0f);
  owner.SendMessage(msg);

  return ezAiActionResult::Succeded;
}

void ezAiActionCCMoveTo::Cancel(ezGameObject& owner)
{
  m_fSpeed = 0.0f;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiActionSpawn);

ezAiActionSpawn::ezAiActionSpawn() = default;
ezAiActionSpawn::~ezAiActionSpawn() = default;

void ezAiActionSpawn::Reset()
{
  m_sChildObjectName.Clear();
}

void ezAiActionSpawn::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.SetFormat("Spawn: {}", m_sChildObjectName.GetHash());
}

ezAiActionResult ezAiActionSpawn::Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog)
{
  if (m_sChildObjectName.IsEmpty())
    return ezAiActionResult::Finished;

  ezGameObject* pSpawner = owner.FindChildByName(m_sChildObjectName);
  if (pSpawner == nullptr)
  {
    ezLog::Error(pLog, "ezAiActionSpawn: Child with name hash '{}' doesn't exist.", m_sChildObjectName.GetHash());
    return ezAiActionResult::Failed;
  }

  ezSpawnComponent* pSpawn = nullptr;
  if (!pSpawner->TryGetComponentOfBaseType(pSpawn))
  {
    ezLog::Error(pLog, "ezAiActionSpawn: ezSpawnComponent does not exist on child with name hash '{}'.", m_sChildObjectName.GetHash());
    return ezAiActionResult::Failed;
  }

  if (pSpawn->TriggerManualSpawn())
    return ezAiActionResult::Finished;
  else
    return ezAiActionResult::Succeded; // wait
}

void ezAiActionSpawn::Cancel(ezGameObject& owner)
{
  m_sChildObjectName.Clear();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiActionQuip);

ezAiActionQuip::ezAiActionQuip() = default;
ezAiActionQuip::~ezAiActionQuip() = default;

void ezAiActionQuip::Reset()
{
  m_sLogMsg.Clear();
}

void ezAiActionQuip::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.SetFormat("Log: {}", m_sLogMsg);
}

ezAiActionResult ezAiActionQuip::Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog)
{
  if (m_sLogMsg.IsEmpty())
    return ezAiActionResult::Finished; // or canceled

  ezLog::Info(m_sLogMsg);

  return ezAiActionResult::Finished;
}

void ezAiActionQuip::Cancel(ezGameObject& owner)
{
  Reset();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiActionNavigateTo);

ezAiActionNavigateTo::ezAiActionNavigateTo() = default;
ezAiActionNavigateTo::~ezAiActionNavigateTo() = default;

void ezAiActionNavigateTo::Reset()
{
  m_pTargetPosition = nullptr;
  m_fSpeed = 0.0f;
  m_fReachedDist = 1.0f;
  m_bStarted = false;
}

void ezAiActionNavigateTo::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.SetFormat("NavTo: {}/{}/{} - @{}/sec", m_pTargetPosition->x, m_pTargetPosition->y, m_pTargetPosition->z, m_fSpeed);
}

ezAiActionResult ezAiActionNavigateTo::Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog)
{
  if (m_pTargetPosition == nullptr || m_fSpeed <= 0.0f)
    return ezAiActionResult::Finished; // or canceled

  ezAiNavigationComponent* pNav;
  if (!owner.TryGetComponentOfBaseType(pNav))
  {
    ezLog::Error(pLog, "No ezAiNavigationComponent found.");
    return ezAiActionResult::Failed;
  }

  switch (pNav->GetState())
  {
    case ezAiNavigationComponentState::Idle:
      if (m_bStarted)
      {
        pNav->CancelNavigation();
        return ezAiActionResult::Finished;
      }
      break;

    case ezAiNavigationComponentState::Failed:
      ezLog::Error(pLog, "Path navigation failed.");
      return ezAiActionResult::Failed;
  }

  pNav->m_fReachedDistance = m_fReachedDist;
  pNav->m_fSpeed = m_fSpeed;

  m_bStarted = true;
  pNav->SetDestination(*m_pTargetPosition, false);
  return ezAiActionResult::Succeded;
}

void ezAiActionNavigateTo::Cancel(ezGameObject& owner)
{
  m_fSpeed = 0.0f;
}
