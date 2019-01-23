#pragma once

#include <SampleGamePlugin/SampleGamePlugin.h>
#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>

typedef ezComponentManagerSimple<class DemoComponent, ezComponentUpdateType::WhenSimulating> DemoComponentManager;

class DemoComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(DemoComponent, ezComponent, DemoComponentManager);

public:
  DemoComponent();

  virtual void OnSimulationStarted() override;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void Update();

private:

  float m_fHeight = 1.0f;
  float m_fSpeed;
};
