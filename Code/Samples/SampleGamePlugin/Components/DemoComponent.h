#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

using DemoComponentManager = ezComponentManagerSimple<class DemoComponent, ezComponentUpdateType::WhenSimulating>;

class DemoComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(DemoComponent, ezComponent, DemoComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // DemoComponent

public:
  DemoComponent();
  ~DemoComponent();

private:
  void Update();

  float m_fAmplitude = 1.0f;             // [ property ]
  ezAngle m_Speed = ezAngle::Degree(90); // [ property ]
};
