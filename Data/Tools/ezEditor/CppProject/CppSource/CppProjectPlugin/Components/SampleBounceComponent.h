#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <CppProjectPlugin/CppProjectPluginDLL.h>

using SampleBounceComponentManager = ezComponentManagerSimple<class SampleBounceComponent, ezComponentUpdateType::WhenSimulating>;

class SampleBounceComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(SampleBounceComponent, ezComponent, SampleBounceComponentManager);

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
  SampleBounceComponent();
  ~SampleBounceComponent();

private:
  void Update();

  float m_fAmplitude = 1.0f;                     // [ property ]
  ezAngle m_Speed = ezAngle::MakeFromDegree(90); // [ property ]
};
