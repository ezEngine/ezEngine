#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <RmlUiPlugin/RmlUiInput.h>

using ezRmlUiCanvas3DInteractionComponentManager = ezComponentManagerSimple<class ezRmlUiCanvas3DInteractionExampleComponent, ezComponentUpdateType::WhenSimulating, ezBlockStorageType::Compact, ezWorldUpdatePhase::PostTransform>;

class ezCpuMeshResource;
class ezPhysicsWorldModuleInterface;

class EZ_RMLUIPLUGIN_DLL ezRmlUiCanvas3DInteractionExampleComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRmlUiCanvas3DInteractionExampleComponent, ezComponent, ezRmlUiCanvas3DInteractionComponentManager);

public:
  virtual void OnSimulationStarted() override;

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

public:
  ezRmlUiCanvas3DInteractionExampleComponent();
  ~ezRmlUiCanvas3DInteractionExampleComponent();

  void Update();
  void Interact(ezRmlUiInputSnapshot input);

private:
  
  ezUInt32 m_uiCollisionLayer = 0;
  float m_fMaxDistance = 2.0f;
  ezPhysicsWorldModuleInterface* m_pPhysicsWorldModule = nullptr;
};
