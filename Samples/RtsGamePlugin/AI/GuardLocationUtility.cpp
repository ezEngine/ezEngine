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
  pUnit->AttackClosestEnemey(7.0f, 10.0f);
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
