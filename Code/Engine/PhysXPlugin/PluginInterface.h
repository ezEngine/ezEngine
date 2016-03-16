#pragma once

#include <PhysXPlugin/Basics.h>
#include <GameUtils/Interfaces/PhysicsEngine.h>

namespace physx
{
  class PxPhysics;
  class PxCooking;
}

class ezCollisionFilterConfig;

class EZ_PHYSXPLUGIN_DLL ezPhysXInterface : public ezPhysicsEngineInterface
{
public:

  virtual physx::PxPhysics* GetPhysXAPI() = 0;

  virtual ezCollisionFilterConfig& GetCollisionFilterConfig() = 0;

  virtual void LoadCollisionFilters() = 0;
};

