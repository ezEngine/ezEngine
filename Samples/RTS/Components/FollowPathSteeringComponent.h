#pragma once

#include <Core/World/World.h>
#include <Core/World/GameObject.h>
#include <RTS/Components/SteeringBehaviorComponent.h>

class FollowPathSteeringComponent;
typedef ezComponentManagerSimple<FollowPathSteeringComponent> FollowPathSteeringComponentManager;

class FollowPathSteeringComponent : public SteeringBehaviorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(FollowPathSteeringComponent, FollowPathSteeringComponentManager);

public:
  FollowPathSteeringComponent();

  void SetPath(const ezDeque<ezVec3>* pPath) { m_pPath = pPath; }

private:
  virtual void Update();

  const ezDeque<ezVec3>* m_pPath;
};

