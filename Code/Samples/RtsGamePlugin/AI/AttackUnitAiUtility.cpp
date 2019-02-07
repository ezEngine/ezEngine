#include <RtsGamePluginPCH.h>

#include <RtsGamePlugin/AI/AttackUnitAiUtility.h>
#include <RtsGamePlugin/Components/UnitComponent.h>
#include <RtsGamePlugin/GameState/RtsGameState.h>

double RtsAttackUnitAiUtility::ComputePriority(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent) const
{
  RtsUnitComponent* pUnit = static_cast<RtsUnitComponent*>(pOwnerComponent);

  if (pUnit->m_UnitMode != RtsUnitMode::AttackUnit)
    return 0;

  return 100;
}

void RtsAttackUnitAiUtility::Activate(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent)
{
}

void RtsAttackUnitAiUtility::Deactivate(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent)
{
}

void RtsAttackUnitAiUtility::Execute(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent, ezTime tNow)
{
  RtsUnitComponent* pUnit = static_cast<RtsUnitComponent*>(pOwnerComponent);

  ezGameObject* pTarget = nullptr;
  if (!pOwnerObject->GetWorld()->TryGetObject(pUnit->m_hAssignedUnitToAttack, pTarget))
    return;

  const ezVec2 vTargetPos = pTarget->GetGlobalPosition().GetAsVec2();

  if ((pOwnerObject->GetGlobalPosition().GetAsVec2() - vTargetPos).GetLengthSquared() > 100)
  {
    RtsMsgNavigateTo msg;
    msg.m_vTargetPosition = vTargetPos;

    pOwnerObject->SendMessage(msg);
    return;
  }

  pUnit->FireAt(pUnit->m_hAssignedUnitToAttack);
}
