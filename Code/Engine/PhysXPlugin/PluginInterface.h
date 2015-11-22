#pragma once

namespace physx
{
  class PxPhysics;
  class PxCooking;
}

class ezPhysXInterface
{
public:

  virtual physx::PxPhysics* GetPhysXAPI() = 0;




};

