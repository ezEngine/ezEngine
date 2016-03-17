#pragma once

#include <PhysXPlugin/Basics.h>

namespace physx
{
  class PxPhysics;
  class PxCooking;
}

class ezCollisionFilterConfig;

class ezPhysXInterface
{
public:

  virtual physx::PxPhysics* GetPhysXAPI() = 0;

  virtual ezCollisionFilterConfig& GetCollisionFilterConfig() = 0;

  virtual void LoadCollisionFilters() = 0;
};

