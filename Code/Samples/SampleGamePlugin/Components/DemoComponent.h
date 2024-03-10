#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

// BEGIN-DOCS-CODE-SNIPPET: customcomp-manager
using DemoComponentManager = ezComponentManagerSimple<class DemoComponent, ezComponentUpdateType::WhenSimulating>;
// END-DOCS-CODE-SNIPPET

// BEGIN-DOCS-CODE-SNIPPET: customcomp-class
class DemoComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(DemoComponent, ezComponent, DemoComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // DemoComponent

public:
  DemoComponent();
  ~DemoComponent();

private:
  void Update();

  float m_fAmplitude = 1.0f;                     // [ property ]
  ezAngle m_Speed = ezAngle::MakeFromDegree(90); // [ property ]
};
// END-DOCS-CODE-SNIPPET
