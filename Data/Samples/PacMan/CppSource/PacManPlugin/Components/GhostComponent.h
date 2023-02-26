#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <PacManPlugin/PacManPluginDLL.h>

using GhostComponentManager = ezComponentManagerSimple<class GhostComponent, ezComponentUpdateType::WhenSimulating>;

class GhostComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(GhostComponent, ezComponent, GhostComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // SampleBounceComponent

public:
  GhostComponent();
  ~GhostComponent();

private:
  void Update();

  ezUInt32 m_uiDirection = 0;

  ezPrefabResourceHandle m_hDisappear;
};
