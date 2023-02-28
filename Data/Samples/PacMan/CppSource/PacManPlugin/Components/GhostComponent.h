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
  // GhostComponent

public:
  GhostComponent();
  ~GhostComponent();

private:
  void Update();

  WalkDirection m_Direction = WalkDirection::Up;

  ezPrefabResourceHandle m_hDisappear;

  ezSharedPtr<ezBlackboard> m_pStateBlackboard;

  float m_fSpeed = 2.0f;
};
