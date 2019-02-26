#pragma once

#include <RtsGamePlugin/RtsGamePluginDLL.h>

class RtsAiUtility;

class RtsAiUtilitySystem
{
public:
  RtsAiUtilitySystem();
  ~RtsAiUtilitySystem();

  void AddUtility(ezUniquePtr<RtsAiUtility>&& pUtility);

  void Reevaluate(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent, ezTime tNow, ezTime frequency);
  bool Execute(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent, ezTime tNow);

private:
  ezTime m_LastUpdate;
  RtsAiUtility* m_pActiveUtility = nullptr;
  ezHybridArray<ezUniquePtr<RtsAiUtility>, 8> m_Utilities;
};

class RtsAiUtility
{
public:
  RtsAiUtility();
  virtual ~RtsAiUtility();

  virtual void Activate(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent) = 0;
  virtual void Deactivate(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent) = 0;
  virtual void Execute(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent, ezTime tNow) = 0;
  virtual double ComputePriority(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent) const = 0;
};

class RtsUnitComponentUtility : public RtsAiUtility
{
public:

};
