#pragma once

#include <Core/World/World.h>
#include <Core/World/GameObject.h>

class SteeringBehaviorComponent;
typedef ezComponentManager<SteeringBehaviorComponent> SteeringBehaviorComponentManager;

class SteeringBehaviorComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(SteeringBehaviorComponent, SteeringBehaviorComponentManager);

public:
  SteeringBehaviorComponent();

  static const ezInt32 g_iSteeringDirections = 16;
  static ezVec3 g_vSteeringDirections[g_iSteeringDirections];

  const float* GetDirectionDesire() const { return m_fDirectionDesire; }
  const float* GetDirectionWhisker() const { return m_fDirectionWhisker; }

private:
  virtual void Update() { }

protected:
  float m_fDirectionDesire[g_iSteeringDirections];
  float m_fDirectionWhisker[g_iSteeringDirections];
};

