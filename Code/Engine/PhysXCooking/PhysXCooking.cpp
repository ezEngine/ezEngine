#include <PhysXCooking/PCH.h>
#include <PhysXCooking/PhysXCooking.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Configuration/AbstractInterfaceRegistry.h>
#include <PxPhysicsAPI.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(PhysX, PhysXCooking)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "PhysXPlugin"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
  }

  ON_CORE_SHUTDOWN
  {
  }

  ON_ENGINE_STARTUP
  {
    ezPhysXCooking::Startup();
  }

  ON_ENGINE_SHUTDOWN
  {
    ezPhysXCooking::Shutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION


PxCooking* ezPhysXCooking::s_pCooking = nullptr;
ezPhysXInterface* ezPhysXCooking::s_pPhysX = nullptr;

void ezPhysXCooking::Startup()
{
  s_pPhysX = ezAbstractInterfaceRegistry::RetrieveImplementationForInterface<ezPhysXInterface>("ezPhysXInterface");

  PxCookingParams params = PxCookingParams(s_pPhysX->GetPhysXAPI()->getTolerancesScale());
  params.targetPlatform = PxPlatform::ePC;

  s_pCooking = PxCreateCooking(PX_PHYSICS_VERSION, s_pPhysX->GetPhysXAPI()->getFoundation(), params);
  EZ_ASSERT_DEV(s_pCooking != nullptr, "Initializing PhysX cooking API failed");
}

void ezPhysXCooking::Shutdown()
{
  if (s_pCooking)
  {
    s_pCooking->release();
    s_pCooking = nullptr;
  }
}

void ezPhysXCooking::CookMesh()
{

}
