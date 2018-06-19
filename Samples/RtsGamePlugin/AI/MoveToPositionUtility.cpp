#include <PCH.h>

#include <RtsGamePlugin/AI/MoveToPositionUtility.h>
#include <RtsGamePlugin/Components/UnitComponent.h>
#include <RtsGamePlugin/GameState/RtsGameState.h>

void RtsMoveToPositionAiUtility::Activate(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent)
{
}

void RtsMoveToPositionAiUtility::Deactivate(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent)
{
}

void RtsMoveToPositionAiUtility::Execute(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent, ezTime tNow)
{
  RtsUnitComponent* pUnit = static_cast<RtsUnitComponent*>(pOwnerComponent);

  RtsMsgNavigateTo msg;
  msg.m_vTargetPosition = pUnit->m_vAssignedPosition;

  pOwnerObject->SendMessage(msg);
}

double RtsMoveToPositionAiUtility::ComputePriority(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent) const
{
  RtsUnitComponent* pUnit = static_cast<RtsUnitComponent*>(pOwnerComponent);

  if (pUnit->m_UnitMode != RtsUnitMode::Idle)
    return 0;

  return (pOwnerObject->GetGlobalPosition().GetAsVec2() - pUnit->m_vAssignedPosition).GetLengthSquared();
}
