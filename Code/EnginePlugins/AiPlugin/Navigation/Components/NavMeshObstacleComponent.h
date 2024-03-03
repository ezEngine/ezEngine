#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>

using ezNavMeshObstacleComponentManager = ezComponentManager<class ezNavMeshObstacleComponent, ezBlockStorageType::Compact>;

class EZ_AIPLUGIN_DLL ezNavMeshObstacleComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezNavMeshObstacleComponent, ezComponent, ezNavMeshObstacleComponentManager);

public:
  ezNavMeshObstacleComponent();
  ~ezNavMeshObstacleComponent();

protected:
  virtual void OnActivated() override;
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  void InvalidateSectors();
};
