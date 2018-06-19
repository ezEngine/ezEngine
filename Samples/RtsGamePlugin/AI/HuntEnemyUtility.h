#pragma once

#include <RtsGamePlugin/AI/AiUtilitySystem.h>

class RtsHuntEnemyAiUtility : public RtsUnitComponentUtility
{
public:
  virtual void Activate(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent) override;
  virtual void Deactivate(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent) override;
  virtual void Execute(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent, ezTime tNow) override;
  virtual double ComputePriority(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent) const override;

};
