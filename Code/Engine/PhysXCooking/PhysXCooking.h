#pragma once

#include <PhysXCooking/Basics.h>
#include <PhysXPlugin/PluginInterface.h>

using namespace physx;

class EZ_PHYSXCOOKING_DLL ezPhysXCooking
{
public:

  static void Startup();
  static void Shutdown();

  static void CookMesh();


private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(PhysX, PhysXCooking);

  static PxCooking* s_pCooking;
  static ezPhysXInterface* s_pPhysX;
};

