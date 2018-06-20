#include <PCH.h>

#include <RtsGamePlugin/AI/MoveToPositionUtility.h>
#include <RtsGamePlugin/Components/UnitComponent.h>
#include <RtsGamePlugin/GameState/RtsGameState.h>

void RtsMoveToPositionAiUtility::Activate(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent)
{
}

void RtsMoveToPositionAiUtility::Deactivate(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent)
{
  RtsUnitComponent* pUnit = static_cast<RtsUnitComponent*>(pOwnerComponent);

  RtsMsgStopNavigation msg;
  pOwnerObject->SendMessage(msg);
}

void RtsMoveToPositionAiUtility::Execute(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent, ezTime tNow)
{
  RtsUnitComponent* pUnit = static_cast<RtsUnitComponent*>(pOwnerComponent);

  RtsMsgNavigateTo msg;
  msg.m_vTargetPosition = pUnit->m_vAssignedPosition;

  pOwnerObject->SendMessage(msg);

  // TODO: shoot at close by enemies
}

double RtsMoveToPositionAiUtility::ComputePriority(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent) const
{
  RtsUnitComponent* pUnit = static_cast<RtsUnitComponent*>(pOwnerComponent);

  if (pUnit->m_UnitMode == RtsUnitMode::MoveToPosition)
  {
    // always follow the move command with high priority
    return 1000;
  }

  if (pUnit->m_UnitMode == RtsUnitMode::GuardLocation)
  {
    // return to assigned position when strayed too far from it
    return (pOwnerObject->GetGlobalPosition().GetAsVec2() - pUnit->m_vAssignedPosition).GetLengthSquared();
  }

  return 0;
}
