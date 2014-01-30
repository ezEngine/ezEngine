#pragma once

#include <Core/World/World.h>
#include <Core/World/GameObject.h>

class SteeringBehaviorComponent;
typedef ezComponentManagerNoUpdate<SteeringBehaviorComponent> SteeringBehaviorComponentManager;

class SteeringBehaviorComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(SteeringBehaviorComponent, SteeringBehaviorComponentManager);

public:
  SteeringBehaviorComponent();

  static const ezInt32 g_iSteeringDirections = 32;
  static ezVec3 g_vSteeringDirections[g_iSteeringDirections];

  const float* GetDirectionDesire() const { return m_fDirectionDesire; }
  const float* GetDirectionDanger() const { return m_fDirectionDanger; }

private:
  virtual void Update() { }

protected:
  float m_fDirectionDesire[g_iSteeringDirections];
  float m_fDirectionDanger[g_iSteeringDirections];
};

