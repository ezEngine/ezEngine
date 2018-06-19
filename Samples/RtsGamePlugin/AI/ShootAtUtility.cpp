#include <PCH.h>

#include <RtsGamePlugin/AI/ShootAtUtility.h>
#include <RtsGamePlugin/Components/UnitComponent.h>
#include <RtsGamePlugin/GameState/RtsGameState.h>

double RtsShootAtAiUtility::ComputePriority(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent) const
{
  RtsUnitComponent* pUnit = static_cast<RtsUnitComponent*>(pOwnerComponent);

  if (pUnit->m_UnitMode != RtsUnitMode::AttackUnit &&
      pUnit->m_UnitMode != RtsUnitMode::ShootAtPosition)
    return 0;

  if (pUnit->m_UnitMode == RtsUnitMode::AttackUnit)
  {
    ezGameObject* pTarget = nullptr;
    if (!pOwnerObject->GetWorld()->TryGetObject(pUnit->m_hAssignedUnitToAttack, pTarget) || pTarget == pOwnerObject)
    {
      pUnit->m_hAssignedUnitToAttack.Invalidate();

      // TODO: not very clean
      pUnit->m_UnitMode = RtsUnitMode::Idle;

      return 0;
    }
  }

  return 100;
}

void RtsShootAtAiUtility::Activate(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent)
{
}

void RtsShootAtAiUtility::Deactivate(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent)
{
}

void RtsShootAtAiUtility::Execute(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent, ezTime tNow)
{
  RtsUnitComponent* pUnit = static_cast<RtsUnitComponent*>(pOwnerComponent);

  ezVec2 vTargetPos = pUnit->m_vAssignedShootAtPosition;

  if (pUnit->m_UnitMode == RtsUnitMode::AttackUnit)
  {
    ezGameObject* pTarget = nullptr;
    if (!pOwnerObject->GetWorld()->TryGetObject(pUnit->m_hAssignedUnitToAttack, pTarget) || pTarget == pOwnerObject)
    {
      pUnit->m_hAssignedUnitToAttack.Invalidate();

      // TODO: not very clean
      pUnit->m_UnitMode = RtsUnitMode::Idle;
      return;
    }

    vTargetPos = pTarget->GetGlobalPosition().GetAsVec2();

    if ((pOwnerObject->GetGlobalPosition().GetAsVec2() - vTargetPos).GetLengthSquared() > 100)
    {
      RtsMsgNavigateTo msg;
      msg.m_vTargetPosition = vTargetPos;

      pOwnerObject->SendMessage(msg);
      return;
    }
  }

  if (tNow - pUnit->m_TimeLastShot >= ezTime::Seconds(0.75))
  {
    pUnit->m_TimeLastShot = tNow;

    RtsMsgSetTarget msg;
    msg.m_hObject = pUnit->m_hAssignedUnitToAttack;
    msg.m_vPosition = pUnit->m_vAssignedShootAtPosition;

    ezGameObject* pSpawned = RtsGameState::GetSingleton()->SpawnNamedObjectAt(pOwnerObject->GetGlobalTransform(), "ProtonTorpedo1", pOwnerObject->GetTeamID());

    pSpawned->PostMessage(msg, ezObjectMsgQueueType::AfterInitialized);
  }
}
