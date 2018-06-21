#include <PCH.h>

#include <RtsGamePlugin/AI/GuardLocationUtility.h>
#include <RtsGamePlugin/Components/UnitComponent.h>

void RtsGuardLocationAiUtility::Activate(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent)
{
}

void RtsGuardLocationAiUtility::Deactivate(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent)
{
}

void RtsGuardLocationAiUtility::Execute(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent, ezTime tNow)
{
  RtsUnitComponent* pUnit = static_cast<RtsUnitComponent*>(pOwnerComponent);

  // shoot at close by enemies
  ezGameObject* pEnemy = pUnit->AttackClosestEnemey(7.0f, 10.0f);

  if (pEnemy)
  {
    ezVec3 vDiff = pEnemy->GetGlobalPosition() - pOwnerObject->GetGlobalPosition();

    if (vDiff.GetLengthSquared() > ezMath::Square(5.0f))
    {
      vDiff.Normalize();

      // harass the enemy when it moves away
      RtsMsgNavigateTo msg;
      msg.m_vTargetPosition = pOwnerObject->GetGlobalPosition().GetAsVec2() + vDiff.GetAsVec2();

      pOwnerObject->SendMessage(msg);
    }
  }
}

double RtsGuardLocationAiUtility::ComputePriority(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent) const
{
  RtsUnitComponent* pUnit = static_cast<RtsUnitComponent*>(pOwnerComponent);

  if (pUnit->m_UnitMode == RtsUnitMode::GuardLocation)
  {
    return 10.0f;
  }

  return 0;
}
